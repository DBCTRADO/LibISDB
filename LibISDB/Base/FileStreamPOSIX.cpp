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
 @file   FileStreamPOSIX.cpp
 @brief  ファイルストリームのPOSIX実装
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "FileStreamPOSIX.hpp"
#include "../Utilities/StringUtilities.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef LIBISDB_WINDOWS
#include <io.h>
#include <share.h>
#else
#include <unistd.h>
#endif

#include "DebugDef.hpp"


namespace
{

#ifdef LIBISDB_WINDOWS

typedef __int64 off64_t;
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef long ssize_t;
#endif

inline int posix_close(int fd) { return ::_close(fd); }
inline int fsync(int fd) { return ::_commit(fd); }
inline off64_t lseek64(int fd, off64_t offset, int origin) { return ::_lseeki64(fd, offset, origin); }
inline off64_t tell64(int fd) { return ::_telli64(fd); }
inline off64_t filelength64(int fd) { return ::_filelengthi64(fd); }

ssize_t posix_read(int fd, void *buffer, std::size_t count)
{
#ifdef _WIN64
	if (count > std::numeric_limits<unsigned int>::max())
		return 0;
#endif
	return ::_read(fd, buffer, static_cast<unsigned int>(count));
}

ssize_t posix_write(int fd, const void *buffer, std::size_t count)
{
#ifdef _WIN64
	if (count > std::numeric_limits<unsigned int>::max())
		return 0;
#endif
	return ::_write(fd, buffer, static_cast<unsigned int>(count));
}

#else	// LIBISDB_WINDOWS

inline int posix_close(int fd) { return ::close(fd); }
inline ::off64_t tell64(int fd) { return ::lseek64(fd, 0, SEEK_CUR); }

::off64_t filelength64(int fd)
{
	struct ::stat Stat;

	if (::fstat(fd, &Stat) != 0)
		return -1;

	return Stat.st_size;
}

inline ::ssize_t posix_read(int fd, void *buffer, std::size_t count) { return ::read(fd, buffer, count); }
inline ::ssize_t posix_write(int fd, const void *buffer, std::size_t count) { return ::write(fd, buffer, count); }

#endif	// ndef LIBISDB_WINDOWS

}	// namespace


namespace LibISDB
{


FileStreamPOSIX::FileStreamPOSIX()
	: m_File(-1)
	, m_EOF(false)
	, m_Closer(DefaultCloser())
{
}


FileStreamPOSIX::FileStreamPOSIX(const Closer &closer)
	: m_File(-1)
	, m_EOF(false)
	, m_Closer(closer)
{
}


FileStreamPOSIX::~FileStreamPOSIX()
{
	Close();
}


bool FileStreamPOSIX::Open(const CStringView &FileName, OpenFlag Flags)
{
	if (m_File >= 0) {
		SetError(std::errc::operation_in_progress);
		return false;
	}

	if (FileName.empty()
			|| !(Flags & (OpenFlag::Read | OpenFlag::Write))) {
		SetError(std::errc::invalid_argument);
		return false;
	}

#ifdef LIBISDB_WINDOWS

	int OFlags = _O_BINARY;

	switch (Flags & (OpenFlag::Read | OpenFlag::Write)) {
	case OpenFlag::Read:
		OFlags |= _O_RDONLY;
		break;
	case OpenFlag::Write:
		OFlags |= _O_WRONLY;
		break;
	case OpenFlag::Read | OpenFlag::Write:
		OFlags |= _O_RDWR;
		break;
	}

	if (!!(Flags & OpenFlag::New))
		OFlags |= _O_CREAT | _O_EXCL;
	else if (!!(Flags & OpenFlag::Truncate))
		OFlags |= _O_CREAT | _O_TRUNC;
	else if (!!(Flags & OpenFlag::Append))
		OFlags |= _O_APPEND;
	if (!!(Flags & OpenFlag::SequentialRead))
		OFlags |= _O_SEQUENTIAL;

	int Share;
	switch (Flags & (OpenFlag::ShareRead | OpenFlag::ShareWrite)) {
	case OpenFlag::ShareRead:
		Share = _SH_DENYWR;
		break;
	case OpenFlag::ShareWrite:
		Share = _SH_DENYRD;
		break;
	case OpenFlag::ShareRead | OpenFlag::ShareWrite:
		Share = _SH_DENYNO;
		break;
	case OpenFlag::None:
		Share = _SH_DENYRW;
		break;
	}

	LIBISDB_TRACE(
		LIBISDB_STR("FileStreamPOSIX::Open() : Open file \"%")
			LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("\" %x %x\n"),
		FileName.c_str(), OFlags, Share);

	const ::errno_t Err =
#ifdef LIBISDB_WCHAR
		::_wsopen_s
#else
		::_sopen_s
#endif
			(&m_File, FileName.c_str(), OFlags, Share, _S_IREAD | _S_IWRITE);
	if (Err != 0) {
		SetError(static_cast<std::errc>(Err));
		return false;
	}

#else	// LIBISDB_WINDOWS

	int OFlags;

	switch (Flags & (OpenFlag::Read | OpenFlag::Write)) {
	case OpenFlag::Read:
		OFlags = O_RDONLY;
		break;
	case OpenFlag::Write:
		OFlags = O_WRONLY;
		break;
	case OpenFlag::Read | OpenFlag::Write:
		OFlags = O_RDWR;
		break;
	}

	if (!!(Flags & OpenFlag::New))
		OFlags |= O_CREAT | O_EXCL;
	else if (!!(Flags & OpenFlag::Truncate))
		OFlags |= O_CREAT | O_TRUNC;
	else if (!!(Flags & OpenFlag::Append))
		OFlags |= O_APPEND;

	LIBISDB_TRACE(
		LIBISDB_STR("FileStreamPOSIX::Open() : Open file \"%")
			LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("\" %x\n"),
		FileName.c_str(), OFlags);

	m_File = ::open(FileName.c_str(), OFlags);
	if (m_File < 0) {
		SetError(static_cast<std::errc>(errno));
		return false;
	}

#endif	// ndef LIBISDB_WINDOWS

	m_FileName = FileName;
	m_EOF = false;

	ResetError();

	return true;
}


bool FileStreamPOSIX::Close()
{
	if (m_File >= 0) {
		m_Closer(m_File);
		m_File = -1;
	}

	m_FileName.clear();
	m_EOF = false;

	return true;
}


bool FileStreamPOSIX::IsOpen() const
{
	return m_File >= 0;
}


size_t FileStreamPOSIX::Read(void *pBuff, size_t Size)
{
	if (m_File < 0) {
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)) {
		return 0;
	}

	const ssize_t Result = posix_read(m_File, pBuff, Size);
	if (Result < 0) {
		return 0;
	}

	if (static_cast<size_t>(Result) < Size) {
		m_EOF = true;
	}

	return Result;
}


size_t FileStreamPOSIX::Write(const void *pBuff, size_t Size)
{
	if (m_File < 0) {
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)) {
		return 0;
	}

	const ssize_t Result = posix_write(m_File, pBuff, Size);
	if (Result < 0) {
		return 0;
	}

	return Result;
}


bool FileStreamPOSIX::Flush()
{
	if (m_File < 0) {
		return false;
	}

	return fsync(m_File) == 0;
}


FileStreamPOSIX::SizeType FileStreamPOSIX::GetSize()
{
	if (m_File < 0) {
		return 0;
	}

	off64_t Size = filelength64(m_File);
	if (Size < 0) {
		return 0;
	}

	return Size;
}


FileStreamPOSIX::OffsetType FileStreamPOSIX::GetPos()
{
	if (m_File < 0) {
		return 0;
	}

	const off64_t Pos = tell64(m_File);
	if ((Pos == -1) && (errno > 0)) {
		return 0;
	}

	return Pos;
}


bool FileStreamPOSIX::SetPos(OffsetType Pos, SetPosType Type)
{
	if (m_File < 0) {
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

	m_EOF = false;

	return lseek64(m_File, Pos, Origin) == 0;
}


bool FileStreamPOSIX::IsEnd() const
{
	if (m_File < 0)
		return false;

	return m_EOF;
}




void FileStreamPOSIX::DefaultCloser::operator () (int fd) const
{
	posix_close(fd);
}


}	// namespace LibISDB
