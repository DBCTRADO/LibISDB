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
 @file   DataBuffer.hpp
 @brief  メモリデータバッファ
 @author DBCTRADO
*/


#ifndef LIBISDB_DATA_BUFFER_H
#define LIBISDB_DATA_BUFFER_H


namespace LibISDB
{

	/** メモリデータバッファクラス */
	class DataBuffer
	{
	public:
		static constexpr unsigned long TypeID = 0x00000000UL;

		DataBuffer() = default;
		DataBuffer(const DataBuffer &Src);
		DataBuffer(DataBuffer &&Src) noexcept;
		DataBuffer(size_t BufferSize);
		DataBuffer(const void *pData, size_t DataSize);
		DataBuffer(size_t DataSize, uint8_t Filler);

		virtual ~DataBuffer();

		DataBuffer & operator = (const DataBuffer &Src);
		DataBuffer & operator = (DataBuffer &&Src) noexcept;

		bool operator == (const DataBuffer &rhs) const noexcept;

		uint8_t * GetData() noexcept;
		const uint8_t * GetData() const noexcept;
		uint8_t * GetBuffer() noexcept { return m_pData; }
		size_t GetSize() const noexcept { return m_DataSize; }
		size_t GetBufferSize() const noexcept { return m_BufferSize; }

		void SetAt(size_t Pos, uint8_t Data);
		uint8_t GetAt(size_t Pos) const;

		size_t SetData(const void *pData, size_t DataSize);
		size_t AddData(const void *pData, size_t DataSize);
		size_t AddData(const DataBuffer &Data);
		size_t AddByte(uint8_t Data);
		size_t TrimHead(size_t TrimSize = 1);
		size_t TrimTail(size_t TrimSize = 1);

		size_t AllocateBuffer(size_t Size);

		size_t SetSize(size_t Size);
		size_t SetSize(size_t Size, uint8_t Filler);

		void ClearSize() noexcept;
		void FreeBuffer() noexcept;

		// dynamic_cast は遅いのでその代替
		template<typename T> T * Cast()
		{
			return (m_TypeID == T::TypeID) ? static_cast<T *>(this) : nullptr;
		}
		template<typename T> const T * Cast() const
		{
			return (m_TypeID == T::TypeID) ? static_cast<const T *>(this) : nullptr;
		}
		template<typename T> bool Is() const noexcept { return m_TypeID == T::TypeID; }
		unsigned long GetTypeID() const noexcept { return m_TypeID; }

	protected:
		virtual void * Allocate(size_t Size);
		virtual void Free(void *pBuffer) noexcept;
		virtual void * ReAllocate(void *pBuffer, size_t Size);

		size_t m_DataSize = 0;
		size_t m_BufferSize = 0;
		uint8_t *m_pData = nullptr;
		unsigned long m_TypeID = TypeID;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DATA_BUFFER_H
