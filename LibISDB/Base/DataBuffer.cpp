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
 @file   DataBuffer.cpp
 @brief  メモリデータバッファ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "DataBuffer.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


DataBuffer::DataBuffer(const DataBuffer &Src)
{
	*this = Src;
}


DataBuffer::DataBuffer(DataBuffer &&Src) noexcept
{
	*this = std::move(Src);
}


DataBuffer::DataBuffer(size_t BufferSize)
{
	AllocateBuffer(BufferSize);
}


DataBuffer::DataBuffer(const void *pData, size_t DataSize)
{
	SetData(pData, DataSize);
}


DataBuffer::DataBuffer(size_t DataSize, uint8_t Filler)
{
	SetSize(DataSize, Filler);
}


DataBuffer::~DataBuffer()
{
	if (m_pData != nullptr)
		Free(m_pData);
}


DataBuffer & DataBuffer::operator = (const DataBuffer &Src)
{
	if (&Src != this) {
		SetData(Src.m_pData, Src.m_DataSize);
	}

	return *this;
}


DataBuffer & DataBuffer::operator = (DataBuffer &&Src) noexcept
{
	if (&Src != this) {
		FreeBuffer();
		m_DataSize = Src.m_DataSize;
		m_BufferSize = Src.m_BufferSize;
		m_pData = Src.m_pData;
		Src.m_DataSize = 0;
		Src.m_BufferSize = 0;
		Src.m_pData = nullptr;
	}

	return *this;
}


bool DataBuffer::operator == (const DataBuffer &rhs) const noexcept
{
	if (m_DataSize != rhs.m_DataSize)
		return false;
	return (m_DataSize == 0)
		|| (std::memcmp(m_pData, rhs.m_pData, m_DataSize) == 0);
}


uint8_t * DataBuffer::GetData() noexcept
{
	return (m_DataSize > 0) ? m_pData : nullptr;
}


const uint8_t * DataBuffer::GetData() const noexcept
{
	return (m_DataSize > 0) ? m_pData : nullptr;
}


void DataBuffer::SetAt(size_t Pos, uint8_t Data)
{
	if (LIBISDB_TRACE_ERROR_IF_NOT(Pos < m_DataSize))
		m_pData[Pos] = Data;
}


uint8_t DataBuffer::GetAt(size_t Pos) const
{
	return LIBISDB_TRACE_ERROR_IF_NOT(Pos < m_DataSize) ? m_pData[Pos] : 0x00_u8;
}


size_t DataBuffer::SetData(const void *pData, size_t DataSize)
{
	if (DataSize > 0) {
		if (LIBISDB_TRACE_ERROR_IF(pData == nullptr))
			return 0;

		if (AllocateBuffer(DataSize) < DataSize)
			return m_DataSize;

		std::memcpy(m_pData, pData, DataSize);
	}

	m_DataSize = DataSize;

	return m_DataSize;
}


size_t DataBuffer::AddData(const void *pData, size_t DataSize)
{
	if (DataSize > 0) {
		if (LIBISDB_TRACE_ERROR_IF(pData == nullptr))
			return m_DataSize;
		if (LIBISDB_TRACE_ERROR_IF(DataSize > std::numeric_limits<size_t>::max() - m_DataSize))
			return m_DataSize;

		size_t NewSize = m_DataSize + DataSize;
		if (AllocateBuffer(NewSize) < NewSize)
			return m_DataSize;

		std::memcpy(&m_pData[m_DataSize], pData, DataSize);

		m_DataSize += DataSize;
	}

	return m_DataSize;
}


size_t DataBuffer::AddData(const DataBuffer &Data)
{
	return AddData(Data.m_pData, Data.m_DataSize);
}


size_t DataBuffer::AddByte(uint8_t Data)
{
	// m_DataSize + 1 でオーバーフローすると <= m_DataSize の条件に引っ掛かるため
	// オーバーフローのチェックは不要
	if (AllocateBuffer(m_DataSize + 1) <= m_DataSize)
		return m_DataSize;

	m_pData[m_DataSize] = Data;

	m_DataSize++;

	return m_DataSize;
}


size_t DataBuffer::TrimHead(size_t TrimSize)
{
	if (TrimSize >= m_DataSize) {
		m_DataSize = 0;
	} else if (m_DataSize > 0) {
		std::memmove(m_pData, m_pData + TrimSize, m_DataSize - TrimSize);
		m_DataSize -= TrimSize;
	}

	return m_DataSize;
}


size_t DataBuffer::TrimTail(size_t TrimSize)
{
	if (TrimSize >= m_DataSize) {
		m_DataSize = 0;
	} else {
		m_DataSize -= TrimSize;
	}

	return m_DataSize;
}


size_t DataBuffer::AllocateBuffer(size_t Size)
{
	if (LIBISDB_TRACE_ERROR_IF(Size > RSIZE_MAX))
		return m_BufferSize;
	if (Size <= m_BufferSize)
		return m_BufferSize;

	if (m_pData == nullptr) {
		m_pData = static_cast<uint8_t *>(Allocate(Size));
		if (m_pData != nullptr)
			m_BufferSize = Size;
	} else if (Size > m_BufferSize) {
		const size_t AllocateUnit = 0x100000_z;
		size_t BufferSize = Size;

		if (BufferSize < AllocateUnit) {
			if (BufferSize < m_DataSize * 2)
				BufferSize = m_DataSize * 2;
		} else if (BufferSize <= std::numeric_limits<size_t>::max() - AllocateUnit) {
			BufferSize = (BufferSize + (AllocateUnit - 1)) & ~(AllocateUnit - 1);
		}

		uint8_t *pNewBuffer = static_cast<uint8_t *>(ReAllocate(m_pData, BufferSize));

		if (pNewBuffer != nullptr) {
			m_BufferSize = BufferSize;
			m_pData = pNewBuffer;
		}
	}

	return m_BufferSize;
}


size_t DataBuffer::SetSize(size_t Size)
{
	if (Size > 0) {
		if (AllocateBuffer(Size) < Size)
			return m_DataSize;
	}

	m_DataSize = Size;

	return m_DataSize;
}


size_t DataBuffer::SetSize(size_t Size, uint8_t Filler)
{
	if (SetSize(Size) < Size)
		return m_DataSize;

	if (Size > 0)
		std::memset(m_pData, Filler, Size);

	return m_DataSize;
}


void DataBuffer::ClearSize() noexcept
{
	m_DataSize = 0;
}


void DataBuffer::FreeBuffer() noexcept
{
	m_DataSize = 0;
	m_BufferSize = 0;
	if (m_pData != nullptr) {
		Free(m_pData);
		m_pData = nullptr;
	}
}


void * DataBuffer::Allocate(size_t Size)
{
	return std::malloc(Size);
}


void DataBuffer::Free(void *pBuffer) noexcept
{
	std::free(pBuffer);
}


void * DataBuffer::ReAllocate(void *pBuffer, size_t Size)
{
	return std::realloc(pBuffer, Size);
}


}	// namespace LibISDB
