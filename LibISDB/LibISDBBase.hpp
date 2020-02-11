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
 @file   LibISDBBase.hpp
 @brief  LibISDB 基本ヘッダ
 @author DBCTRADO
*/


#ifndef LIBISDB_BASE_H
#define LIBISDB_BASE_H


#include "LibISDBConfig.hpp"


#if defined(_WIN32) || defined(WIN32) || defined(_WIN64)
#define LIBISDB_WINDOWS
#if defined(UNICODE) || defined(_UNICODE)
#define LIBISDB_WCHAR
#endif
#define LIBISDB_NEWLINE "\r\n"
#endif

#if defined(__STDC_ISO_10646__) || defined(LIBISDB_WINDOWS)
#define LIBISDB_WCHAR_IS_UNICODE
#endif

#if defined(_M_IX86) || defined(__i386__)
#define LIBISDB_X86
#endif
#if defined(_M_AMD64) || defined(__x86_64__)
#define LIBISDB_X64
#endif

#if defined(LIBISDB_X86) || defined(LIBISDB_X64)
#ifndef LIBISDB_NO_SSE
#define LIBISDB_SSE_SUPPORT
#ifndef LIBISDB_NO_SSE2
#define LIBISDB_SSE2_SUPPORT
#endif	// ifndef LIBISDB_NO_SSE2
#endif	// ifndef LIBISDB_NO_SSE
#endif

#if !defined(NDEBUG) && (defined(_DEBUG) || defined(DEBUG))
#define LIBISDB_DEBUG
#endif

#if defined(_MSC_VER)
#define LIBISDB_PRAGMA_MSVC(x) __pragma(x)
#if !defined(_SECURE_SCL) && !defined(LIBISDB_DEBUG)
#define _SECURE_SCL 0
#endif
#endif

#ifndef LIBISDB_PRAGMA_MSVC
#define LIBISDB_PRAGMA_MSVC(x)
#endif

#ifdef __has_attribute
#define LIBISDB_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#define LIBISDB_HAS_ATTRIBUTE(x) 0
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define LIBISDB_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || LIBISDB_HAS_ATTRIBUTE(always_inline)
#define LIBISDB_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define LIBISDB_FORCE_INLINE inline
#endif

#ifndef LIBISDB_MSB_FIRST
#if defined(__BIG_ENDIAN__)
#define LIBISDB_MSB_FIRST
#endif
#endif

#if defined(_MSC_VER)
#define LIBISDB_PRAGMA_PACK_PUSH_1 LIBISDB_PRAGMA_MSVC(pack(push, 1))
#define LIBISDB_PRAGMA_PACK_POP    LIBISDB_PRAGMA_MSVC(pack(pop))
#else
#define LIBISDB_PRAGMA_PACK_PUSH_1
#define LIBISDB_PRAGMA_PACK_POP
#endif

#if defined(__GNUC__) || LIBISDB_HAS_ATTRIBUTE(packed)
#define LIBISDB_ATTRIBUTE_PACKED __attribute__((packed))
#else
#define LIBISDB_ATTRIBUTE_PACKED
#endif

#if !defined(LIBISDB_NO_SEH) && defined(_MSC_VER)
#define LIBISDB_ENABLE_SEH
#define LIBISDB_SEH_TRY       __try
#define LIBISDB_SEH_EXCEPT(x) __except(x)
#define LIBISDB_SEH_FINALLY   __finally
#else
#define LIBISDB_SEH_TRY
#define LIBISDB_SEH_EXCEPT(x) if constexpr (false)
#define LIBISDB_SEH_FINALLY
#endif

#if defined(_POSIX_C_SOURCE) || defined(_XOPEN_SOURCE)
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#define _LARGEFILE64_SOURCE
#endif

#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <limits>
#include <utility>
#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>
#include <optional>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cassert>
#ifdef LIBISDB_WCHAR
#include <cwchar>
#endif

#include "Templates/cstring_view.hpp"
#include "Templates/EnumFlags.hpp"
#include "Templates/ReturnArg.hpp"


#ifdef LIBISDB_DEBUG
#define LIBISDB_ASSERT assert
#else
#define LIBISDB_ASSERT(x) ((void)0)
#endif


namespace LibISDB
{

	inline namespace StdDef
	{
		using std::size_t;
#ifndef RSIZE_MAX
		typedef std::size_t rsize_t;
		constexpr rsize_t RSIZE_MAX = std::numeric_limits<rsize_t>::max() / 2;
#endif
	}

	inline namespace StdInt
	{
		using std::int8_t;
		using std::uint8_t;
		using std::int16_t;
		using std::uint16_t;
		using std::int32_t;
		using std::uint32_t;
		using std::int64_t;
		using std::uint64_t;
		using std::int_fast8_t;
		using std::uint_fast8_t;
		using std::int_fast16_t;
		using std::uint_fast16_t;
		using std::int_fast32_t;
		using std::uint_fast32_t;
		using std::int_fast64_t;
		using std::uint_fast64_t;
		using std::int_least8_t;
		using std::uint_least8_t;
		using std::int_least16_t;
		using std::uint_least16_t;
		using std::int_least32_t;
		using std::uint_least32_t;
		using std::int_least64_t;
		using std::uint_least64_t;
		using std::intmax_t;
		using std::uintmax_t;
		using std::intptr_t;
		using std::uintptr_t;
	}

	inline namespace Literals
	{

		constexpr int8_t operator"" _i8(unsigned long long a) {
			return static_cast<int8_t>(a);
		}
		constexpr uint8_t operator"" _u8(unsigned long long a) {
			return static_cast<uint8_t>(a);
		}
		constexpr int16_t operator"" _i16(unsigned long long a) {
			return static_cast<int16_t>(a);
		}
		constexpr uint16_t operator"" _u16(unsigned long long a) {
			return static_cast<uint16_t>(a);
		}
		constexpr int32_t operator"" _i32(unsigned long long a) {
			return static_cast<int32_t>(a);
		}
		constexpr uint32_t operator"" _u32(unsigned long long a) {
			return static_cast<uint32_t>(a);
		}
		constexpr int64_t operator"" _i64(unsigned long long a) {
			return static_cast<int64_t>(a);
		}
		constexpr uint64_t operator"" _u64(unsigned long long a) {
			return static_cast<uint64_t>(a);
		}
		constexpr size_t operator"" _z(unsigned long long a) {
			return static_cast<size_t>(a);
		}

	}	// namespace Literals

#define LIBISDB_CAT_SYMBOL_(a, b) a##b
#define LIBISDB_CAT_SYMBOL(a, b) LIBISDB_CAT_SYMBOL_(a, b)

#ifdef LIBISDB_WCHAR
	typedef wchar_t CharType;
#define LIBISDB_STR(s)  LIBISDB_CAT_SYMBOL(L, s)
#define LIBISDB_CHAR(c) LIBISDB_CAT_SYMBOL(L, c)
#else
	typedef char CharType;
#define LIBISDB_STR(s)  s
#define LIBISDB_CHAR(c) c
#endif

#ifdef LIBISDB_WINDOWS
#define LIBISDB_PRIs "hs"
#define LIBISDB_PRIc "hc"
#else
#define LIBISDB_PRIs "s"
#define LIBISDB_PRIc "c"
#endif
#define LIBISDB_PRIls "ls"
#define LIBISDB_PRIlc "lc"

#if defined(LIBISDB_WINDOWS) && (!defined(LIBISDB_WCHAR) || !defined(_CRT_STDIO_ISO_WIDE_SPECIFIERS))
#define LIBISDB_PRIS "s"
#define LIBISDB_PRIC "c"
#else
#ifdef LIBISDB_WCHAR
#define LIBISDB_PRIS LIBISDB_PRIls
#define LIBISDB_PRIC LIBISDB_PRIlc
#else
#define LIBISDB_PRIS LIBISDB_PRIs
#define LIBISDB_PRIC LIBISDB_PRIc
#endif
#endif

#ifndef LIBISDB_NEWLINE
#define LIBISDB_NEWLINE "\n"
#endif

	typedef std::basic_string<CharType> String;
	typedef std::basic_string_view<CharType> StringView;
	typedef basic_cstring_view<CharType> CStringView;

}	// namespace LibISDB


#endif	// ifndef LIBISDB_BASE_H
