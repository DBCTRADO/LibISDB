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
 @file   ErrorHandler.hpp
 @brief  エラー処理用
 @author DBCTRADO
*/


#ifndef LIBISDB_ERROR_HANDLER_H
#define LIBISDB_ERROR_HANDLER_H


#include <system_error>


namespace LibISDB
{

	/** エラー文字列クラス */
	class ErrorString
	{
	public:
		ErrorString() noexcept;
		ErrorString(const ErrorString &Src) noexcept;
		ErrorString(ErrorString &&Src) noexcept;
		~ErrorString();

		ErrorString & operator = (const ErrorString &Src) noexcept;
		ErrorString & operator = (ErrorString &&Src) noexcept;
		ErrorString & operator = (const CharType *pSrc) noexcept;

		void clear() noexcept;
		bool empty() const noexcept;
		const CharType * c_str() const noexcept;

	private:
		static CharType * DuplicateString(const CharType *pSrc) noexcept;

		CharType *m_pString;
	};

	/** エラー詳細内容クラス */
	class ErrorDescription
	{
	public:
		ErrorDescription() noexcept;
		ErrorDescription(
			const std::error_code &ErrorCode,
			const CharType *pText = nullptr,
			const CharType *pAdvise = nullptr,
			const CharType *pSystemMessage = nullptr) noexcept;
		ErrorDescription(
			int Code,
			const std::error_category &Category,
			const CharType *pText = nullptr,
			const CharType *pAdvise = nullptr,
			const CharType *pSystemMessage = nullptr) noexcept
			: ErrorDescription(std::error_code(Code, Category), pText, pAdvise, pSystemMessage)
		{
		}
		template<typename T> ErrorDescription(
			T Code,
			const CharType *pText = nullptr,
			const CharType *pAdvise = nullptr,
			const CharType *pSystemMessage = nullptr) noexcept
			: ErrorDescription(std::make_error_code(Code), pText, pAdvise, pSystemMessage)
		{
		}

		void Reset() noexcept;
		void SetErrorCode(const std::error_code &ErrorCode) noexcept;
		void SetErrorCode(int Code, const std::error_category &Category) noexcept;
		template<typename T> void SetErrorCode(T Code) noexcept
		{
			SetErrorCode(std::make_error_code(Code));
		}
		const std::error_code & GetErrorCode() const noexcept { return m_ErrorCode; }
		void SetText(const CharType *pText) noexcept;
		const CharType * GetText() const noexcept { return m_Text.c_str(); }
		void SetAdvise(const CharType *pAdvise) noexcept;
		const CharType * GetAdvise() const noexcept { return m_Advise.c_str(); }
		void SetSystemMessage(const CharType *pSystemMessage) noexcept;
		const CharType * GetSystemMessage() const noexcept { return m_SystemMessage.c_str(); }

	protected:
		std::error_code m_ErrorCode;
		ErrorString m_Text;
		ErrorString m_Advise;
		ErrorString m_SystemMessage;
	};

	/** エラー内容を保持するクラス */
	class ErrorHandler
	{
	public:
		ErrorHandler() = default;
		ErrorHandler(const ErrorHandler &) = default;
		ErrorHandler(ErrorHandler &&) = default;
		virtual ~ErrorHandler();
		ErrorHandler & operator = (const ErrorHandler &) = default;
		ErrorHandler & operator = (ErrorHandler &&) = default;

		const ErrorDescription & GetLastErrorDescription() const noexcept;
		const std::error_code & GetLastErrorCode() const noexcept;
		const CharType * GetLastErrorText() const noexcept;
		const CharType * GetLastErrorAdvise() const noexcept;
		const CharType * GetLastErrorSystemMessage() const noexcept;

	protected:
		void SetError(
			const std::error_code &ErrorCode,
			const CharType *pText = nullptr,
			const CharType *pAdvise = nullptr,
			const CharType *pSystemMessage = nullptr) noexcept;
		void SetError(
			int Code,
			const std::error_category &Category,
			const CharType *pText = nullptr,
			const CharType *pAdvise = nullptr,
			const CharType *pSystemMessage = nullptr) noexcept
		{
			SetError(std::error_code(Code, Category), pText, pAdvise, pSystemMessage);
		}
		template<typename T> void SetError(
			T Code,
			const CharType *pText = nullptr,
			const CharType *pAdvise = nullptr,
			const CharType *pSystemMessage = nullptr) noexcept
		{
			SetError(std::make_error_code(Code), pText, pAdvise, pSystemMessage);
		}
		void SetError(const ErrorDescription &Error) noexcept;
		void SetErrorCode(const std::error_code &ErrorCode) noexcept;
		void SetErrorCode(int Code, const std::error_category &Category) noexcept;
		template<typename T> void SetErrorCode(T Code) noexcept
		{
			SetErrorCode(std::make_error_code(Code));
		}
		void SetErrorText(const CharType *pText) noexcept;
		void SetErrorAdvise(const CharType *pAdvise) noexcept;
		void SetErrorSystemMessage(const CharType *pSystemMessage) noexcept;
		void ResetError() noexcept;

#ifdef LIBISDB_WINDOWS
		void SetWin32Error(uint32_t ErrorCode, const CharType *pText = nullptr) noexcept;
		bool SetErrorSystemMessageByWin32ErrorCode(uint32_t ErrorCode) noexcept;
		void SetHRESULTError(int32_t ErrorCode, const CharType *pText = nullptr) noexcept;
#endif

	private:
		ErrorDescription m_ErrorDescription;
	};

#ifdef LIBISDB_WINDOWS
	const std::error_category & HRESULTErrorCategory() noexcept;

	inline std::error_code HRESULTErrorCode(int32_t hr) noexcept
	{
		return std::error_code(hr, HRESULTErrorCategory());
	}
#endif

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ERROR_HANDLER_H
