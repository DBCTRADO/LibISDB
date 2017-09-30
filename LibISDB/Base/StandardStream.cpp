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
 @file   StandardStream.cpp
 @brief  標準ストリーム
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StandardStream.hpp"
#include "FileStream.hpp"
#include "../Utilities/StringUtilities.hpp"

#ifdef LIBISDB_WINDOWS
#include <fcntl.h>
#include <io.h>
#endif

#include "DebugDef.hpp"


namespace LibISDB
{


const CharType StandardInputStream::Name[] = LIBISDB_STR("///stdin\\\\\\");


StandardInputStream::StandardInputStream()
	: StandardStream(stdin)
{
#ifdef LIBISDB_WINDOWS
	::_setmode(::_fileno(stdin), _O_BINARY);
#endif
}




const CharType StandardOutputStream::Name[] = LIBISDB_STR("///stdout\\\\\\");


StandardOutputStream::StandardOutputStream()
	: StandardStream(stdout)
{
#ifdef LIBISDB_WINDOWS
	::_setmode(::_fileno(stdout), _O_BINARY);
#endif
}




FileStreamBase * OpenFileStream(const CStringView &Name, FileStreamBase::OpenFlag OpenFlags)
{
	if (Name.empty())
		return nullptr;

	if (Name.compare(StandardInputStream::Name) == 0) {
		if ((OpenFlags & (FileStreamBase::OpenFlag::Read | FileStreamBase::OpenFlag::Write)) != FileStreamBase::OpenFlag::Read)
			return nullptr;
		return new StandardInputStream;
	}

	if (Name.compare(StandardOutputStream::Name) == 0) {
		if ((OpenFlags & (FileStreamBase::OpenFlag::Read | FileStreamBase::OpenFlag::Write)) != FileStreamBase::OpenFlag::Write)
			return nullptr;
		return new StandardOutputStream;
	}

	FileStream *pStream = new FileStream;

	pStream->Open(Name, OpenFlags);

	return pStream;
}


}	// namespace LibISDB
