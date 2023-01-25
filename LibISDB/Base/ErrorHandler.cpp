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
 @file   ErrorHandler.cpp
 @brief  エラー処理用
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#include <tchar.h>
#endif	// LIBISDB_WINDOWS
#include "ErrorHandler.hpp"
#include "../Utilities/StringFormat.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


ErrorString::ErrorString() noexcept
	: m_pString(nullptr)
{
}


ErrorString::ErrorString(const ErrorString &Src) noexcept
	: m_pString(DuplicateString(Src.m_pString))
{
}


ErrorString::ErrorString(ErrorString &&Src) noexcept
	: m_pString(Src.m_pString)
{
	Src.m_pString = nullptr;
}


ErrorString::~ErrorString()
{
	std::free(m_pString);
}


ErrorString & ErrorString::operator = (const ErrorString &Src) noexcept
{
	if (&Src != this) {
		std::free(m_pString);
		m_pString = DuplicateString(Src.m_pString);
	}
	return *this;
}


ErrorString & ErrorString::operator = (ErrorString &&Src) noexcept
{
	if (&Src != this) {
		std::free(m_pString);
		m_pString = Src.m_pString;
		Src.m_pString = nullptr;
	}
	return *this;
}


ErrorString & ErrorString::operator = (const CharType *pSrc) noexcept
{
	clear();
	m_pString = DuplicateString(pSrc);
	return *this;
}


void ErrorString::clear() noexcept
{
	if (m_pString != nullptr) {
		std::free(m_pString);
		m_pString = nullptr;
	}
}


bool ErrorString::empty() const noexcept
{
	return (m_pString == nullptr) || (m_pString[0] == LIBISDB_CHAR('\0'));
}


const CharType * ErrorString::c_str() const noexcept
{
	if (m_pString == nullptr)
		return LIBISDB_STR("");
	return m_pString;
}


CharType * ErrorString::DuplicateString(const CharType *pSrc) noexcept
{
	if (pSrc == nullptr)
		return nullptr;

	size_t Size = (StringLength(pSrc) + 1) * sizeof(CharType);
	CharType *pNewString = static_cast<CharType *>(std::malloc(Size));
	if (pNewString == nullptr)
		return nullptr;

	std::memcpy(pNewString, pSrc, Size);

	return pNewString;
}




ErrorDescription::ErrorDescription() noexcept
{
}


ErrorDescription::ErrorDescription(
	const std::error_code &ErrorCode,
	const CharType *pText, const CharType *pAdvise, const CharType *pSystemMessage) noexcept
	: m_ErrorCode(ErrorCode)
{
	SetText(pText);
	SetAdvise(pAdvise);
	SetSystemMessage(pSystemMessage);
}


void ErrorDescription::Reset() noexcept
{
	m_ErrorCode.clear();
	m_Text.clear();
	m_Advise.clear();
	m_SystemMessage.clear();
}


void ErrorDescription::SetErrorCode(const std::error_code &ErrorCode) noexcept
{
	m_ErrorCode = ErrorCode;
}


void ErrorDescription::SetErrorCode(int Code, const std::error_category &Category) noexcept
{
	m_ErrorCode.assign(Code, Category);
}


void ErrorDescription::SetText(const CharType *pText) noexcept
{
	m_Text = pText;
}


void ErrorDescription::SetAdvise(const CharType *pAdvise) noexcept
{
	m_Advise = pAdvise;
}


void ErrorDescription::SetSystemMessage(const CharType *pSystemMessage) noexcept
{
	m_SystemMessage = pSystemMessage;
}




ErrorHandler::~ErrorHandler()
{
}


const ErrorDescription &ErrorHandler::GetLastErrorDescription() const noexcept
{
	return m_ErrorDescription;
}


const std::error_code & ErrorHandler::GetLastErrorCode() const noexcept
{
	return m_ErrorDescription.GetErrorCode();
}


const CharType * ErrorHandler::GetLastErrorText() const noexcept
{
	return m_ErrorDescription.GetText();
}


const CharType * ErrorHandler::GetLastErrorAdvise() const noexcept
{
	return m_ErrorDescription.GetAdvise();
}


const CharType * ErrorHandler::GetLastErrorSystemMessage() const noexcept
{
	return m_ErrorDescription.GetSystemMessage();
}


void ErrorHandler::SetError(
	const std::error_code &ErrorCode,
	const CharType *pText, const CharType *pAdvise, const CharType *pSystemMessage) noexcept
{
	m_ErrorDescription = ErrorDescription(ErrorCode, pText, pAdvise, pSystemMessage);
}


void ErrorHandler::SetError(const ErrorDescription &Error) noexcept
{
	m_ErrorDescription = Error;
}


void ErrorHandler::SetErrorCode(const std::error_code &ErrorCode) noexcept
{
	m_ErrorDescription.SetErrorCode(ErrorCode);
}


void ErrorHandler::SetErrorCode(int Code, const std::error_category &Category) noexcept
{
	m_ErrorDescription.SetErrorCode(Code, Category);
}


void ErrorHandler::SetErrorText(const CharType *pText) noexcept
{
	m_ErrorDescription.SetText(pText);
}


void ErrorHandler::SetErrorAdvise(const CharType *pAdvise) noexcept
{
	m_ErrorDescription.SetAdvise(pAdvise);
}


void ErrorHandler::SetErrorSystemMessage(const CharType *pSystemMessage) noexcept
{
	m_ErrorDescription.SetSystemMessage(pSystemMessage);
}


void ErrorHandler::ResetError() noexcept
{
	m_ErrorDescription.Reset();
}


#ifdef LIBISDB_WINDOWS


void ErrorHandler::SetWin32Error(uint32_t ErrorCode, const CharType *pText) noexcept
{
	SetError(static_cast<int>(ErrorCode), std::system_category());
	SetErrorText(pText);
	SetErrorSystemMessageByWin32ErrorCode(ErrorCode);
}


bool ErrorHandler::SetErrorSystemMessageByWin32ErrorCode(uint32_t ErrorCode) noexcept
{
	TCHAR Text[256];

	if (::FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
			ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			Text, static_cast<DWORD>(std::size(Text)), nullptr) <= 0) {
		m_ErrorDescription.SetSystemMessage(nullptr);
		return false;
	}

	m_ErrorDescription.SetSystemMessage(Text);

	return true;
}


void ErrorHandler::SetHRESULTError(int32_t ErrorCode, const CharType *pText) noexcept
{
	SetError(static_cast<int>(ErrorCode), HRESULTErrorCategory());
	SetErrorText(pText);
	SetErrorSystemMessageByWin32ErrorCode(ErrorCode);
}




struct ErrorCategory_HRESULT : public std::error_category
{
	const char * name() const noexcept override { return "HRESULT"; }
	std::string message(int ev) const override;
};


std::string ErrorCategory_HRESULT::message(int ev) const
{
	char szText[256];

	if (::FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
			ev, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			szText, (DWORD)std::size(szText), nullptr) <= 0) {
		StringFormat(szText, "HRESULT {:08x}", ev);
	}

	return std::string(szText);
}


const std::error_category & HRESULTErrorCategory() noexcept
{
	static ErrorCategory_HRESULT Category;
	return Category;
}


#endif	// LIBISDB_WINDOWS


}	// namespace LibISDB
