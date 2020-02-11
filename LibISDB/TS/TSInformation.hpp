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
 @file   TSInformation.hpp
 @brief  TS 情報
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_INFORMATION_H
#define LIBISDB_TS_INFORMATION_H


namespace LibISDB
{

	const CharType * GetStreamTypeText(uint8_t StreamType);
	const CharType * GetAreaText_ja(uint16_t AreaCode);
	const CharType * GetComponentTypeText_ja(uint8_t StreamContent, uint8_t ComponentType);
	const CharType * GetVideoComponentTypeText_ja(uint8_t ComponentType);
	const CharType * GetAudioComponentTypeText_ja(uint8_t ComponentType);
	const CharType * GetPredefinedPIDText(uint16_t PID);

	enum class LanguageTextType {
		Long,
		Simple,
		Short,
	};

	constexpr size_t MAX_LANGUAGE_TEXT_LENGTH = 16;

	bool GetLanguageText_ja(
		uint32_t LanguageCode, CharType *pText, size_t MaxText,
		LanguageTextType Type = LanguageTextType::Long);
	bool LanguageCodeToText(
		uint32_t LanguageCode, CharType *pText, size_t MaxText, bool UpperCase = true);

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TS_INFORMATION_H
