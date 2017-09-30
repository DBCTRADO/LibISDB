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
 @file   DataStreamer.cpp
 @brief  データストリーマ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "DataStreamer.hpp"
#include "../Utilities/Clock.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


DataStreamer::DataStreamer()
	: m_InputStartPos(StreamBuffer::POS_BEGIN)
	, m_OutputErrorNotified(false)
{
}


DataStreamer::~DataStreamer()
{
	Close();
}


bool DataStreamer::Initialize()
{
	Close();

	m_Statistics.Reset();
	m_OutputErrorNotified = false;

	return true;
}


void DataStreamer::Close()
{
	Stop();

	m_StreamReader.Close();
	m_InputBuffer.reset();
}


bool DataStreamer::Start()
{
	if (IsStarted())
		return false;

	if (m_InputBuffer) {
		m_StreamReader.Open(m_InputBuffer);

		if (m_InputStartPos >= 0) {
			m_StreamReader.SetPos(m_InputStartPos);
			m_InputStartPos = StreamBuffer::POS_BEGIN;
		}

		if (!StartStreamingThread()) {
			m_StreamReader.Close();
			return false;
		}
	}

	return true;
}


bool DataStreamer::Stop(const std::chrono::milliseconds &Timeout)
{
	if (IsStarted()) {
		m_StreamingThreadTimeout = Timeout;
		StopStreamingThread();
	}

	return true;
}


bool DataStreamer::Pause()
{
	if (!IsStarted())
		return false;

	BlockLock Lock(m_Lock);

	m_StreamReader.Close();

	return true;
}


bool DataStreamer::Resume()
{
	if (!IsStarted())
		return false;

	BlockLock Lock(m_Lock);

	if (m_InputBuffer) {
		m_StreamReader.Open(m_InputBuffer);
		m_StreamReader.SeekToEnd();
	}

	return true;
}


bool DataStreamer::InputData(const uint8_t *pData, size_t DataSize)
{
	BlockLock Lock(m_Lock);
	bool Result;

	if (m_InputBuffer) {
		Result = m_InputBuffer->PushBack(pData, DataSize);
	} else if (m_OutputCacheBuffer.GetBufferSize() > 0) {
		Result = OutputDataWithCache(pData, DataSize);
	} else if (IsOutputValid()) {
		Result = OutputData(pData, DataSize) == DataSize;
	} else {
		return false;
	}

	m_Statistics.InputBytes += DataSize;

	return Result;
}


bool DataStreamer::InputData(const DataBuffer *pData)
{
	if (pData == nullptr)
		return false;

	return InputData(pData->GetData(), pData->GetSize());
}


void DataStreamer::ClearBuffer()
{
	BlockLock Lock(m_Lock);

	if (m_InputBuffer)
		m_InputBuffer->Clear();

	ClearOutput();
}


bool DataStreamer::FlushBuffer(const std::chrono::milliseconds &Timeout)
{
	LIBISDB_TRACE(LIBISDB_STR("DataStreamer::FlushBuffer()\n"));

	if (IsStarted())
		return false;

	BlockLock Lock(m_Lock);
	TickClock Clock;
	const TickClock::ClockType StartTime = Clock.Get();

	while (m_StreamReader.IsDataAvailable()) {
		if ((Timeout.count() > 0) && (TickClock::DurationType(Clock.Get() - StartTime) >= Timeout)) {
			Log(Logger::LogType::Warning, LIBISDB_STR("書き出し待ちデータを全て書き出すのに時間が掛かり過ぎているため中止します。"));
			return false;
		}

		if (!FillOutputCache())
			break;
		if (!OutputCachedData())
			return false;
	}

	if (!OutputCachedData())
		return false;

	return true;
}


bool DataStreamer::CreateInputBuffer(size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount)
{
	if ((BlockSize == 0) || (MaxBlockCount == 0) || (MinBlockCount > MaxBlockCount))
		return false;

	std::shared_ptr<StreamBuffer> Buffer = std::make_shared<StreamBuffer>();

	if (!Buffer->Create(BlockSize, MinBlockCount, MaxBlockCount))
		return false;

	return SetInputBuffer(Buffer);
}


bool DataStreamer::FreeInputBuffer()
{
	BlockLock Lock(m_Lock);

	if (!m_InputBuffer)
		return false;

	m_StreamReader.Close();
	m_InputBuffer.reset();

	return true;
}


bool DataStreamer::SetInputBuffer(const std::shared_ptr<StreamBuffer> &Buffer)
{
	if (!Buffer)
		return false;

	BlockLock Lock(m_Lock);

	if (m_InputBuffer != Buffer) {
		const bool IsReaderOpen = m_StreamReader.IsOpen();
		if (IsReaderOpen)
			m_StreamReader.Close();

		m_InputBuffer = Buffer;

		if (IsReaderOpen)
			m_StreamReader.Open(m_InputBuffer);
	}

	return true;
}


std::shared_ptr<StreamBuffer> DataStreamer::GetInputBuffer() const
{
	return m_InputBuffer;
}


std::shared_ptr<StreamBuffer> DataStreamer::DetachInputBuffer()
{
	BlockLock Lock(m_Lock);

	m_StreamReader.Close();

	return std::move(m_InputBuffer);
}


bool DataStreamer::HasInputBuffer() const noexcept
{
	return static_cast<bool>(m_InputBuffer);
}


bool DataStreamer::SetInputStartPos(StreamBuffer::PosType Pos)
{
	m_InputStartPos = Pos;

	return true;
}


bool DataStreamer::AllocateOutputCacheBuffer(size_t Size)
{
	BlockLock Lock(m_Lock);

	return m_OutputCacheBuffer.AllocateBuffer(Size) >= Size;
}


bool DataStreamer::GetStatistics(Statistics *pStats) const
{
	if (pStats == nullptr)
		return false;

	BlockLock Lock(m_Lock);

	*pStats = m_Statistics;

	return true;
}


bool DataStreamer::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool DataStreamer::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


bool DataStreamer::FillOutputCache()
{
	const size_t BufferSize = m_OutputCacheBuffer.GetBufferSize();
	size_t BufferUsed = m_OutputCacheBuffer.GetSize();

	if (BufferUsed < BufferSize) {
		const size_t ReadSize = m_StreamReader.Read(
			m_OutputCacheBuffer.GetBuffer() + BufferUsed,
			BufferSize - BufferUsed);
		if (ReadSize == 0)
			return false;
		BufferUsed += ReadSize;
		m_OutputCacheBuffer.SetSize(BufferUsed);
		if (BufferUsed < BufferSize)
			return false;
	}

	return true;
}


bool DataStreamer::OutputCachedData()
{
	size_t BufferUsed = m_OutputCacheBuffer.GetSize();
	if (BufferUsed == 0)
		return true;

	uint8_t *pData = m_OutputCacheBuffer.GetBuffer();
	const size_t Written = OutputData(pData, BufferUsed);

	if (Written > 0) {
		m_Statistics.OutputBytes += Written;
		m_Statistics.OutputCount++;
	}

	if (Written < BufferUsed) {
		m_Statistics.OutputErrorCount++;
		if (Written > 0) {
			BufferUsed -= Written;
			std::memmove(pData, pData + Written, BufferUsed);
		}
		m_OutputCacheBuffer.SetSize(BufferUsed);
		return false;
	}

	m_OutputCacheBuffer.SetSize(0);

	return true;
}


bool DataStreamer::OutputDataWithCache(const uint8_t *pData, size_t DataSize)
{
	const size_t BufferSize = m_OutputCacheBuffer.GetBufferSize();
	size_t BufferUsed = m_OutputCacheBuffer.GetSize();
	size_t Remain = DataSize;

	while (Remain > 0) {
		if (BufferUsed < BufferSize) {
			const size_t CopySize = std::min(BufferSize - BufferUsed, Remain);
			std::memcpy(m_OutputCacheBuffer.GetBuffer(), pData + (DataSize - Remain), CopySize);
			BufferUsed += CopySize;
			m_OutputCacheBuffer.SetSize(BufferUsed);
			if (BufferUsed < BufferSize)
				break;
			Remain -= CopySize;
		}

		if (!OutputCachedData())
			return false;
	}

	return true;
}


bool DataStreamer::ProcessStream()
{
	bool IsFilled = false, Result = false;

	m_Lock.Lock();

	if (m_StreamReader.IsDataAvailable())
		IsFilled = FillOutputCache();

	m_Lock.Unlock();

	if (IsFilled) {
		if (OutputCachedData()) {
			Result = true;
		} else {
			if ((m_Statistics.OutputErrorCount > 0) && !m_OutputErrorNotified) {
				m_OutputErrorNotified = true;
				m_EventListenerList.CallEventListener(&DataStreamer::EventListener::OnOutputError, this);
			}
		}
	}

	return Result;
}


}	// namespace LibISDB
