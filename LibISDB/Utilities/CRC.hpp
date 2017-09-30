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
 @file   CRC.hpp
 @brief  CRC 計算
 @author DBCTRADO
*/


#ifndef LIBISDB_CRC_H
#define LIBISDB_CRC_H


#include "Hasher.hpp"


namespace LibISDB
{

	/**< CRC-16 (IBM) */
	struct CRC16 {
		typedef uint16_t ValueType;
		static constexpr ValueType InitialValue = 0x0000_u16;

		static ValueType Calc(const uint8_t *pData, size_t DataSize, ValueType CRC = InitialValue) noexcept;
	};

	/**< CRC-16-CCITT */
	struct CRC16CCITT {
		typedef uint16_t ValueType;
		static constexpr ValueType InitialValue = 0x0000_u16;

		static ValueType Calc(const uint8_t *pData, size_t DataSize, ValueType CRC = InitialValue) noexcept;
	};

	/**< CRC-32 (ISO 3309 / ITU-T V.42) */
	struct CRC32 {
		typedef uint32_t ValueType;
		static constexpr ValueType InitialValue = 0x00000000_u32;

		static ValueType Calc(const uint8_t *pData, size_t DataSize, ValueType CRC = InitialValue) noexcept;
	};

	/**< CRC-32/MPEG-2 (ISO/IEC 13818-1) */
	struct CRC32MPEG2 {
		typedef uint32_t ValueType;
		static constexpr ValueType InitialValue = 0xFFFFFFFF_u32;

		static ValueType Calc(const uint8_t *pData, size_t DataSize, ValueType CRC = InitialValue) noexcept;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_CRC_H
