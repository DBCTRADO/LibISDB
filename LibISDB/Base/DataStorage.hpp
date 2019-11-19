/*
  LibISDB
  Copyright(c) 2017-2019 DBCTRADO

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
 @file   DataStorage.hpp
 @brief  データの貯蔵
 @author DBCTRADO
*/


#ifndef LIBISDB_DATA_STORAGE_H
#define LIBISDB_DATA_STORAGE_H


#include "DataBuffer.hpp"
#include "FileStream.hpp"
#include <algorithm>


namespace LibISDB
{

	/** データストレージ基底クラス */
	class DataStorage
	{
	public:
		typedef Stream::SizeType SizeType;

		virtual ~DataStorage() = default;

		virtual bool Allocate(SizeType Size) = 0;
		virtual bool IsAllocated() const;
		virtual void Free() noexcept = 0;
		virtual SizeType GetCapacity() const = 0;
		virtual SizeType GetDataSize() const = 0;
		virtual bool IsFull() const;
		virtual bool IsEnd() const;
		virtual size_t Read(void *pData, size_t Size) = 0;
		virtual size_t Write(const void *pData, size_t Size) = 0;
		virtual bool SetPos(SizeType Pos) = 0;
		virtual SizeType GetPos() const = 0;
	};

	/** メモリデータストレージクラス */
	class MemoryDataStorage : public DataStorage
	{
	public:
	// DataStorage
		bool Allocate(SizeType Size) override;
		void Free() noexcept override;
		SizeType GetCapacity() const override;
		SizeType GetDataSize() const override;
		size_t Read(void *pData, size_t Size) override;
		size_t Write(const void *pData, size_t Size) override;
		bool SetPos(SizeType Pos) override;
		SizeType GetPos() const override;

	protected:
		DataBuffer m_Buffer;
		size_t m_Pos = 0;
	};

	/** ストリームデータストレージクラス */
	class StreamDataStorage
		: public DataStorage
	{
	public:
	// DataStorage
		bool Allocate(SizeType Size) override;
		void Free() noexcept override;
		SizeType GetCapacity() const override;
		SizeType GetDataSize() const override;
		size_t Read(void *pData, size_t Size) override;
		size_t Write(const void *pData, size_t Size) override;
		bool SetPos(SizeType Pos) override;
		SizeType GetPos() const override;

	protected:
		std::unique_ptr<Stream> m_Stream;
		SizeType m_Capacity = 0;
	};

	/** ファイルデータストレージクラス */
	class FileDataStorage
		: public StreamDataStorage
	{
	public:
	// DataStorage
		bool Allocate(SizeType Size) override;
		void Free() noexcept override;

	// FileDataStorage
		bool SetFileName(const StringView &FileName);
		const String & GetFileName() const noexcept { return m_FileName; }
		void SetOpenFlags(FileStream::OpenFlag Flags) noexcept { m_OpenFlags = Flags; }
		FileStream::OpenFlag GetOpenFlags() const noexcept { return m_OpenFlags; }
		void SetPreallocate(bool Preallocate) noexcept { m_Preallocate = Preallocate; }
		bool GetPreallocate() const noexcept { return m_Preallocate; }

	protected:
		String m_FileName;
		FileStream::OpenFlag m_OpenFlags =
			FileStream::OpenFlag::Read |
			FileStream::OpenFlag::Write |
			FileStream::OpenFlag::Create |
			FileStream::OpenFlag::Truncate;
		bool m_Preallocate = true;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DATA_STORAGE_H
