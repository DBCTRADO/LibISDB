/*
  LibISDB
  Copyright(c) 2017-2018 DBCTRADO

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
 @file   FileStreamGenericC.cpp
 @brief  ファイルストリームのC標準規格実装
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "FileStreamGenericC.hpp"
#include "../Utilities/StringUtilities.hpp"

#ifdef _MSC_VER
#include <io.h>
#elif !defined(LIBISDB_NO_POSIX_FILE)
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "DebugDef.hpp"


namespace LibISDB
{


FileStreamGenericC::FileStreamGenericC()
	: m_File(nullptr, DefaultCloser())
{
}


FileStreamGenericC::FileStreamGenericC(const Closer &closer)
	: m_File(nullptr, closer)
{
}


FileStreamGenericC::~FileStreamGenericC()
{
	Close();
}


bool FileStreamGenericC::Open(const CStringView &FileName, OpenFlag Flags)
{
	if (m_File) {
		SetError(std::errc::operation_in_progress);
		return false;
	}

	if (FileName.empty()
			|| !(Flags & (OpenFlag::Read | OpenFlag::Write))) {
		SetError(std::errc::invalid_argument);
		return false;
	}

	CharType Mode[4];
	if ((Flags & (OpenFlag::Read | OpenFlag::Write)) == (OpenFlag::Read | OpenFlag::Write)) {
		// Read + Write
		if (!!(Flags & OpenFlag::Append))
			StringCopy(Mode, LIBISDB_STR("a+b"));
		else //if (!!(Flags & OpenFlag::Truncate))
			StringCopy(Mode, LIBISDB_STR("w+b"));
	} else if (!!(Flags & OpenFlag::Write)) {
		// Write only
		if (!!(Flags & OpenFlag::Append))
			StringCopy(Mode, LIBISDB_STR("ab"));
		else //if (!!(Flags & OpenFlag::Truncate))
			StringCopy(Mode, LIBISDB_STR("wb"));
	} else {
		// Read only
		if (!!(Flags & OpenFlag::Create))
			StringCopy(Mode, LIBISDB_STR("rb"));
		else
			StringCopy(Mode, LIBISDB_STR("r+b"));
	}

	LIBISDB_TRACE(
		LIBISDB_STR("FileStreamGenericC::Open() : Open file \"%")
			LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("\" \"%")
			LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("\"\n"),
		FileName.c_str(), Mode);

	std::FILE *pFile;

#ifdef LIBISDB_WINDOWS

	::errno_t Err =
#ifdef LIBISDB_WCHAR
		::_wfopen_s(&pFile, FileName.c_str(), Mode);
#else
		::_fopen_s(&pFile, FileName.c_str(), Mode);
#endif
	if (Err != 0) {
		SetError((std::errc)Err);
		return false;
	}

#else

	pFile = std::fopen(FileName.c_str(), Mode);
	if (pFile == nullptr) {
		SetError(static_cast<std::errc>(errno));
		return false;
	}

#endif

	m_File.reset(pFile);
	m_FileName = FileName;

	ResetError();

	return true;
}


bool FileStreamGenericC::Close()
{
	if (m_File) {
		m_File.reset();
	}

	m_FileName.clear();

	return true;
}


bool FileStreamGenericC::IsOpen() const
{
	return static_cast<bool>(m_File);
}


size_t FileStreamGenericC::Read(void *pBuff, size_t Size)
{
	if (!m_File) {
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)) {
		return 0;
	}

	return std::fread(pBuff, 1, Size, m_File.get());
}


size_t FileStreamGenericC::Write(const void *pBuff, size_t Size)
{
	if (!m_File) {
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)) {
		return 0;
	}

	return std::fwrite(pBuff, 1, Size, m_File.get());
}


bool FileStreamGenericC::Flush()
{
	if (!m_File) {
		return false;
	}

	return std::fflush(m_File.get()) == 0;
}


FileStreamGenericC::SizeType FileStreamGenericC::GetSize()
{
	if (!m_File) {
		return 0;
	}

#ifdef _MSC_VER

	// MSVC

	__int64 Size = ::_filelengthi64(::_fileno(m_File.get()));
	if (Size < 0) {
		return 0;
	}
	return Size;

#elif !defined(LIBISDB_NO_POSIX_FILE)

	// POSIX

	struct ::stat Stat;
	if (::fstat(::fileno(m_File.get()), &Stat) != 0) {
		return 0;
	}
	return Stat.st_size;

#else

	// Standard C

	std::FILE *fp = m_File.get();
	const long CurPos = std::ftell(fp);
	std::fseek(fp, 0, SEEK_END);
	const long Size = std::ftell(fp);
	std::fseek(fp, CurPos, SEEK_SET);

	if (Size < 0) {
		return 0;
	}

	return Size;

#endif
}


FileStreamGenericC::OffsetType FileStreamGenericC::GetPos()
{
	if (!m_File) {
		return 0;
	}

#ifdef _MSC_VER
	const __int64 Pos = ::_ftelli64(m_File.get());
#elif !defined(LIBISDB_NO_POSIX_FILE)
	const ::off_t Pos = ::ftello(m_File.get());
#else
	const long Pos = std::ftell(m_File.get());
#endif

	if ((Pos == -1) && (errno > 0)) {
		return 0;
	}

	return Pos;
}


bool FileStreamGenericC::SetPos(OffsetType Pos, SetPosType Type)
{
	if (!m_File) {
		return false;
	}

	int Origin;
	switch (Type) {
	case SetPosType::Begin:   Origin = SEEK_SET; break;
	case SetPosType::Current: Origin = SEEK_CUR; break;
	case SetPosType::End:     Origin = SEEK_END; break;
	default:
		return false;
	}

#ifdef _MSC_VER
	return ::_fseeki64(m_File.get(), Pos, Origin) == 0;
#elif !defined(LIBISDB_NO_POSIX_FILE)
	return ::fseeko(m_File.get(), Pos, Origin) == 0;
#else
	return std::fseek(m_File.get(), Pos, Origin) == 0;
#endif
}


bool FileStreamGenericC::IsEnd() const
{
	if (!m_File)
		return false;

	return std::feof(m_File.get()) != 0;
}


}	// namespace LibISDB
