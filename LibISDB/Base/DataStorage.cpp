/*
  LibISDB
  Copyright(c) 2017-2020 DBCTRADO

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
 @file   DataStorage.cpp
 @brief  データの貯蔵
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "DataStorage.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


bool DataStorage::IsAllocated() const
{
	return GetCapacity() > 0;
}


bool DataStorage::IsFull() const
{
	return GetCapacity() <= GetDataSize();
}


bool DataStorage::IsEnd() const
{
	return GetCapacity() <= GetPos();
}




bool MemoryDataStorage::Allocate(SizeType Size)
{
	if (LIBISDB_TRACE_ERROR_IF(Size > RSIZE_MAX))
		return false;

	return m_Buffer.AllocateBuffer(static_cast<size_t>(Size)) >= static_cast<size_t>(Size);
}


void MemoryDataStorage::Free() noexcept
{
	m_Buffer.FreeBuffer();
}


DataStorage::SizeType MemoryDataStorage::GetCapacity() const
{
	return m_Buffer.GetBufferSize();
}


DataStorage::SizeType MemoryDataStorage::GetDataSize() const
{
	return m_Buffer.GetSize();
}


size_t MemoryDataStorage::Read(void *pData, size_t Size)
{
	if (m_Pos >= m_Buffer.GetSize())
		return 0;

	const size_t CopySize = std::min(Size, m_Buffer.GetSize() - m_Pos);
	std::memcpy(pData, m_Buffer.GetData() + m_Pos, CopySize);
	m_Pos += CopySize;

	return CopySize;
}


size_t MemoryDataStorage::Write(const void *pData, size_t Size)
{
	if (m_Pos >= m_Buffer.GetBufferSize())
		return 0;

	const size_t CopySize = std::min(Size, m_Buffer.GetBufferSize() - m_Pos);
	std::memcpy(m_Buffer.GetBuffer() + m_Pos, pData, CopySize);
	m_Pos += CopySize;
	if (m_Buffer.GetSize() < m_Pos)
		m_Buffer.SetSize(m_Pos);

	return CopySize;
}


bool MemoryDataStorage::SetPos(SizeType Pos)
{
	if (Pos > m_Buffer.GetBufferSize())
		return false;

	m_Pos = static_cast<size_t>(Pos);

	return true;
}


DataStorage::SizeType MemoryDataStorage::GetPos() const
{
	return m_Pos;
}




bool StreamDataStorage::Allocate(SizeType Size)
{
	if (!m_Stream)
		return false;

	m_Capacity = Size;

	return true;
}


void StreamDataStorage::Free() noexcept
{
	m_Stream.reset();
	m_Capacity = 0;
}


DataStorage::SizeType StreamDataStorage::GetCapacity() const
{
	return m_Capacity;
}


DataStorage::SizeType StreamDataStorage::GetDataSize() const
{
	if (!m_Stream)
		return 0;

	return m_Stream->GetSize();
}


size_t StreamDataStorage::Read(void *pData, size_t Size)
{
	if (!m_Stream)
		return 0;

	return m_Stream->Read(pData, Size);
}


size_t StreamDataStorage::Write(const void *pData, size_t Size)
{
	if (!m_Stream)
		return 0;

	const SizeType Pos = m_Stream->GetPos();
	if (Pos >= m_Capacity)
		return 0;

	return m_Stream->Write(pData, static_cast<size_t>(std::min(static_cast<SizeType>(Size), m_Capacity - Pos)));
}


bool StreamDataStorage::SetPos(SizeType Pos)
{
	if (!m_Stream)
		return false;

	return m_Stream->SetPos(Pos);
}


DataStorage::SizeType StreamDataStorage::GetPos() const
{
	if (!m_Stream)
		return 0;

	return m_Stream->GetPos();
}




bool FileDataStorage::Allocate(SizeType Size)
{
	if (LIBISDB_TRACE_ERROR_IF(m_FileName.empty()))
		return false;

	Free();

	FileStream *pStream = new FileStream;

	if (!pStream->Open(m_FileName.c_str(), m_OpenFlags)) {
		delete pStream;
		return false;
	}

	if (m_Preallocate)
		pStream->Preallocate(Size);

	m_Stream.reset(pStream);
	m_Capacity = Size;

	return true;
}


void FileDataStorage::Free() noexcept
{
	if (m_Stream) {
		m_Stream->Close();

#ifdef LIBISDB_WINDOWS
		::DeleteFile(m_FileName.c_str());
#else
		std::remove(m_FileName.c_str());
#endif
	}

	StreamDataStorage::Free();
}


bool FileDataStorage::SetFileName(const StringView &FileName)
{
	if (m_Stream)
		return false;

	if (FileName.empty()) {
		m_FileName.clear();
		return false;
	}

	m_FileName = FileName;

	return true;
}


}	// namespace LibISDB
