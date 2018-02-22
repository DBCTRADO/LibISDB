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
 @file   EDCBPluginWriter.cpp
 @brief  EDCB プラグイン書き出し
 @author DBCTRADO
*/


#include "../../LibISDBPrivate.hpp"
#include "../../LibISDBWindows.hpp"
#include "EDCBPluginWriter.hpp"
#include "../../Base/DebugDef.hpp"


namespace std
{

	template<> struct is_error_code_enum<LibISDB::EDCBPluginWriter::ErrorCode> : public true_type {};

	error_code make_error_code(LibISDB::EDCBPluginWriter::ErrorCode Code) noexcept
	{
		return std::error_code(static_cast<int>(Code), LibISDB::EDCBPluginWriter::GetErrorCategory());
	}

}


namespace LibISDB
{


EDCBPluginWriter::ErrorCategory EDCBPluginWriter::m_ErrorCategory;


EDCBPluginWriter::EDCBPluginWriter()
	: m_hLib(nullptr)
	, m_ID(0)
	, m_IsOpen(false)
	, m_hFile(INVALID_HANDLE_VALUE)
	, m_PreallocationUnit(0)
{
}


EDCBPluginWriter::~EDCBPluginWriter()
{
	Free();
}


bool EDCBPluginWriter::Load(const CStringView &FileName)
{
	if (m_hLib != nullptr)
		return false;

	HMODULE hLib = ::LoadLibrary(FileName.c_str());
	if (hLib == nullptr) {
		SetWin32Error(::GetLastError(), LIBISDB_STR("DLLをロードできません。"));
		return false;
	}

	CreateCtrl pCreateCtrl =
		reinterpret_cast<CreateCtrl>(::GetProcAddress(hLib, "CreateCtrl"));
	m_pDeleteCtrl =
		reinterpret_cast<DeleteCtrl>(::GetProcAddress(hLib, "DeleteCtrl"));
	m_pStartSave =
		reinterpret_cast<StartSave>(::GetProcAddress(hLib, "StartSave"));
	m_pStopSave =
		reinterpret_cast<StopSave>(::GetProcAddress(hLib, "StopSave"));
	m_pGetSaveFilePath =
		reinterpret_cast<GetSaveFilePath>(::GetProcAddress(hLib, "GetSaveFilePath"));
	m_pAddTSBuff =
		reinterpret_cast<AddTSBuff>(::GetProcAddress(hLib, "AddTSBuff"));

	if (!pCreateCtrl || !m_pDeleteCtrl || !m_pStartSave || !m_pStopSave
			|| !m_pGetSaveFilePath || !m_pAddTSBuff) {
		::FreeLibrary(hLib);
		SetError(ErrorCode::GetPluginFunction, LIBISDB_STR("プラグインから必要な関数を取得できません。"));
		return false;
	}

	DWORD ID;
	if (!pCreateCtrl(&ID)) {
		::FreeLibrary(hLib);
		SetError(ErrorCode::CreateCtrl, LIBISDB_STR("保存用インスタンスを作成できません。"));
		return false;
	}

	m_hLib = hLib;
	m_ID = ID;

	return true;
}


void EDCBPluginWriter::Free()
{
	if (m_hLib) {
		Close();

		m_pDeleteCtrl(m_ID);
		m_ID = 0;

		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
	}
}


bool EDCBPluginWriter::Open(const CStringView &FileName, OpenFlag Flags)
{
	if (!m_hLib || m_IsOpen) {
		return false;
	}

	if (!m_pStartSave(m_ID, FileName.c_str(), !!(Flags & OpenFlag::Overwrite), m_PreallocationUnit)) {
		SetError(ErrorCode::StartSave, LIBISDB_STR("プラグインでの保存を開始できません。"));
		return false;
	}

	m_IsOpen = true;

	WCHAR ActualPath[MAX_PATH];
	DWORD PathLength = static_cast<DWORD>(CountOf(ActualPath));
	if (m_pGetSaveFilePath(m_ID, ActualPath, &PathLength)) {
		m_hFile = ::CreateFileW(
			ActualPath,
			FILE_READ_ATTRIBUTES,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, 0, nullptr);
	}

	ResetError();

	return true;
}


bool EDCBPluginWriter::Reopen(const CStringView &FileName, OpenFlag Flags)
{
	Close();

	return Open(FileName, Flags);
}


void EDCBPluginWriter::Close()
{
	if (m_IsOpen) {
		m_pStopSave(m_ID);
		m_IsOpen = false;
	}

	if (m_hFile != INVALID_HANDLE_VALUE) {
		::CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
	}
}


bool EDCBPluginWriter::IsOpen() const
{
	return m_IsOpen;
}


size_t EDCBPluginWriter::Write(const void *pBuffer, size_t Size)
{
	if (!m_IsOpen) {
		return 0;
	}

#ifdef _WIN64
	if (Size > std::numeric_limits<DWORD>::max())
		Size = std::numeric_limits<DWORD>::max();
#endif

	DWORD Write = 0;
	m_pAddTSBuff(m_ID, static_cast<BYTE *>(const_cast<void *>(pBuffer)), (DWORD)Size, &Write);

	return Write;
}


bool EDCBPluginWriter::GetFileName(String *pFileName) const
{
	if (pFileName == nullptr)
		return false;

	pFileName->clear();

	if (!m_IsOpen)
		return false;

	WCHAR Path[MAX_PATH];
	DWORD Size = static_cast<DWORD>(CountOf(Path));
	if (!m_pGetSaveFilePath(m_ID, Path, &Size) || (Size == 0))
		return false;

	*pFileName = Path;

	return true;
}


StreamWriter::SizeType EDCBPluginWriter::GetWriteSize() const
{
	LARGE_INTEGER FileSize;

	if ((m_hFile == INVALID_HANDLE_VALUE)
			|| !::GetFileSizeEx(m_hFile, &FileSize))
		return 0;

	return FileSize.QuadPart;
}


bool EDCBPluginWriter::IsWriteSizeAvailable() const
{
	return m_hFile != INVALID_HANDLE_VALUE;
}


bool EDCBPluginWriter::SetPreallocationUnit(SizeType PreallocationUnit)
{
	m_PreallocationUnit = PreallocationUnit;

	return true;
}


std::string EDCBPluginWriter::ErrorCategory::message(int ev) const
{
	switch (static_cast<ErrorCode>(ev)) {
	case ErrorCode::GetPluginFunction:
		return std::string("Cannot retrieve plugin function");
	case ErrorCode::CreateCtrl:
		return std::string("CreateCtrl failed");
	case ErrorCode::StartSave:
		return std::string("StartSave failed");
	}

	return std::string();
}


}	// namespace LibISDB
