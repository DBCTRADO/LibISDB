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
 @file   StringFormat.hpp
 @brief  文字列フォーマット
 @author DBCTRADO
*/


#ifndef LIBISDB_STRING_FORMAT_H
#define LIBISDB_STRING_FORMAT_H


#include <format>
#include <locale>


namespace LibISDB
{

#ifdef LIBISDB_WCHAR

	using FormatArgs = std::wformat_args;
	using FormatContext = std::wformat_context;

	template<typename... TArgs> auto MakeFormatArgs(const TArgs&... Args)
	{
		return std::make_wformat_args(Args...);
	}

#else

	using FormatArgs = std::format_args;
	using FormatContext = std::format_context;

	template<typename... TArgs> auto MakeFormatArgs(const TArgs&... Args)
	{
		return std::make_format_args(Args...);
	}

#endif

	const std::locale & GetDefaultLocaleClass();

	std::string StringVFormatArgs(std::string_view Format, std::format_args Args);
	std::wstring StringVFormatArgs(std::wstring_view Format, std::wformat_args Args);
	void StringVFormatArgs(std::string *pOutString, std::string_view Format, std::format_args Args);
	void StringVFormatArgs(std::wstring *pOutString, std::wstring_view Format, std::wformat_args Args);
	size_t StringVFormatArgs(
		char *pOutString, size_t MaxOutLength, std::string_view Format, std::format_args Args);
	size_t StringVFormatArgs(
		wchar_t *pOutString, size_t MaxOutLength, std::wstring_view Format, std::wformat_args Args);

	template<typename... TArgs> void StringVFormat(std::string *pOutString, std::string_view Format, const TArgs&... Args)
	{
		StringVFormatArgs(pOutString, Format, std::make_format_args(Args...));
	}

	template<typename... TArgs> void StringVFormat(std::wstring *pOutString, std::wstring_view Format, const TArgs&... Args)
	{
		StringVFormatArgs(pOutString, Format, std::make_wformat_args(Args...));
	}

	template<typename... TArgs> size_t StringVFormat(
		char *pOutString, size_t MaxOutLength, std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(pOutString, MaxOutLength, Format, std::make_format_args(Args...));
	}

	template<typename... TArgs> size_t StringVFormat(
		wchar_t *pOutString, size_t MaxOutLength, std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(pOutString, MaxOutLength, Format, std::make_wformat_args(Args...));
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringVFormat(
		char (&OutString)[MaxOutLength], std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(OutString, MaxOutLength, Format, std::make_format_args(Args...));
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringVFormat(
		wchar_t (&OutString)[MaxOutLength], std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(OutString, MaxOutLength, Format, std::make_wformat_args(Args...));
	}

	std::string StringVFormatArgs(const std::locale &Locale, std::string_view Format, std::format_args Args);
	std::wstring StringVFormatArgs(const std::locale &Locale, std::wstring_view Format, std::wformat_args Args);
	void StringVFormatArgs(
		std::string *pOutString, const std::locale &Locale,
		std::string_view Format, std::format_args Args);
	void StringVFormatArgs(
		std::wstring *pOutString, const std::locale &Locale,
		std::wstring_view Format, std::wformat_args Args);
	size_t StringVFormatArgs(
		char *pOutString, size_t MaxOutLength, const std::locale &Locale,
		std::string_view Format, std::format_args Args);
	size_t StringVFormatArgs(
		wchar_t *pOutString, size_t MaxOutLength, const std::locale &Locale,
		std::wstring_view Format, std::wformat_args Args);

	template<typename... TArgs> void StringVFormat(
		std::string *pOutString, const std::locale &Locale, std::string_view Format, const TArgs&... Args)
	{
		StringVFormatArgs(pOutString, Locale, Format, std::make_format_args(Args...));
	}

	template<typename... TArgs> void StringVFormat(
		std::wstring *pOutString, const std::locale &Locale, std::wstring_view Format, const TArgs&... Args)
	{
		StringVFormatArgs(pOutString, Locale, Format, std::make_wformat_args(Args...));
	}

	template<typename... TArgs> size_t StringVFormat(
		char *pOutString, size_t MaxOutLength, const std::locale &Locale,
		std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(pOutString, MaxOutLength, Locale, Format, std::make_format_args(Args...));
	}

	template<typename... TArgs> size_t StringVFormat(
		wchar_t *pOutString, size_t MaxOutLength, const std::locale &Locale,
		std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(pOutString, MaxOutLength, Locale, Format, std::make_wformat_args(Args...));
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringVFormat(
		char (&OutString)[MaxOutLength], const std::locale &Locale,
		std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(OutString, MaxOutLength, Locale, Format, std::make_format_args(Args...));
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringVFormat(
		wchar_t (&OutString)[MaxOutLength], const std::locale &Locale,
		std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(OutString, MaxOutLength, Locale, Format, std::make_wformat_args(Args...));
	}

	template<typename... TArgs> void StringVFormatLocale(
		std::string *pOutString, std::string_view Format, const TArgs&... Args)
	{
		StringVFormatArgs(pOutString, GetDefaultLocaleClass(), Format, std::make_format_args(Args...));
	}

	template<typename... TArgs> void StringVFormatLocale(
		std::wstring *pOutString, std::wstring_view Format, const TArgs&... Args)
	{
		StringVFormatArgs(pOutString, GetDefaultLocaleClass(), Format, std::make_wformat_args(Args...));
	}

	template<typename... TArgs> size_t StringVFormatLocale(
		char *pOutString, size_t MaxOutLength, std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(pOutString, MaxOutLength, GetDefaultLocaleClass(), Format, std::make_format_args(Args...));
	}

	template<typename... TArgs> size_t StringVFormatLocale(
		wchar_t *pOutString, size_t MaxOutLength, std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(pOutString, MaxOutLength, GetDefaultLocaleClass(), Format, std::make_wformat_args(Args...));
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringVFormatLocale(
		char (&OutString)[MaxOutLength], std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(OutString, MaxOutLength, GetDefaultLocaleClass(), Format, std::make_format_args(Args...));
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringVFormatLocale(
		wchar_t (&OutString)[MaxOutLength], std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatArgs(OutString, MaxOutLength, GetDefaultLocaleClass(), Format, std::make_wformat_args(Args...));
	}

	/*
		StringFormat / StringFormatLocale は、本来であれば std::format を使い、コンパイル時に書式文字列の正当性がチェックされるべきだが、
		C++20 の時点では書式文字列を定数式のまま転送することができないため、とりあえず std::vformat を使っている。
	*/

	template<typename... TArgs> void StringFormat(std::string *pOutString, std::string_view Format, const TArgs&... Args)
	{
		StringVFormat(pOutString, Format, Args...);
	}

	template<typename... TArgs> void StringFormat(std::wstring *pOutString, std::wstring_view Format, const TArgs&... Args)
	{
		StringVFormat(pOutString, Format, Args...);
	}

	template<typename... TArgs> size_t StringFormat(
		char *pOutString, size_t MaxOutLength, std::string_view Format, const TArgs&... Args)
	{
		return StringVFormat(pOutString, MaxOutLength, Format, Args...);
	}

	template<typename... TArgs> size_t StringFormat(
		wchar_t *pOutString, size_t MaxOutLength, std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormat(pOutString, MaxOutLength, Format, Args...);
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringFormat(
		char (&OutString)[MaxOutLength], std::string_view Format, const TArgs&... Args)
	{
		return StringVFormat(OutString, Format, Args...);
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringFormat(
		wchar_t (&OutString)[MaxOutLength], std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormat(OutString, Format, Args...);
	}

	template<typename... TArgs> void StringFormat(
		std::string *pOutString, const std::locale &Locale,
		std::string_view Format, const TArgs&... Args)
	{
		StringVFormat(pOutString, Locale, Format, Args...);
	}

	template<typename... TArgs> void StringFormat(
		std::wstring *pOutString, const std::locale &Locale,
		std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormat(pOutString, Locale, Format, Args...);
	}

	template<typename... TArgs> size_t StringFormat(
		char *pOutString, size_t MaxOutLength,
		const std::locale &Locale,
		std::string_view Format, const TArgs&... Args)
	{
		return StringVFormat(pOutString, MaxOutLength, Locale, Format, Args...);
	}

	template<typename... TArgs> size_t StringFormat(
		wchar_t *pOutString, size_t MaxOutLength,
		const std::locale &Locale,
		std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormat(pOutString, MaxOutLength, Locale, Format, Args...);
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringFormat(
		char (&OutString)[MaxOutLength],
		const std::locale &Locale,
		std::string_view Format, const TArgs&... Args)
	{
		return StringVFormat(OutString, Locale, Format, Args...);
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringFormat(
		wchar_t (&OutString)[MaxOutLength],
		const std::locale &Locale,
		std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormat(OutString, Locale, Format, Args...);
	}

	template<typename... TArgs> void StringFormatLocale(
		std::string *pOutString, std::string_view Format, const TArgs&... Args)
	{
		StringVFormatLocale(pOutString, Format, Args...);
	}

	template<typename... TArgs> void StringFormatLocale(
		std::wstring *pOutString, std::wstring_view Format, const TArgs&... Args)
	{
		StringVFormatLocale(pOutString, Format, Args...);
	}

	template<typename... TArgs> size_t StringFormatLocale(
		char *pOutString, size_t MaxOutLength,
		std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatLocale(pOutString, MaxOutLength, Format, Args...);
	}

	template<typename... TArgs> size_t StringFormatLocale(
		wchar_t *pOutString, size_t MaxOutLength,
		std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatLocale(pOutString, MaxOutLength, Format, Args...);
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringFormatLocale(
		char (&OutString)[MaxOutLength],
		std::string_view Format, const TArgs&... Args)
	{
		return StringVFormatLocale(OutString, Format, Args...);
	}

	template<size_t MaxOutLength, typename... TArgs> size_t StringFormatLocale(
		wchar_t (&OutString)[MaxOutLength],
		std::wstring_view Format, const TArgs&... Args)
	{
		return StringVFormatLocale(OutString, Format, Args...);
	}

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STRING_FORMAT_H
