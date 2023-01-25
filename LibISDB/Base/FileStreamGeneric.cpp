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
 @file   FileStreamGeneric.cpp
 @brief  ファイルストリームのC++標準規格実装
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "FileStreamGeneric.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


FileStreamGeneric::FileStreamGeneric()
{
}


FileStreamGeneric::~FileStreamGeneric()
{
	Close();
}


bool FileStreamGeneric::Open(const CStringView &FileName, OpenFlag Flags)
{
	if (m_Stream.is_open()) {
		SetError(std::errc::operation_in_progress);
		return false;
	}

	if (FileName.empty()
			|| !(Flags & (OpenFlag::Read | OpenFlag::Write))) {
		SetError(std::errc::invalid_argument);
		return false;
	}

	std::ios_base::openmode Mode = std::ios_base::binary;
	if (!!(Flags & OpenFlag::Read))
		Mode |= std::ios_base::in;
	if (!!(Flags & OpenFlag::Write))
		Mode |= std::ios_base::out;
	if (!!(Flags & OpenFlag::Append))
		Mode |= std::ios_base::ate;
	else if (!!(Flags & OpenFlag::Truncate))
		Mode |= std::ios_base::trunc;

	LIBISDB_TRACE(
		LIBISDB_STR("FileStreamGeneric::Open() : Open file \"{}\"\n"),
		FileName);
	m_Stream.exceptions(std::ios_base::failbit);
	try {
		m_Stream.open(FileName.c_str(), Mode);
	} catch (const std::ios_base::failure &Failure) {
		SetError(Failure.code().value(), Failure.code().category());
		return false;
	}
	m_Stream.exceptions(std::ios_base::goodbit);

	m_FileName = FileName;

	ResetError();

	return true;
}


bool FileStreamGeneric::Close()
{
	if (m_Stream.is_open()) {
		m_Stream.close();
	}

	m_FileName.clear();

	return true;
}


bool FileStreamGeneric::IsOpen() const
{
	return m_Stream.is_open();
}


size_t FileStreamGeneric::Read(void *pBuff, size_t Size)
{
	if (!m_Stream.is_open()) {
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)) {
		return 0;
	}

	m_Stream.read(static_cast<char *>(pBuff), Size);

	if (!m_Stream)
		return static_cast<size_t>(m_Stream.gcount());

	return Size;
}


size_t FileStreamGeneric::Write(const void *pBuff, size_t Size)
{
	if (!m_Stream.is_open()) {
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)) {
		return 0;
	}

	return static_cast<size_t>(m_Stream.rdbuf()->sputn(static_cast<const char *>(pBuff), Size));
}


bool FileStreamGeneric::Flush()
{
	if (!m_Stream.is_open()) {
		return false;
	}

	m_Stream.flush();

	return !m_Stream.fail();
}


FileStreamGeneric::SizeType FileStreamGeneric::GetSize()
{
	if (!m_Stream.is_open()) {
		return 0;
	}

	const std::fstream::pos_type CurPos = m_Stream.tellp();
	if (m_Stream.fail())
		return 0;

	m_Stream.seekp(0, std::ios_base::end);
	const std::fstream::pos_type Size = m_Stream.tellp();

	m_Stream.seekp(CurPos);

	return Size < 0 ? static_cast<std::fstream::pos_type>(0) : Size;
}


FileStreamGeneric::OffsetType FileStreamGeneric::GetPos()
{
	if (!m_Stream.is_open()) {
		return 0;
	}

	const std::fstream::pos_type Pos = m_Stream.tellp();

	if (m_Stream.fail()) {
		return 0;
	}

	return Pos;
}


bool FileStreamGeneric::SetPos(OffsetType Pos, SetPosType Type)
{
	if (!m_Stream.is_open()) {
		return false;
	}

	std::fstream::seekdir Dir;
	switch (Type) {
	case SetPosType::Begin:   Dir = std::fstream::beg; break;
	case SetPosType::Current: Dir = std::fstream::cur; break;
	case SetPosType::End:     Dir = std::fstream::end; break;
	default:
		return false;
	}

	m_Stream.seekp(Pos, Dir);

	return !m_Stream.fail();
}


bool FileStreamGeneric::IsEnd() const
{
	return m_Stream.eof();
}


}	// namespace LibISDB
