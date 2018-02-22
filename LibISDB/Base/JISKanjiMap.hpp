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
 @file   JISKanjiMap.hpp
 @brief  JIS X 0213 漢字変換
 @author DBCTRADO
*/


#ifndef LIBISDB_JIS_KANJI_MAP_H
#define LIBISDB_JIS_KANJI_MAP_H


namespace LibISDB
{

	size_t JISX0213KanjiToUTF32(int Plane, uint16_t Code, char32_t *pUTF32, size_t Length);
	size_t JISX0213KanjiToUTF16(int Plane, uint16_t Code, char16_t *pUTF16, size_t Length);
	size_t JISX0213KanjiToUTF8(int Plane, uint16_t Code, char *pUTF8, size_t Length);

#if defined(LIBISDB_WCHAR) && defined(LIBISDB_WCHAR_IS_UNICODE)
	inline size_t JISX0213KanjiToWChar(int Plane, uint16_t Code, wchar_t *pWChar, size_t Length)
	{
#if WCHAR_MAX == 0xFFFF
		return JISX0213KanjiToUTF16(Plane, Code, reinterpret_cast<char16_t *>(pWChar), Length);
#elif WCHAR_MAX == 0xFFFFFFFF
		return JISX0213KanjiToUTF32(Plane, Code, reinterpret_cast<char32_t *>(pWChar), Length);
#else
		static_assert(false);
#endif
	}
#endif

}	// namespace LibISDB


#endif	// ifndef LIBISDB_JIS_KANJI_MAP_H
