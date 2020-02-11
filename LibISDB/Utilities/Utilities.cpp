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
 @file   Utilities.cpp
 @brief  ユーティリティ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "Utilities.hpp"


namespace LibISDB
{


uint32_t GetBCD(const uint8_t *pData, size_t NibbleLength)
{
	uint32_t Value = 0;
	const size_t Bytes = NibbleLength >> 1;

	for (size_t i = 0; i < Bytes; i++) {
		Value *= 100;
		Value += GetBCD(pData[i]);
	}

	if (NibbleLength & 1) {
		Value *= 10;
		Value += pData[NibbleLength >> 1] >> 4;
	}

	return Value;
}


}	// namespace LibISDB
