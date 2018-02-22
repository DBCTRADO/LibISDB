/*
  LibISDB
  Copyright(c) 2017-2018 DBCTRADO

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
 @file   ObjectBase.cpp
 @brief  オブジェクト基底クラス
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "ObjectBase.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


ObjectBase::ObjectBase()
	: m_pLogger(nullptr)
{
}


void ObjectBase::SetLogger(Logger *pLogger)
{
	m_pLogger = pLogger;
}


void ObjectBase::Log(Logger::LogType Type, const CharType *pFormat, ...)
{
	std::va_list Args;

	va_start(Args, pFormat);
	LogV(Type, pFormat, Args);
	va_end(Args);
}


void ObjectBase::LogV(Logger::LogType Type, const CharType *pFormat, std::va_list Args)
{
	if ((m_pLogger != nullptr) && (pFormat != nullptr)) {
		m_pLogger->LogV(Type, pFormat, Args);
	}
}


void ObjectBase::LogRaw(Logger::LogType Type, const CharType *pText)
{
	if ((m_pLogger != nullptr) && (pText != nullptr)) {
		m_pLogger->LogRaw(Type, pText);
	}
}


}	// namespace LibISDB
