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
 @file   StringFormat.cpp
 @brief  文字列フォーマット
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StringFormat.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{

namespace
{


template<typename T> class StringBufferOutputIterator
{
public:
	using value_type = T;
	using reference = T&;
	using pointer = T*;
	using difference_type = std::ptrdiff_t;

	StringBufferOutputIterator() noexcept
		: m_pBuffer(nullptr)
		, m_pCurrent(nullptr)
		, m_pEnd(nullptr)
	{
	}

	StringBufferOutputIterator(T *pBuffer, size_t Size) noexcept
		: m_pBuffer(pBuffer)
		, m_pCurrent(pBuffer)
		, m_pEnd(pBuffer + Size)
	{
	}

	StringBufferOutputIterator & operator ++ () noexcept
	{
		if (m_pCurrent < m_pEnd)
			++m_pCurrent;
		return *this;
	}

	StringBufferOutputIterator operator ++ (int) noexcept
	{
		if (m_pCurrent < m_pEnd) {
			const StringBufferOutputIterator Old = *this;
			m_pCurrent++;
			return Old;
		}

		return *this;
	}

	T & operator * () noexcept
	{
		if (m_pCurrent < m_pEnd)
			return *m_pCurrent;
		return m_Dummy;
	}

	T * Begin() const noexcept { return m_pBuffer; }
	T * End() const noexcept { return m_pEnd; }
	T * Current() const noexcept { return m_pCurrent; }

protected:
	T *m_pBuffer;
	T *m_pCurrent;
	T *m_pEnd;
	T m_Dummy{};
};


}




const std::locale & GetDefaultLocaleClass()
{
	static std::locale DefaultLocale("");

	return DefaultLocale;
}


std::string StringVFormatArgs(std::string_view Format, std::format_args Args)
{
	return std::vformat(Format, Args);
}


std::wstring StringVFormatArgs(std::wstring_view Format, std::wformat_args Args)
{
	return std::vformat(Format, Args);
}


void StringVFormatArgs(std::string *pOutString, std::string_view Format, std::format_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr))
		return;
	*pOutString = std::vformat(Format, Args);
}


void StringVFormatArgs(std::wstring *pOutString, std::wstring_view Format, std::wformat_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr))
		return;
	*pOutString = std::vformat(Format, Args);
}


size_t StringVFormatArgs(
	char *pOutString, size_t MaxOutLength, std::string_view Format, std::format_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr || MaxOutLength < 1))
		return 0;
	const StringBufferOutputIterator<char> it(pOutString, MaxOutLength - 1);
	auto itEnd = std::vformat_to(it, Format, Args);
	*itEnd.Current() = 0;
	return itEnd.Current() - pOutString;
}


size_t StringVFormatArgs(
	wchar_t *pOutString, size_t MaxOutLength, std::wstring_view Format, std::wformat_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr || MaxOutLength < 1))
		return 0;
	const StringBufferOutputIterator<wchar_t> it(pOutString, MaxOutLength - 1);
	auto itEnd = std::vformat_to(it, Format, Args);
	*itEnd.Current() = 0;
	return itEnd.Current() - pOutString;
}


std::string StringVFormatArgs(
	const std::locale &Locale,
	std::string_view Format, std::format_args Args)
{
	return std::vformat(Locale, Format, Args);
}


std::wstring StringVFormatArgs(
	const std::locale &Locale,
	std::wstring_view Format, std::wformat_args Args)
{
	return std::vformat(Locale, Format, Args);
}


void StringVFormatArgs(
	std::string *pOutString, const std::locale &Locale,
	std::string_view Format, std::format_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr))
		return;
	*pOutString = std::vformat(Locale, Format, Args);
}


void StringVFormatArgs(
	std::wstring *pOutString, const std::locale &Locale,
	std::wstring_view Format, std::wformat_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr))
		return;
	*pOutString = std::vformat(Locale, Format, Args);
}


size_t StringVFormatArgs(
	char *pOutString, size_t MaxOutLength, const std::locale &Locale,
	std::string_view Format, std::format_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr || MaxOutLength < 1))
		return 0;
	const StringBufferOutputIterator<char> it(pOutString, MaxOutLength - 1);
	auto itEnd = std::vformat_to(it, Locale, Format, Args);
	*itEnd.Current() = 0;
	return itEnd.Current() - pOutString;
}


size_t StringVFormatArgs(
	wchar_t *pOutString, size_t MaxOutLength, const std::locale &Locale,
	std::wstring_view Format, std::wformat_args Args)
{
	if (LIBISDB_TRACE_ERROR_IF(pOutString == nullptr || MaxOutLength < 1))
		return 0;
	const StringBufferOutputIterator<wchar_t> it(pOutString, MaxOutLength - 1);
	auto itEnd = std::vformat_to(it, Locale, Format, Args);
	*itEnd.Current() = 0;
	return itEnd.Current() - pOutString;
}


}	// namespace LibISDB
