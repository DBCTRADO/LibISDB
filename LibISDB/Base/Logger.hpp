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
 @file   Logger.hpp
 @brief  ログ出力
 @author DBCTRADO
*/


#ifndef LIBISDB_LOGGER_H
#define LIBISDB_LOGGER_H


namespace LibISDB
{

	/** ログ出力基底クラス */
	class Logger
	{
	public:
		enum class LogType {
			Verbose,
			Information,
			Warning,
			Error,
		};

		static constexpr size_t MAX_LENGTH = 1024;

		void Log(LogType Type, const CharType *pFormat, ...);
		void LogV(LogType Type, const CharType *pFormat, std::va_list Args);
		void LogRaw(LogType Type, const CharType *pText);

	protected:
		virtual void OnLog(LogType Type, const CharType *pText) = 0;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_LOGGER_H
