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
 @file   EDCBPluginWriter.hpp
 @brief  EDCB 保存プラグイン
 @author DBCTRADO
*/


#ifndef LIBISDB_EDCB_PLUGIN_WRITER_H
#define LIBISDB_EDCB_PLUGIN_WRITER_H


#include "../../Base/StreamWriter.hpp"


namespace LibISDB
{

	/** EDCB 保存プラグインクラス */
	class EDCBPluginWriter
		: public StreamWriter
	{
	public:
		enum class ErrorCode {
			Success,
			GetPluginFunction,
			CreateCtrl,
			StartSave,
		};

		struct ErrorCategory : public std::error_category {
			const char * name() const noexcept override { return "EDCBPluginWriter"; }
			std::string message(int ev) const override;
		};

		EDCBPluginWriter();
		~EDCBPluginWriter();

		bool Load(const CStringView &FileName);
		void Free();

	// StreamWriter
		bool Open(const CStringView &FileName, OpenFlag Flags = OpenFlag::None) override;
		bool Reopen(const CStringView &FileName, OpenFlag Flags = OpenFlag::None) override;
		void Close() override;
		bool IsOpen() const override;
		size_t Write(const void *pBuffer, size_t Size) override;
		bool GetFileName(String *pFileName) const override;
		SizeType GetWriteSize() const override;
		bool IsWriteSizeAvailable() const override;
		bool SetPreallocationUnit(SizeType PreallocationUnit) override;

		static const ErrorCategory & GetErrorCategory() noexcept { return m_ErrorCategory; }

	private:
		typedef BOOL (WINAPI *GetPlugInName)(WCHAR *name, DWORD *nameSize);
		typedef void (WINAPI *Setting)(HWND parentWnd);
		typedef BOOL (WINAPI *CreateCtrl)(DWORD *id);
		typedef BOOL (WINAPI *DeleteCtrl)(DWORD id);
		typedef BOOL (WINAPI *StartSave)(DWORD id, LPCWSTR fileName, BOOL overWriteFlag, ULONGLONG createSize);
		typedef BOOL (WINAPI *StopSave)(DWORD id);
		typedef BOOL (WINAPI *GetSaveFilePath)(DWORD id, WCHAR *filePath, DWORD *filePathSize);
		typedef BOOL (WINAPI *AddTSBuff)(DWORD id, BYTE *data, DWORD size, DWORD *writeSize);

		static ErrorCategory m_ErrorCategory;

		HMODULE m_hLib;

		DeleteCtrl m_pDeleteCtrl;
		StartSave m_pStartSave;
		StopSave m_pStopSave;
		GetSaveFilePath m_pGetSaveFilePath;
		AddTSBuff m_pAddTSBuff;

		DWORD m_ID;
		bool m_IsOpen;
		HANDLE m_hFile;

		SizeType m_PreallocationUnit;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_EDCB_PLUGIN_WRITER_H
