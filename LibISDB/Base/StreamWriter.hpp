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
 @file   StreamWriter.hpp
 @brief  ストリーム書き出し
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_WRITER_H
#define LIBISDB_STREAM_WRITER_H


#include "ErrorHandler.hpp"
#include "FileStream.hpp"


namespace LibISDB
{

	/** ストリーム書き出し基底クラス */
	class StreamWriter
		: public ErrorHandler
	{
	public:
		typedef Stream::SizeType SizeType;

		/** オープンフラグ */
		enum class OpenFlag {
			None      = 0x0000U, /**< 指定なし */
			Overwrite = 0x0001U, /**< 上書き */
			LIBISDB_ENUM_FLAGS_TRAILER
		};

		virtual ~StreamWriter() = default;

		virtual bool Open(const String &FileName, OpenFlag Flags = OpenFlag::None) = 0;
		virtual bool Reopen(const String &FileName, OpenFlag Flags = OpenFlag::None) = 0;
		virtual void Close() = 0;
		virtual bool IsOpen() const = 0;
		virtual size_t Write(const void *pBuffer, size_t Size) = 0;
		virtual bool GetFileName(String *pFileName) const = 0;
		virtual SizeType GetWriteSize() const = 0;
		virtual bool IsWriteSizeAvailable() const = 0;
		virtual bool SetPreallocationUnit(SizeType PreallocationUnit) { return false; }
	};

	/** ファイルストリーム書き出しクラス */
	class FileStreamWriter
		: public StreamWriter
	{
	public:
		FileStreamWriter() noexcept;
		~FileStreamWriter();

	// StreamWriter
		bool Open(const String &FileName, OpenFlag Flags = OpenFlag::None) override;
		bool Reopen(const String &FileName, OpenFlag Flags = OpenFlag::None) override;
		void Close() override;
		bool IsOpen() const override;
		size_t Write(const void *pBuffer, size_t Size) override;
		bool GetFileName(String *pFileName) const override;
		SizeType GetWriteSize() const override;
		bool IsWriteSizeAvailable() const override;
		bool SetPreallocationUnit(SizeType PreallocationUnit) override;

	private:
		FileStream * OpenFile(const String &FileName, OpenFlag Flags);

		std::unique_ptr<FileStream> m_File;
		SizeType m_WriteSize;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_WRITER_H
