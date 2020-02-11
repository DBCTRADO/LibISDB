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
 @file   Logger.cpp
 @brief  ログ出力
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "Logger.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


void Logger::Log(LogType Type, const CharType *pFormat, ...)
{
	std::va_list Args;

	va_start(Args, pFormat);
	LogV(Type, pFormat, Args);
	va_end(Args);
}


void Logger::LogV(LogType Type, const CharType *pFormat, std::va_list Args)
{
	CharType Buffer[MAX_LENGTH];

	StringPrintfV(Buffer, pFormat, Args);
	OnLog(Type, Buffer);
}


void Logger::LogRaw(LogType Type, const CharType *pText)
{
	OnLog(Type, pText);
}


}	// namespace LibISDB
