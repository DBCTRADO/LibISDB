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
 @file   FileStreamWindows.cpp
 @brief  ファイルストリームの Windows 実装
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"


#ifdef LIBISDB_WINDOWS


#include "FileStreamWindows.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


FileStreamWindows::FileStreamWindows() noexcept
	: m_hFile(INVALID_HANDLE_VALUE)

	, m_PreallocationUnit(0)
	, m_PreallocatedSize(0)
	, m_IsPreallocationFailed(false)
{
}


FileStreamWindows::~FileStreamWindows()
{
	Close();
}


bool FileStreamWindows::Open(const String &FileName, OpenFlag Flags)
{
	if (m_hFile != INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_in_progress);
		return false;
	}

	if (FileName.empty()
			|| !(Flags & (OpenFlag::Read | OpenFlag::Write))) {
		SetError(std::errc::invalid_argument);
		return false;
	}

	DWORD Access = 0;
	if (!!(Flags & OpenFlag::Read))
		Access |= GENERIC_READ;
	if (!!(Flags & OpenFlag::Write))
		Access |= GENERIC_WRITE;

	DWORD Share = 0;
	if (!!(Flags & OpenFlag::ShareRead))
		Share |= FILE_SHARE_READ;
	if (!!(Flags & OpenFlag::ShareWrite))
		Share |= FILE_SHARE_WRITE;
	if (!!(Flags & OpenFlag::ShareDelete))
		Share |= FILE_SHARE_DELETE;

	DWORD Create;
	if (!!(Flags & OpenFlag::New)) {
		Create = CREATE_NEW;
	} else if (!!(Flags & OpenFlag::Truncate)) {
		if (!!(Flags & OpenFlag::Create))
			Create = CREATE_ALWAYS;
		else
			Create = TRUNCATE_EXISTING;
	} else if (!!(Flags & OpenFlag::Create)) {
		Create = OPEN_ALWAYS;
	} else {
		Create = OPEN_EXISTING;
	}

	DWORD Attributes = FILE_ATTRIBUTE_NORMAL;
	if (!!(Flags & OpenFlag::SequentialRead))
		Attributes |= FILE_FLAG_SEQUENTIAL_SCAN;
	if (!!(Flags & OpenFlag::RandomAccess))
		Attributes |= FILE_FLAG_RANDOM_ACCESS;

	// 長いパス対応
	String Path;
	if ((FileName.length() >= MAX_PATH) && (FileName.compare(0, 3, LIBISDB_STR("\\\\?")) != 0)) {
		if ((FileName[0] == LIBISDB_CHAR('\\')) && (FileName[1] == LIBISDB_CHAR('\\'))) {
			Path = LIBISDB_STR("\\\\?\\UNC");
			Path += FileName.c_str() + 1;
		} else {
			Path = LIBISDB_STR("\\\\?\\");
			Path += FileName;
		}
	}

	// ファイルを開く
	LIBISDB_TRACE(
		LIBISDB_STR("FileStreamWindows::Open() : Open file \"{}\"\n"),
		!Path.empty() ? Path.c_str() : FileName.c_str());
	m_hFile = ::CreateFile(
		!Path.empty() ? Path.c_str() : FileName.c_str(),
		Access, Share, nullptr, Create, Attributes, nullptr);
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetWin32Error(::GetLastError());
		return false;
	}

	if (!!(Flags & OpenFlag::Append))
		::SetFilePointer(m_hFile, 0, nullptr, FILE_END);

	// I/O優先度の設定
	if (!!(Flags & (OpenFlag::PriorityLow | OpenFlag::PriorityIdle))) {
		alignas(8) FILE_IO_PRIORITY_HINT_INFO PriorityHint;
		PriorityHint.PriorityHint = !!(Flags & OpenFlag::PriorityIdle) ? IoPriorityHintVeryLow : IoPriorityHintLow;
		LIBISDB_TRACE(
			LIBISDB_STR("Set file I/O priority hint {}\n"),
			static_cast<std::underlying_type_t<PRIORITY_HINT>>(PriorityHint.PriorityHint));
		if (!::SetFileInformationByHandle(m_hFile, FileIoPriorityHintInfo, &PriorityHint, sizeof(PriorityHint))) {
			LIBISDB_TRACE(LIBISDB_STR("Failed (Error {:#x})\n"), ::GetLastError());
		}
	}

	m_FileName = FileName;

	m_PreallocatedSize = 0;
	m_IsPreallocationFailed = false;

	ResetError();

	return true;
}


bool FileStreamWindows::Close()
{
	bool OK = true;

	if (m_hFile != INVALID_HANDLE_VALUE) {
		if (m_PreallocatedSize != 0)
			::SetEndOfFile(m_hFile);

		if (!::CloseHandle(m_hFile)) {
			SetWin32Error(::GetLastError());
			OK = false;
		}
		m_hFile = INVALID_HANDLE_VALUE;
	}

	m_FileName.clear();

	return OK;
}


bool FileStreamWindows::IsOpen() const
{
	return m_hFile != INVALID_HANDLE_VALUE;
}


size_t FileStreamWindows::Read(void *pBuff, size_t Size)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)
#ifdef _WIN64
			|| (Size > std::numeric_limits<uint32_t>::max())
#endif
		) {
		SetError(std::errc::invalid_argument);
		return 0;
	}

	DWORD Read = 0;

	if (!::ReadFile(m_hFile, pBuff, static_cast<DWORD>(Size), &Read, nullptr)) {
		SetWin32Error(::GetLastError());
		return 0;
	}

	ResetError();

	return Read;
}


size_t FileStreamWindows::Write(const void *pBuff, size_t Size)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return 0;
	}

	if ((pBuff == nullptr) || (Size == 0)
#ifdef _WIN64
			|| (Size > std::numeric_limits<uint32_t>::max())
#endif
		) {
		SetError(std::errc::invalid_argument);
		return 0;
	}

	if ((m_PreallocationUnit != 0) && !m_IsPreallocationFailed) {
		LARGE_INTEGER CurPos, FileSize;

		if (::SetFilePointerEx(m_hFile, LARGE_INTEGER{0, 0}, &CurPos, FILE_CURRENT)
				&& ::GetFileSizeEx(m_hFile, &FileSize)
				&& CurPos.QuadPart + static_cast<LONGLONG>(Size) > FileSize.QuadPart) {
			const LONGLONG ExtendSize = RoundUp(static_cast<LONGLONG>(Size), static_cast<LONGLONG>(m_PreallocationUnit));
			LIBISDB_TRACE(
				LIBISDB_STR("Preallocate file: {} + {} bytes ({})\n"),
				FileSize.QuadPart, ExtendSize, m_FileName);
			FileSize.QuadPart += ExtendSize;
			if (::SetFilePointerEx(m_hFile, FileSize, nullptr, FILE_BEGIN)) {
				if (::SetEndOfFile(m_hFile)) {
					m_PreallocatedSize = FileSize.QuadPart;
				} else {
					LIBISDB_TRACE(LIBISDB_STR("SetEndOfFile() failed (Error {:#x})\n"), ::GetLastError());
					m_IsPreallocationFailed = true;
				}
				::SetFilePointerEx(m_hFile, CurPos, nullptr, FILE_BEGIN);
			}
		}
	}

	DWORD Written = 0;

	if (!::WriteFile(m_hFile, pBuff, static_cast<DWORD>(Size), &Written, nullptr)) {
		SetWin32Error(::GetLastError());
		return 0;
	}
	if (Written != static_cast<DWORD>(Size)) {
		SetWin32Error(ERROR_WRITE_FAULT);
		return Written;
	}

	ResetError();

	return Written;
}


bool FileStreamWindows::Flush()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return false;
	}

	if (!::FlushFileBuffers(m_hFile)) {
		SetWin32Error(::GetLastError());
		return false;
	}

	ResetError();

	return true;
}


FileStreamWindows::SizeType FileStreamWindows::GetSize()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return 0;
	}

	LARGE_INTEGER FileSize;

	if (!::GetFileSizeEx(m_hFile, &FileSize)) {
		SetWin32Error(::GetLastError());
		return 0;
	}

	ResetError();

	return FileSize.QuadPart;
}


FileStreamWindows::OffsetType FileStreamWindows::GetPos()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return 0;
	}

	LARGE_INTEGER Pos;

	if (!::SetFilePointerEx(m_hFile, LARGE_INTEGER{0, 0}, &Pos, FILE_CURRENT)) {
		SetWin32Error(::GetLastError());
		return 0;
	}

	ResetError();

	return Pos.QuadPart;
}


bool FileStreamWindows::SetPos(OffsetType Pos, SetPosType Type)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return false;
	}

	DWORD MoveMethod;
	switch (Type) {
	case SetPosType::Begin:   MoveMethod = FILE_BEGIN;   break;
	case SetPosType::Current: MoveMethod = FILE_CURRENT; break;
	case SetPosType::End:     MoveMethod = FILE_END;     break;
	default:
		return false;
	}

	LARGE_INTEGER DistanceToMove;
	DistanceToMove.QuadPart = Pos;
	if (!::SetFilePointerEx(m_hFile, DistanceToMove, nullptr, MoveMethod)) {
		SetWin32Error(::GetLastError());
		return false;
	}

	ResetError();

	return true;
}


bool FileStreamWindows::IsEnd() const
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		//SetError(std::errc::operation_not_permitted);
		return false;
	}

	LARGE_INTEGER Size, Pos;

	if (!::GetFileSizeEx(m_hFile, &Size)
			|| !::SetFilePointerEx(m_hFile, LARGE_INTEGER{0, 0}, &Pos, FILE_CURRENT)) {
		//SetWin32Error(::GetLastError());
		return false;
	}

	//ResetError();

	return Pos.QuadPart >= Size.QuadPart;
}


bool FileStreamWindows::Preallocate(SizeType Size)
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return false;
	}

	LARGE_INTEGER FileSize, CurPos;

	if (!::GetFileSizeEx(m_hFile, &FileSize)) {
		SetWin32Error(::GetLastError());
		return false;
	}

	if (static_cast<SizeType>(FileSize.QuadPart) >= Size) {
		SetError(std::errc::invalid_argument);
		return false;
	}

	FileSize.QuadPart = Size;
	if (!::SetFilePointerEx(m_hFile, LARGE_INTEGER{0, 0}, &CurPos, FILE_CURRENT)
			|| !::SetFilePointerEx(m_hFile, FileSize, nullptr, FILE_BEGIN)) {
		SetWin32Error(::GetLastError());
		return false;
	}
	if (!::SetEndOfFile(m_hFile)) {
		SetWin32Error(::GetLastError());
		::SetFilePointerEx(m_hFile, CurPos, nullptr, FILE_BEGIN);
		return false;
	}
	::SetFilePointerEx(m_hFile, CurPos, nullptr, FILE_BEGIN);

	m_PreallocatedSize = Size;

	ResetError();

	return true;
}


bool FileStreamWindows::SetPreallocationUnit(SizeType Unit)
{
	m_PreallocationUnit = Unit;

	return true;
}


FileStreamBase::SizeType FileStreamWindows::GetPreallocationUnit() const
{
	return m_PreallocationUnit;
}


FileStreamBase::SizeType FileStreamWindows::GetPreallocatedSpace()
{
	if (m_hFile == INVALID_HANDLE_VALUE) {
		SetError(std::errc::operation_not_permitted);
		return 0;
	}

	ResetError();

	if (m_PreallocatedSize == 0)
		return 0;

	LARGE_INTEGER Pos;
	if (!::SetFilePointerEx(m_hFile, LARGE_INTEGER{0, 0}, &Pos, FILE_CURRENT)) {
		SetWin32Error(::GetLastError());
		return 0;
	}

	if (Pos.QuadPart >= static_cast<LONGLONG>(m_PreallocatedSize))
		return 0;

	return m_PreallocatedSize - Pos.QuadPart;
}


bool FileStreamWindows::GetTime(FILETIME *pCreationTime, FILETIME *pLastAccessTime, FILETIME *pLastWriteTime) const
{
	if (m_hFile == INVALID_HANDLE_VALUE)
		return false;
	return ::GetFileTime(m_hFile, pCreationTime, pLastAccessTime, pLastWriteTime) != FALSE;
}


}	// namespace LibISDB


#endif	// LIBISDB_WINDOWS
