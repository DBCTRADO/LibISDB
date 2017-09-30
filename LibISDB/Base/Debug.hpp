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
 @file   Debug.hpp
 @brief  デバッグ
 @author DBCTRADO
*/


#ifndef LIBISDB_DEBUG_H
#define LIBISDB_DEBUG_H


#ifdef LIBISDB_DEBUG
#if defined(_MSC_VER)
#if !defined(_CRTDBG_MAP_ALLOC) && !defined(LIBISDB_NO_MAP_ALLOC)
#define _CRTDBG_MAP_ALLOC
#endif
#include <crtdbg.h>
#define LIBISDB_DEBUG_NEW ::new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else	// _MSC_VER
#include <cassert>
#endif	// !def _MSC_VER
#endif	// LIBISDB_DEBUG


namespace LibISDB
{

	constexpr std::size_t MAX_TRACE_TEXT_LENGTH = 1024;

	enum class TraceType {
		Verbose,
		Information,
		Warning,
		Error,
	};

	void DebugTrace(TraceType Type, const CharType *pFormat, ...);
	void DebugTraceV(TraceType Type, const CharType *pFormat, std::va_list Args);

#if !defined(LIBISDB_ENABLE_TRACE) && !defined(LIBISDB_NO_TRACE) && defined(LIBISDB_DEBUG)
#define LIBISDB_ENABLE_TRACE
#endif

#ifdef LIBISDB_ENABLE_TRACE
#define LIBISDB_TRACE_ LibISDB::DebugTrace
#else
#define LIBISDB_TRACE_(...) ((void)0)
#endif

#define LIBISDB_TRACE(...)         LIBISDB_TRACE_(LibISDB::TraceType::Information, __VA_ARGS__)
#ifdef LIBISDB_ENABLE_VERBOSE_TRACE
#define LIBISDB_TRACE_VERBOSE(...) LIBISDB_TRACE_(LibISDB::TraceType::Verbose, __VA_ARGS__)
#else
#define LIBISDB_TRACE_VERBOSE(...) ((void)0)
#endif
#define LIBISDB_TRACE_WARNING(...) LIBISDB_TRACE_(LibISDB::TraceType::Warning, __VA_ARGS__)
#define LIBISDB_TRACE_ERROR(...)   LIBISDB_TRACE_(LibISDB::TraceType::Error, __VA_ARGS__)

	bool TraceIf(TraceType Type, bool Condition, const char *pExpression, const char *pFile, int Line);

#ifdef LIBISDB_ENABLE_TRACE
#define LIBISDB_TRACE_IF(Type, Condition) \
	LibISDB::TraceIf(Type, Condition, #Condition, __FILE__, __LINE__)
#define LIBISDB_TRACE_WARNING_IF(Condition) \
	LibISDB::TraceIf(LibISDB::TraceType::Warning, Condition, #Condition, __FILE__, __LINE__)
#define LIBISDB_TRACE_WARNING_IF_NOT(Condition) \
	(!LibISDB::TraceIf(LibISDB::TraceType::Warning, !(Condition), "!(" #Condition ")", __FILE__, __LINE__))
#define LIBISDB_TRACE_ERROR_IF(Condition) \
	LibISDB::TraceIf(LibISDB::TraceType::Error, Condition, #Condition, __FILE__, __LINE__)
#define LIBISDB_TRACE_ERROR_IF_NOT(Condition) \
	(!LibISDB::TraceIf(LibISDB::TraceType::Error, !(Condition), "!(" #Condition ")", __FILE__, __LINE__))
#else
#define LIBISDB_TRACE_IF(Condition)             (Condition)
#define LIBISDB_TRACE_WARNING_IF(Condition)     (Condition)
#define LIBISDB_TRACE_WARNING_IF_NOT(Condition) (Condition)
#define LIBISDB_TRACE_ERROR_IF(Condition)       (Condition)
#define LIBISDB_TRACE_ERROR_IF_NOT(Condition)   (Condition)
#endif

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DEBUG_H
