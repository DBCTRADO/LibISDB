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
 @file   StringUtilities.hpp
 @brief  文字列操作
 @author DBCTRADO
*/


#ifndef LIBISDB_STRING_UTILITIES_H
#define LIBISDB_STRING_UTILITIES_H


#if !defined(_MSC_VER) && __has_include(<strings.h>)
#include <strings.h>
#endif

#include <cctype>
#include <cwctype>


namespace LibISDB
{

	template<typename T> inline bool StringIsEmpty(const T *pString) {
		return (pString == nullptr) || (pString[0] == '\0');
	}

	template<typename T> constexpr std::size_t StringLength(const T *pString) {
		return std::char_traits<T>::length(pString);
	}
	inline std::size_t StringLength(const char *pString, std::size_t Length) { return ::strnlen(pString, Length); }
	inline std::size_t StringLength(const wchar_t *pString, std::size_t Length) { return ::wcsnlen(pString, Length); }

	template<typename... TArgs> inline int StringPrintf(char *pString, std::size_t Length, const char *pFormat, TArgs... Args) {
		return std::snprintf(pString, Length, pFormat, Args...);
	}
	template<typename... TArgs> inline int StringPrintf(wchar_t *pString, std::size_t Length, const wchar_t *pFormat, TArgs... Args) {
		return std::swprintf(pString, Length, pFormat, Args...);
	}
	inline int StringPrintfV(char *pString, std::size_t Length, const char *pFormat, std::va_list Args) {
		return std::vsnprintf(pString, Length, pFormat, Args);
	}
	inline int StringPrintfV(wchar_t *pString, std::size_t Length, const wchar_t *pFormat, std::va_list Args) {
		return std::vswprintf(pString, Length, pFormat, Args);
	}
	template<typename T, std::size_t N, typename... TArgs> inline int StringPrintf(T (&String)[N], const T *pFormat, TArgs... Args) {
		return StringPrintf(String, N, pFormat, Args...);
	}
	template<typename T, std::size_t N> inline int StringPrintfV(T (&String)[N], const T *pFormat, std::va_list Args) {
		return StringPrintfV(String, N, pFormat, Args);
	}

LIBISDB_PRAGMA_MSVC(warning(push))
LIBISDB_PRAGMA_MSVC(warning(disable:4996))
	inline void StringCopy(char *pDstString, const char *pSrcString) { std::strcpy(pDstString, pSrcString); }
	inline void StringCopy(wchar_t *pDstString, const wchar_t *pSrcString) { std::wcscpy(pDstString, pSrcString); }
LIBISDB_PRAGMA_MSVC(warning(pop))

#if defined(_MSC_VER)
	inline void StringCopy(char *pDstString, const char *pSrcString, std::size_t Length) {
		::strncpy_s(pDstString, Length, pSrcString, _TRUNCATE);
	}
	inline void StringCopy(wchar_t *pDstString, const wchar_t *pSrcString, std::size_t Length) {
		::wcsncpy_s(pDstString, Length, pSrcString, _TRUNCATE);
	}
#else
	inline void StringCopy(char *pDstString, const char *pSrcString, std::size_t Length) {
		StringPrintf(pDstString, Length, "%s", pSrcString);
	}
	inline void StringCopy(wchar_t *pDstString, const wchar_t *pSrcString, std::size_t Length) {
		StringPrintf(pDstString, Length, L"%ls", pSrcString);
	}
#endif

	template<typename T, std::size_t N> inline void StringCopy(T (&DstString)[N], const T *pSrcString) {
		StringCopy(DstString, pSrcString, N);
	}

	inline int StringCompare(const char *pString1, const char *pString2) {
		return std::strcmp(pString1, pString2);
	}
	inline int StringCompare(const wchar_t *pString1, const wchar_t *pString2) {
		return std::wcscmp(pString1, pString2);
	}
	inline int StringCompare(const char *pString1, const char *pString2, std::size_t Length) {
		return std::strncmp(pString1, pString2, Length);
	}
	inline int StringCompare(const wchar_t *pString1, const wchar_t *pString2, std::size_t Length) {
		return std::wcsncmp(pString1, pString2, Length);
	}

#if defined(_MSC_VER)
	inline int StringCompareI(const char *pString1, const char *pString2) {
		return ::_stricmp(pString1, pString2);
	}
	inline int StringCompareI(const wchar_t *pString1, const wchar_t *pString2) {
		return ::_wcsicmp(pString1, pString2);
	}
	inline int StringCompareI(const char *pString1, const char *pString2, std::size_t Length) {
		return ::_strnicmp(pString1, pString2, Length);
	}
	inline int StringCompareI(const wchar_t *pString1, const wchar_t *pString2, std::size_t Length) {
		return ::_wcsnicmp(pString1, pString2, Length);
	}
#else
	inline int StringCompareI(const char *pString1, const char *pString2) {
		return ::strcasecmp(pString1, pString2);
	}
	inline int StringCompareI(const wchar_t *pString1, const wchar_t *pString2) {
		return ::wcscasecmp(pString1, pString2);
	}
	inline int StringCompareI(const char *pString1, const char *pString2, std::size_t Length) {
		return ::strncasecmp(pString1, pString2, Length);
	}
	inline int StringCompareI(const wchar_t *pString1, const wchar_t *pString2, std::size_t Length) {
		return ::wcsncasecmp(pString1, pString2, Length);
	}
#endif

	inline char ToUpper(char c) { return static_cast<char>(std::toupper(c)); }
	inline wchar_t ToUpper(wchar_t c) { return static_cast<wchar_t>(std::towupper(c)); }
	inline char ToLower(char c) { return static_cast<char>(std::tolower(c)); }
	inline wchar_t ToLower(wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); }

	template<typename T> inline int StringCompareI(
		const std::basic_string<T> &String1, const std::basic_string<T> &String2)
	{
		auto it1  = String1.begin();
		auto end1 = String1.end();
		auto it2  = String2.begin();
		auto end2 = String2.end();

		for (;(it1 != end1) && (it2 != end2); ++it1, ++it2) {
			const T c1 = ToLower(*it1);
			const T c2 = ToLower(*it2);
			if (c1 < c2)
				return -1;
			if (c1 > c2)
				return 1;
		}

		if (it1 != end1)
			return 1;
		if (it2 != end2)
			return -1;
		return 0;
	}

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STRING_UTILITIES_H
