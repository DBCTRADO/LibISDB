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
 @file   Debug.cpp
 @brief  デバッグ
 @author DBCTRADO
*/


#include "../LibISDBBase.hpp"
#include "Debug.hpp"
#include "DateTime.hpp"
#include "../Utilities/StringFormat.hpp"
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#endif


namespace LibISDB
{


void DebugTraceV(TraceType Type, std::string_view Format, std::format_args Args)
{
	char Buffer[MAX_TRACE_TEXT_LENGTH];
	DateTime Time;

	Time.NowLocal();

#ifdef LIBISDB_WINDOWS

	const size_t Length = StringFormat(
		Buffer,
		"{:02}/{:02} {:02}:{:02}:{:02} {:04X} > {}",
		Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second,
		::GetCurrentThreadId(),
		Type == TraceType::Warning ? "[Warning] " :
		Type == TraceType::Error   ? "[Error] " :
		"");
	StringVFormatArgs(Buffer + Length, std::size(Buffer) - Length, Format, Args);
	::OutputDebugStringA(Buffer);

#else

	const size_t Length = StringFormat(
		Buffer,
		"{:02}/{:02} {:02}:{:02}:{:02} > {}",
		Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second,
#ifndef LIBISDB_NO_ANSI_ESCAPE_SEQUENCE
		Type == TraceType::Warning ? "\033[33m[Warning]\033[0m " :
		Type == TraceType::Error   ? "\033[31m[Error]\033[0m " :
#else
		Type == TraceType::Warning ? "[Warning] " :
		Type == TraceType::Error   ? "[Error] " :
#endif
		"");
	StringVFormatArgs(Buffer + Length, std::size(Buffer) - Length, Format, Args);
	std::fputs(Buffer, stderr);

#endif
}


void DebugTraceV(TraceType Type, std::wstring_view Format, std::wformat_args Args)
{
	wchar_t Buffer[MAX_TRACE_TEXT_LENGTH];
	DateTime Time;

	Time.NowLocal();

#ifdef LIBISDB_WINDOWS

	const size_t Length = StringFormat(
		Buffer,
		L"{:02}/{:02} {:02}:{:02}:{:02} {:04X} > {}",
		Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second,
		::GetCurrentThreadId(),
		Type == TraceType::Warning ? L"[Warning] " :
		Type == TraceType::Error   ? L"[Error] " :
		L"");
	StringVFormatArgs(Buffer + Length, std::size(Buffer) - Length, Format, Args);
	::OutputDebugStringW(Buffer);

#else

	const size_t Length = StringFormat(
		Buffer,
		L"{:02}/{:02} {:02}:{:02}:{:02} > {}",
		Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second,
#ifndef LIBISDB_NO_ANSI_ESCAPE_SEQUENCE
		Type == TraceType::Warning ? L"\033[33m[Warning]\033[0m " :
		Type == TraceType::Error   ? L"\033[31m[Error]\033[0m " :
#else
		Type == TraceType::Warning ? L"[Warning] " :
		Type == TraceType::Error   ? L"[Error] " :
#endif
		L"");
	StringVFormatArgs(Buffer + Length, std::size(Buffer) - Length, Format, Args);
	std::fputws(Buffer, stderr);

#endif
}


bool TraceIf(TraceType Type, bool Condition, const char *pExpression, const std::source_location &Location)
{
	if (Condition) {
		DebugTrace(Type, "{}({}): {}\n", Location.file_name(), Location.line(), pExpression);
	}

	return Condition;
}


}	// namespace LibISDB
