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
 @file   FileStreamGenericC.hpp
 @brief  ファイルストリームのC標準規格実装
 @author DBCTRADO
*/


#ifndef LIBISDB_FILE_STREAM_GENERIC_C_H
#define LIBISDB_FILE_STREAM_GENERIC_C_H


#include "Stream.hpp"
#include <functional>
#include <memory>


namespace LibISDB
{

	/** ファイルストリームのC標準規格実装クラス */
	class FileStreamGenericC
		: public FileStreamBase
	{
	public:
		typedef std::function<void(std::FILE *)> Closer;
		struct DefaultCloser {
			void operator () (std::FILE *p) const { std::fclose(p); }
		};
		struct NopCloser {
			void operator () (std::FILE *p) const {}
		};

		FileStreamGenericC();
		FileStreamGenericC(const Closer &closer);
		~FileStreamGenericC();

		bool Open(const CStringView &FileName, OpenFlag Flags) override;
		bool Close() override;
		bool IsOpen() const override;

		size_t Read(void *pBuff, size_t Size) override;
		size_t Write(const void *pBuff, size_t Size) override;
		bool Flush() override;

		SizeType GetSize() override;
		OffsetType GetPos() override;
		bool SetPos(OffsetType Pos, SetPosType Type) override;

		bool IsEnd() const override;

	protected:
		std::unique_ptr<std::FILE, Closer> m_File;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_FILE_STREAM_GENERIC_C_H
