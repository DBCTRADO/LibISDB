/*
  LibISDB
  Copyright(c) 2017 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 @file   StreamBuffer.cpp
 @brief  ストリームバッファ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamBuffer.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


StreamBuffer::StreamBuffer()
	: m_BlockSize(0)
	, m_MinBlockCount(0)
	, m_MaxBlockCount(0)
	, m_SerialPos(0)
{
}


StreamBuffer::~StreamBuffer()
{
	Destroy();
}


bool StreamBuffer::Create(
	size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount,
	DataStorageManager *pDataStorageManager)
{
	LIBISDB_TRACE(
		LIBISDB_STR("StreamBuffer::Create() : %zu bytes (%zu - %zu blocks)\n"),
		BlockSize, MinBlockCount, MaxBlockCount);

	if (!CheckBufferSize(BlockSize, MinBlockCount, MaxBlockCount))
		return false;

	BlockLock Lock(m_Lock);

	m_BlockSize = BlockSize;
	m_MinBlockCount = MinBlockCount;
	m_MaxBlockCount = MaxBlockCount;
	m_Queue.clear();
	m_SerialPos = 0;

	if (pDataStorageManager != nullptr)
		m_DataStorageManager.reset(pDataStorageManager);
	else
		m_DataStorageManager = std::make_shared<MemoryDataStorageManager>();

	return true;
}


void StreamBuffer::Destroy()
{
	BlockLock Lock(m_Lock);

	m_Queue.clear();
	m_BlockSize = 0;
	m_MinBlockCount = 0;
	m_MaxBlockCount = 0;
	m_SerialPos = 0;
	m_DataStorageManager.reset();
}


bool StreamBuffer::IsCreated() const noexcept
{
	return m_BlockSize > 0;
}


void StreamBuffer::Clear()
{
	BlockLock Lock(m_Lock);

	m_Queue.clear();
}


bool StreamBuffer::SetSize(size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount, bool Discard)
{
	LIBISDB_TRACE(
		LIBISDB_STR("StreamBuffer::SetSize() : %zu bytes (%zu - %zu blocks)\n"),
		BlockSize, MinBlockCount, MaxBlockCount);

	if (!CheckBufferSize(BlockSize, MinBlockCount, MaxBlockCount))
		return false;

	BlockLock Lock(m_Lock);

	if (m_BlockSize != BlockSize) {
		m_BlockSize = BlockSize;
		m_MinBlockCount = MinBlockCount;
		m_MaxBlockCount = MaxBlockCount;

		if (!m_Queue.empty()) {
			std::deque<QueueBlock> Queue;
			m_Queue.swap(Queue);

			const size_t MaxSize = BlockSize * MaxBlockCount;
			size_t TotalSize = 0;
			auto it = Queue.end();
			do {
				--it;
				const size_t DataSize = it->GetDataSize();
				if (DataSize > MaxSize - TotalSize) {
					++it;
					break;
				}
				TotalSize += DataSize;
			} while (it != Queue.begin());

			DataBuffer Buffer;
			for (; it != Queue.end(); ++it) {
				size_t Size = it->GetDataSize();
				if (Buffer.AllocateBuffer(Size) < Size)
					break;
				Size = it->Read(0, Buffer.GetBuffer(), Size);
				PushBack(Buffer.GetBuffer(), Size);
			}
		}
	} else if (m_MaxBlockCount != MaxBlockCount) {
		m_MaxBlockCount = MaxBlockCount;

		if (Discard) {
			while (m_Queue.size() > MaxBlockCount) {
				m_Queue.pop_front();
			}
		}
	}

	m_MinBlockCount = MinBlockCount;

	return true;
}


bool StreamBuffer::IsEmpty() const
{
	BlockLock Lock(m_Lock);

	return m_Queue.empty();
}


bool StreamBuffer::IsFull() const
{
	BlockLock Lock(m_Lock);

	if (m_MaxBlockCount == 0)
		return true;
	if (m_Queue.size() < m_MaxBlockCount)
		return false;

	return m_Queue.back().IsFull();
}


size_t StreamBuffer::GetFreeSpace() const
{
	BlockLock Lock(m_Lock);

	size_t Free = 0;

	if (m_Queue.size() < m_MaxBlockCount)
		Free += (m_MaxBlockCount - m_Queue.size()) * m_BlockSize;

	if (m_Queue.size() >= 2) {
		size_t Discardable;

		if (m_Queue.size() > m_MinBlockCount)
			Discardable = m_Queue.size() - m_MinBlockCount;
		else
			Discardable = 1;
		if (Discardable >= m_Queue.size())
			Discardable = m_Queue.size() - 1;

		for (size_t i = 0; i < Discardable; i++) {
			if (IsBlockLocked(m_Queue[i]))
				break;
			Free += m_Queue[i].GetCapacity();
		}
	}

	if (!m_Queue.empty()) {
		if (!m_Queue.back().IsFull()) {
			const size_t Capacity = m_Queue.back().GetCapacity();
			const size_t DataSize = m_Queue.back().GetDataSize();
			if (DataSize < Capacity)
				Free += Capacity - DataSize;
		}
	}

	return Free;
}


size_t StreamBuffer::PushBack(const uint8_t *pData, size_t DataSize)
{
	if ((pData == nullptr) || (DataSize == 0))
		return 0;

	if (m_BlockSize == 0)
		return 0;

	BlockLock Lock(m_Lock);

	size_t Pos = 0;

	if (!m_Queue.empty()) {
		QueueBlock &Last = m_Queue.back();
		if (!Last.IsFull()) {
			const size_t CopySize = Last.Write(pData, DataSize);
			m_SerialPos += CopySize;
			if ((CopySize == DataSize) || !Last.IsFull())
				return CopySize;
			Pos = CopySize;
		}
	}

	do {
		QueueBlock Block;

		if (m_Queue.size() < m_MaxBlockCount) {
			//LIBISDB_TRACE_VERBOSE(LIBISDB_STR("Create DataStorage [%zu] %lld\n"), m_Queue.size(), m_SerialPos);
			DataStorage *pStorage = m_DataStorageManager->CreateDataStorage();

			if (pStorage == nullptr)
				break;
			if (!pStorage->Allocate(m_BlockSize)) {
				delete pStorage;
				break;
			}
			Block.SetStorage(pStorage);
		} else {
			if (IsBlockLocked(m_Queue.front()))
				break;
			//LIBISDB_TRACE_VERBOSE(LIBISDB_STR("Reuse DataStorage [%zu] %lld\n"), m_Queue.size() - 1, m_SerialPos);
			Block = std::move(m_Queue.front());
			m_Queue.pop_front();
			Block.Reuse();
		}

		const size_t CopySize = Block.Write(pData + Pos, DataSize - Pos);
		Pos += CopySize;

		m_Queue.push_back(std::move(Block));
		m_Queue.back().SetSerialPos(m_SerialPos);
		m_SerialPos += CopySize;

		if (!Block.IsFull())
			break;
	} while (Pos < DataSize);

	return Pos;
}


size_t StreamBuffer::PushBack(const DataBuffer *pData)
{
	if (pData == nullptr)
		return 0;

	return PushBack(pData->GetData(), pData->GetSize());
}


int StreamBuffer::GetBlockIndexBySerialPos(PosType Pos) const
{
	if (m_Queue.empty())
		return -1;

	const PosType First = m_Queue.front().GetSerialPos();
	if (Pos < First)
		return -1;
	const size_t Index = static_cast<size_t>((Pos - First) / m_BlockSize);
	if (Index >= m_Queue.size())
		return -1;

	return static_cast<int>(Index);
}


StreamBuffer::QueueBlock * StreamBuffer::GetBlockBySerialPos(PosType Pos)
{
	const int Index = GetBlockIndexBySerialPos(Pos);

	if (Index < 0)
		return nullptr;

	return &m_Queue[Index];
}


bool StreamBuffer::SetReaderPos(Reader *pReader, PosType Pos)
{
	BlockLock Lock(m_Lock);

	auto Result = m_ReaderPosList.emplace(pReader, Pos);
	if (!Result.second)
		Result.first->second = Pos;

	FreeUnusedBlocks();

	return true;
}


bool StreamBuffer::ResetReaderPos(Reader *pReader)
{
	BlockLock Lock(m_Lock);

	auto it = m_ReaderPosList.find(pReader);
	if (it == m_ReaderPosList.end())
		return false;

	m_ReaderPosList.erase(it);

	FreeUnusedBlocks();

	return true;
}


StreamBuffer::PosType StreamBuffer::GetBeginPos() const
{
	BlockLock Lock(m_Lock);

	if (m_Queue.empty())
		return m_SerialPos;

	return m_Queue.front().GetSerialPos();
}


StreamBuffer::PosType StreamBuffer::GetEndPos() const
{
	BlockLock Lock(m_Lock);

	if (m_Queue.empty())
		return m_SerialPos;

	return m_Queue.back().GetSerialPos() + m_Queue.back().GetDataSize();
}


bool StreamBuffer::GetDataRange(ReturnArg<PosType> Begin, ReturnArg<PosType> End) const
{
	BlockLock Lock(m_Lock);

	if (m_Queue.empty()) {
		Begin = m_SerialPos;
		End = m_SerialPos;
		return false;
	}

	Begin = m_Queue.front().GetSerialPos();
	End = m_Queue.back().GetSerialPos() + m_Queue.back().GetDataSize();

	return true;
}


bool StreamBuffer::IsBlockLocked(const QueueBlock &Block) const
{
	for (auto &e : m_ReaderPosList) {
		if ((e.second >= 0) && (e.second < Block.GetSerialPos() + static_cast<long long>(Block.GetCapacity())))
			return true;
	}

	return false;
}


void StreamBuffer::FreeUnusedBlocks()
{
	if ((m_MinBlockCount < m_MaxBlockCount)
			&& (m_Queue.size() > m_MinBlockCount)) {
		do {
			if (IsBlockLocked(m_Queue.front()))
				break;
			m_Queue.pop_front();
		} while (m_Queue.size() > m_MinBlockCount);
	}
}


size_t StreamBuffer::Read(PosType *pPos, void *pBuffer, size_t Size)
{
	BlockLock Lock(m_Lock);

	PosType Pos = *pPos;
	auto it = m_Queue.begin();
	size_t Offset;
	const int Index = GetBlockIndexBySerialPos(Pos);
	if (Index < 0) {
		if (m_Queue.empty() || (Pos > m_Queue.front().GetSerialPos()))
			return 0;
		Pos = m_Queue.front().GetSerialPos();
		Offset = 0;
	} else {
		it += Index;
		Offset = static_cast<size_t>(Pos - it->GetSerialPos());
		if (it->GetDataSize() <= Offset) {
			++it;
			Offset = 0;
		}
	}

	size_t ReadSize = 0;
	uint8_t *pDst = static_cast<uint8_t *>(pBuffer);

	for (; (it != m_Queue.end()) && (ReadSize < Size); ++it) {
		const size_t CopySize = it->Read(Offset, pDst + ReadSize, Size - ReadSize);
		/*
		LIBISDB_TRACE_VERBOSE(
			LIBISDB_STR("Block read [%zu] %lld+%zu: %zu / %zu (%zu / %zu)\n"),
			it - m_Queue.begin(), it->GetSerialPos(), Offset, CopySize, Size - ReadSize, it->GetDataSize(), it->GetCapacity());
		*/
		ReadSize += CopySize;
		Pos = it->GetSerialPos() + Offset + CopySize;
		Offset = 0;
	}

	*pPos = Pos;

	return ReadSize;
}


bool StreamBuffer::CheckBufferSize(size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount)
{
	if ((BlockSize == 0) || (MaxBlockCount == 0))
		return false;
	if (BlockSize > std::numeric_limits<size_t>::max() / MaxBlockCount)
		return false;
	if (MinBlockCount > MaxBlockCount)
		return false;
	return true;
}




StreamBuffer::QueueBlock::QueueBlock()
	: m_SerialPos(POS_INVALID)
{
}


StreamBuffer::QueueBlock::QueueBlock(QueueBlock &&Src)
	: QueueBlock()
{
	*this = std::move(Src);
}


StreamBuffer::QueueBlock::~QueueBlock()
{
	Free();
}


StreamBuffer::QueueBlock & StreamBuffer::QueueBlock::operator = (QueueBlock &&Src)
{
	if (&Src != this) {
		Free();

		m_Storage = std::move(Src.m_Storage);
		m_SerialPos = Src.m_SerialPos;
	}

	return *this;
}


bool StreamBuffer::QueueBlock::SetStorage(DataStorage *pStorage)
{
	if (pStorage == nullptr)
		return false;

	m_Storage.reset(pStorage);

	return true;
}


void StreamBuffer::QueueBlock::Free()
{
	if (m_Storage)
		m_Storage->Free();
}


void StreamBuffer::QueueBlock::Reuse()
{
	if (m_Storage)
		m_Storage->SetPos(0);
	m_SerialPos = POS_INVALID;
}


size_t StreamBuffer::QueueBlock::Write(const void *pData, size_t DataSize)
{
	if (!m_Storage)
		return 0;

	const DataStorage::SizeType Capacity = m_Storage->GetCapacity();
	const DataStorage::SizeType Pos = m_Storage->GetPos();
	if (Pos >= Capacity)
		return 0;

	size_t WriteSize = static_cast<size_t>(
		std::min(Capacity - Pos, static_cast<DataStorage::SizeType>(DataSize)));
	if (WriteSize > 0)
		WriteSize = m_Storage->Write(pData, WriteSize);

	return WriteSize;
}


size_t StreamBuffer::QueueBlock::Read(size_t Offset, void *pData, size_t DataSize)
{
	if (!m_Storage)
		return 0;

	const DataStorage::SizeType Pos = m_Storage->GetPos();

	if (Offset >= Pos)
		return 0;
	if (!m_Storage->SetPos(Offset))
		return 0;

	size_t ReadSize = static_cast<size_t>(
		std::min(Pos - Offset, static_cast<DataStorage::SizeType>(DataSize)));
	if (ReadSize > 0)
		ReadSize = m_Storage->Read(pData, ReadSize);

	m_Storage->SetPos(Pos);

	return ReadSize;
}


size_t StreamBuffer::QueueBlock::GetCapacity() const
{
	if (!m_Storage)
		return 0;
	return static_cast<size_t>(m_Storage->GetCapacity());
}


size_t StreamBuffer::QueueBlock::GetDataSize() const
{
	if (!m_Storage)
		return 0;
	return static_cast<size_t>(m_Storage->GetPos());
}


bool StreamBuffer::QueueBlock::IsFull() const
{
	if (!m_Storage)
		return true;
	return m_Storage->IsEnd();
}




bool StreamBuffer::Reader::Open(const std::shared_ptr<StreamBuffer> &Buffer)
{
	if (!Buffer || m_Buffer)
		return false;

	m_Buffer = Buffer;

	return true;
}


void StreamBuffer::Reader::Close()
{
	m_Buffer.reset();
}




StreamBuffer::SequentialReader::SequentialReader()
	: m_Pos(StreamBuffer::POS_INVALID)
{
}


StreamBuffer::SequentialReader::~SequentialReader()
{
	Close();
}


bool StreamBuffer::SequentialReader::Open(const std::shared_ptr<StreamBuffer> &Buffer)
{
	if (!Reader::Open(Buffer))
		return false;

	m_Pos = m_Buffer->GetBeginPos();
	m_Buffer->SetReaderPos(this, m_Pos);

	return true;
}


void StreamBuffer::SequentialReader::Close()
{
	ResetPos();

	Reader::Close();
}


size_t StreamBuffer::SequentialReader::Read(void *pBuffer, size_t Size)
{
	if ((pBuffer == nullptr) || (Size == 0) || !m_Buffer)
		return 0;

	const PosType OldPos = m_Pos;
	const size_t ReadSize = m_Buffer->Read(&m_Pos, pBuffer, Size);
	if (m_Pos != OldPos)
		m_Buffer->SetReaderPos(this, m_Pos);

	return ReadSize;
}


bool StreamBuffer::SequentialReader::SetPos(PosType Pos)
{
	if (!m_Buffer || (Pos < 0))
		return false;

	if (Pos != m_Pos) {
		m_Pos = Pos;
		m_Buffer->SetReaderPos(this, m_Pos);
	}

	return true;
}


bool StreamBuffer::SequentialReader::SeekToBegin()
{
	if (!m_Buffer)
		return false;

	const PosType Pos = m_Buffer->GetBeginPos();
	if (Pos != m_Pos) {
		m_Pos = Pos;
		m_Buffer->SetReaderPos(this, m_Pos);
	}

	return true;
}


bool StreamBuffer::SequentialReader::SeekToEnd()
{
	if (!m_Buffer)
		return false;

	const PosType Pos = m_Buffer->GetEndPos();
	if (Pos != m_Pos) {
		m_Pos = Pos;
		m_Buffer->SetReaderPos(this, m_Pos);
	}

	return true;
}


bool StreamBuffer::SequentialReader::IsDataAvailable() const
{
	if (m_Pos == StreamBuffer::POS_INVALID)
		return false;

	PosType Begin, End;

	if (!m_Buffer->GetDataRange(&Begin, &End))
		return false;

	if (m_Pos == StreamBuffer::POS_BEGIN)
		return End > Begin;

	return End > m_Pos;
}


void StreamBuffer::SequentialReader::ResetPos()
{
	if (m_Buffer)
		m_Buffer->ResetReaderPos(this);
	m_Pos = StreamBuffer::POS_INVALID;
}


}	// namespace LibISDB
