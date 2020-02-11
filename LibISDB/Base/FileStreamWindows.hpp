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
 @file   FileStreamWindows.hpp
 @brief  ファイルストリームの Windows 実装
 @author DBCTRADO
*/


#ifndef LIBISDB_FILE_STREAM_WINDOWS_H
#define LIBISDB_FILE_STREAM_WINDOWS_H


#ifdef LIBISDB_WINDOWS


#include "../LibISDBWindows.hpp"
#include "Stream.hpp"


namespace LibISDB
{

	/** ファイルストリームの Windows 実装クラス */
	class FileStreamWindows
		: public FileStreamBase
	{
	public:
		FileStreamWindows();
		~FileStreamWindows();

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

		bool Preallocate(SizeType Size) override;
		bool SetPreallocationUnit(SizeType Unit) override;
		SizeType GetPreallocationUnit() const override;
		SizeType GetPreallocatedSpace() override;

		bool GetTime(FILETIME *pCreationTime, FILETIME *pLastAccessTime, FILETIME *pLastWriteTime) const;

	protected:
		HANDLE m_hFile;

		SizeType m_PreallocationUnit;
		SizeType m_PreallocatedSize;
		bool m_IsPreallocationFailed;
	};

}	// namespace LibISDB


#endif	// LIBISDB_WINDOWS


#endif	// ifndef LIBISDB_FILE_STREAM_WINDOWS_H
