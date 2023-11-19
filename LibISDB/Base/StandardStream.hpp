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
 @file   StandardStream.hpp
 @brief  標準ストリーム
 @author DBCTRADO
*/


#ifndef LIBISDB_STANDARD_STREAM_H
#define LIBISDB_STANDARD_STREAM_H


#include "FileStreamGenericC.hpp"


namespace LibISDB
{

	/** 標準ストリーム基底クラス */
	class StandardStream
		: public FileStreamGenericC
	{
	public:
		StandardStream(FILE *pFile)
			: FileStreamGenericC(NopCloser())
		{
			m_File.reset(pFile);
		}
	};

	/** 標準入力ストリームクラス */
	class StandardInputStream
		: public StandardStream
	{
	public:
		static const CharType Name[];

		StandardInputStream();
	};

	/** 標準出力ストリームクラス */
	class StandardOutputStream
		: public StandardStream
	{
	public:
		static const CharType Name[];

		StandardOutputStream();
	};


	FileStreamBase * OpenFileStream(const String &Name, FileStreamBase::OpenFlag OpenFlags);

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STANDARD_STREAM_H
