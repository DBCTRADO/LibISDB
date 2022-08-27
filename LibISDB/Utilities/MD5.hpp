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
 @file   MD5.hpp
 @brief  MD5 計算
 @author DBCTRADO
*/


#ifndef LIBISDB_MD5_H
#define LIBISDB_MD5_H


namespace LibISDB
{

	/** MD5 のハッシュ値 */
	union MD5Value
	{
		uint8_t Value[16];
		uint32_t Value32[4];

		bool operator == (const MD5Value &rhs) const noexcept
		{
			return std::memcmp(Value, rhs.Value, 16) == 0;
		}
	};

	MD5Value CalcMD5(const uint8_t *pData, size_t DataSize) noexcept;

}	// namespace LibISDB


#endif	// ifndef LIBISDB_MD5_H
