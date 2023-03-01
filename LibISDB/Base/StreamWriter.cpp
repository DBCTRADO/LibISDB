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
 @file   StreamWriter.cpp
 @brief  ストリーム書き出し
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamWriter.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


FileStreamWriter::FileStreamWriter() noexcept
	: m_WriteSize(0)
{
}


FileStreamWriter::~FileStreamWriter()
{
	Close();
}


bool FileStreamWriter::Open(const CStringView &FileName, OpenFlag Flags)
{
	if (m_File)
		return false;

	FileStream *pFile = OpenFile(FileName, Flags);
	if (pFile == nullptr)
		return false;

	m_File.reset(pFile);
	m_WriteSize = 0;

	ResetError();

	return true;
}


bool FileStreamWriter::Reopen(const CStringView &FileName, OpenFlag Flags)
{
	FileStream *pFile = OpenFile(FileName, Flags);

	if (pFile == nullptr)
		return false;

	Close();

	m_File.reset(pFile);

	return true;
}


void FileStreamWriter::Close()
{
	if (m_File) {
		m_File->Close();
		m_File.reset();
	}
}


bool FileStreamWriter::IsOpen() const
{
	return static_cast<bool>(m_File);
}


size_t FileStreamWriter::Write(const void *pBuffer, size_t Size)
{
	if (!m_File) {
		return 0;
	}

	const size_t Write = m_File->Write(pBuffer, Size);

	m_WriteSize += Write;

	return Write;
}


bool FileStreamWriter::GetFileName(String *pFileName) const
{
	if (pFileName == nullptr)
		return false;

	if (!m_File) {
		pFileName->clear();
		return false;
	}

	*pFileName = m_File->GetFileName();

	return !pFileName->empty();
}


StreamWriter::SizeType FileStreamWriter::GetWriteSize() const
{
	return m_WriteSize;
}


bool FileStreamWriter::IsWriteSizeAvailable() const
{
	return static_cast<bool>(m_File);
}


bool FileStreamWriter::SetPreallocationUnit(SizeType PreallocationUnit)
{
	if (!m_File)
		return false;

	return m_File->SetPreallocationUnit(PreallocationUnit);
}


FileStream * FileStreamWriter::OpenFile(const CStringView &FileName, OpenFlag Flags)
{
	FileStream *pFile = new FileStream;
	FileStream::OpenFlag StreamFlags = FileStream::OpenFlag::Write | FileStream::OpenFlag::ShareRead;
	if (!(Flags & OpenFlag::Overwrite))
		StreamFlags |= FileStream::OpenFlag::New;
	else
		StreamFlags |= FileStream::OpenFlag::Create | FileStream::OpenFlag::Truncate;

	if (!pFile->Open(FileName, StreamFlags)) {
		SetError(pFile->GetLastErrorDescription());
		delete pFile;
		return nullptr;
	}

	return pFile;
}


}	// namespace LibISDB
