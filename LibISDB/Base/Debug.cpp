/*
  LibISDB
  Copyright(c) 2017-2019 DBCTRADO

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
 @file   Debug.cpp
 @brief  デバッグ
 @author DBCTRADO
*/


#include "../LibISDBBase.hpp"
#include "Debug.hpp"
#include "DateTime.hpp"
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#endif
#include "../Utilities/StringUtilities.hpp"


namespace LibISDB
{


void DebugTrace(TraceType Type, const CharType *pFormat, ...)
{
	std::va_list Args;

	va_start(Args, pFormat);
	DebugTraceV(Type, pFormat, Args);
	va_end(Args);
}


void DebugTraceV(TraceType Type, const CharType *pFormat, std::va_list Args)
{
	CharType Buffer[MAX_TRACE_TEXT_LENGTH];
	DateTime Time;

	Time.NowLocal();

#ifdef LIBISDB_WINDOWS

	int Length = StringPrintf(
		Buffer,
		LIBISDB_STR("%02d/%02d %02d:%02d:%02d %04X > %") LIBISDB_STR(LIBISDB_PRIS),
		Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second,
		::GetCurrentThreadId(),
		Type == TraceType::Warning ? LIBISDB_STR("[Warning] ") :
		Type == TraceType::Error   ? LIBISDB_STR("[Error] ") :
		LIBISDB_STR(""));
	StringPrintfV(Buffer + Length, CountOf(Buffer) - Length, pFormat, Args);
	::OutputDebugString(Buffer);

#else

	int Length = StringPrintf(
		Buffer,
		LIBISDB_STR("%02d/%02d %02d:%02d:%02d > %") LIBISDB_STR(LIBISDB_PRIS),
		Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second,
#ifndef LIBISDB_NO_ANSI_ESCAPE_SEQUENCE
		Type == TraceType::Warning ? LIBISDB_STR("\033[33m[Warning]\033[0m ") :
		Type == TraceType::Error   ? LIBISDB_STR("\033[31m[Error]\033[0m ") :
#else
		Type == TraceType::Warning ? LIBISDB_STR("[Warning] ") :
		Type == TraceType::Error   ? LIBISDB_STR("[Error] ") :
#endif
		LIBISDB_STR(""));
	StringPrintfV(Buffer + Length, CountOf(Buffer) - Length, pFormat, Args);

#ifdef LIBISDB_WCHAR
	std::fputws(Buffer, stderr);
#else
	std::fputs(Buffer, stderr);
#endif

#endif
}


bool TraceIf(TraceType Type, bool Condition, const char *pExpression, const char *pFile, int Line)
{
	if (Condition) {
		DebugTrace(
			Type,
			LIBISDB_STR("%") LIBISDB_STR(LIBISDB_PRIs) LIBISDB_STR("(%d): %") LIBISDB_STR(LIBISDB_PRIs) LIBISDB_STR("\n"),
			pFile, Line, pExpression);
	}

	return Condition;
}


}	// namespace LibISDB
