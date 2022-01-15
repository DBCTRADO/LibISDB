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
 @file   MD5.cpp
 @brief  MD5 計算
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "MD5.hpp"
#include "Utilities.hpp"


namespace LibISDB
{


namespace
{

constexpr uint32_t F1(uint32_t x, uint32_t y, uint32_t z) { return z ^ (x & (y ^ z)); }
constexpr uint32_t F2(uint32_t x, uint32_t y, uint32_t z) { return F1(z, x, y); }
constexpr uint32_t F3(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
constexpr uint32_t F4(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

template<typename TFunc> LIBISDB_FORCE_INLINE void MD5Step(
	TFunc func, uint32_t &w, uint32_t x, uint32_t y, uint32_t z, uint32_t data, int shift)
{
	w += func(x, y, z) + data;
	w = RotateLeft32(w, shift);
	w += x;
}

void MD5Transform(uint32_t MD5[4], const uint32_t *p)
{
	uint32_t a, b, c, d;

	a = MD5[0];
	b = MD5[1];
	c = MD5[2];
	d = MD5[3];

	MD5Step(F1, a, b, c, d, p[ 0] + 0xD76AA478_u32,  7);
	MD5Step(F1, d, a, b, c, p[ 1] + 0xE8C7B756_u32, 12);
	MD5Step(F1, c, d, a, b, p[ 2] + 0x242070DB_u32, 17);
	MD5Step(F1, b, c, d, a, p[ 3] + 0xC1BDCEEE_u32, 22);
	MD5Step(F1, a, b, c, d, p[ 4] + 0xF57C0FAF_u32,  7);
	MD5Step(F1, d, a, b, c, p[ 5] + 0x4787C62A_u32, 12);
	MD5Step(F1, c, d, a, b, p[ 6] + 0xA8304613_u32, 17);
	MD5Step(F1, b, c, d, a, p[ 7] + 0xFD469501_u32, 22);
	MD5Step(F1, a, b, c, d, p[ 8] + 0x698098D8_u32,  7);
	MD5Step(F1, d, a, b, c, p[ 9] + 0x8B44F7AF_u32, 12);
	MD5Step(F1, c, d, a, b, p[10] + 0xFFFF5BB1_u32, 17);
	MD5Step(F1, b, c, d, a, p[11] + 0x895CD7BE_u32, 22);
	MD5Step(F1, a, b, c, d, p[12] + 0x6B901122_u32,  7);
	MD5Step(F1, d, a, b, c, p[13] + 0xFD987193_u32, 12);
	MD5Step(F1, c, d, a, b, p[14] + 0xA679438E_u32, 17);
	MD5Step(F1, b, c, d, a, p[15] + 0x49B40821_u32, 22);

	MD5Step(F2, a, b, c, d, p[ 1] + 0xF61E2562_u32,  5);
	MD5Step(F2, d, a, b, c, p[ 6] + 0xC040B340_u32,  9);
	MD5Step(F2, c, d, a, b, p[11] + 0x265E5A51_u32, 14);
	MD5Step(F2, b, c, d, a, p[ 0] + 0xE9B6C7AA_u32, 20);
	MD5Step(F2, a, b, c, d, p[ 5] + 0xD62F105D_u32,  5);
	MD5Step(F2, d, a, b, c, p[10] + 0x02441453_u32,  9);
	MD5Step(F2, c, d, a, b, p[15] + 0xD8A1E681_u32, 14);
	MD5Step(F2, b, c, d, a, p[ 4] + 0xE7D3FBC8_u32, 20);
	MD5Step(F2, a, b, c, d, p[ 9] + 0x21E1CDE6_u32,  5);
	MD5Step(F2, d, a, b, c, p[14] + 0xC33707D6_u32,  9);
	MD5Step(F2, c, d, a, b, p[ 3] + 0xF4D50D87_u32, 14);
	MD5Step(F2, b, c, d, a, p[ 8] + 0x455A14ED_u32, 20);
	MD5Step(F2, a, b, c, d, p[13] + 0xA9E3E905_u32,  5);
	MD5Step(F2, d, a, b, c, p[ 2] + 0xFCEFA3F8_u32,  9);
	MD5Step(F2, c, d, a, b, p[ 7] + 0x676F02D9_u32, 14);
	MD5Step(F2, b, c, d, a, p[12] + 0x8D2A4C8A_u32, 20);

	MD5Step(F3, a, b, c, d, p[ 5] + 0xFFFA3942_u32,  4);
	MD5Step(F3, d, a, b, c, p[ 8] + 0x8771F681_u32, 11);
	MD5Step(F3, c, d, a, b, p[11] + 0x6D9D6122_u32, 16);
	MD5Step(F3, b, c, d, a, p[14] + 0xFDE5380C_u32, 23);
	MD5Step(F3, a, b, c, d, p[ 1] + 0xA4BEEA44_u32,  4);
	MD5Step(F3, d, a, b, c, p[ 4] + 0x4BDECFA9_u32, 11);
	MD5Step(F3, c, d, a, b, p[ 7] + 0xF6BB4B60_u32, 16);
	MD5Step(F3, b, c, d, a, p[10] + 0xBEBFBC70_u32, 23);
	MD5Step(F3, a, b, c, d, p[13] + 0x289B7EC6_u32,  4);
	MD5Step(F3, d, a, b, c, p[ 0] + 0xEAA127FA_u32, 11);
	MD5Step(F3, c, d, a, b, p[ 3] + 0xD4EF3085_u32, 16);
	MD5Step(F3, b, c, d, a, p[ 6] + 0x04881D05_u32, 23);
	MD5Step(F3, a, b, c, d, p[ 9] + 0xD9D4D039_u32,  4);
	MD5Step(F3, d, a, b, c, p[12] + 0xE6DB99E5_u32, 11);
	MD5Step(F3, c, d, a, b, p[15] + 0x1FA27CF8_u32, 16);
	MD5Step(F3, b, c, d, a, p[ 2] + 0xC4AC5665_u32, 23);

	MD5Step(F4, a, b, c, d, p[ 0] + 0xF4292244_u32,  6);
	MD5Step(F4, d, a, b, c, p[ 7] + 0x432AFF97_u32, 10);
	MD5Step(F4, c, d, a, b, p[14] + 0xAB9423A7_u32, 15);
	MD5Step(F4, b, c, d, a, p[ 5] + 0xFC93A039_u32, 21);
	MD5Step(F4, a, b, c, d, p[12] + 0x655B59C3_u32,  6);
	MD5Step(F4, d, a, b, c, p[ 3] + 0x8F0CCC92_u32, 10);
	MD5Step(F4, c, d, a, b, p[10] + 0xFFEFF47D_u32, 15);
	MD5Step(F4, b, c, d, a, p[ 1] + 0x85845DD1_u32, 21);
	MD5Step(F4, a, b, c, d, p[ 8] + 0x6FA87E4F_u32,  6);
	MD5Step(F4, d, a, b, c, p[15] + 0xFE2CE6E0_u32, 10);
	MD5Step(F4, c, d, a, b, p[ 6] + 0xA3014314_u32, 15);
	MD5Step(F4, b, c, d, a, p[13] + 0x4E0811A1_u32, 21);
	MD5Step(F4, a, b, c, d, p[ 4] + 0xF7537E82_u32,  6);
	MD5Step(F4, d, a, b, c, p[11] + 0xBD3AF235_u32, 10);
	MD5Step(F4, c, d, a, b, p[ 2] + 0x2AD7D2BB_u32, 15);
	MD5Step(F4, b, c, d, a, p[ 9] + 0xEB86D391_u32, 21);

	MD5[0] += a;
	MD5[1] += b;
	MD5[2] += c;
	MD5[3] += d;
}

}


MD5Value CalcMD5(const uint8_t *pData, size_t DataSize) noexcept
{
	const uint8_t *pSrc = pData;
	MD5Value MD5;
	const uint64_t BitsSize = (uint64_t)DataSize << 3;
	uint32_t BlockData[16];

	MD5.Value32[0] = 0x67452301_u32;
	MD5.Value32[1] = 0xEFCDAB89_u32;
	MD5.Value32[2] = 0x98BADCFE_u32;
	MD5.Value32[3] = 0x10325476_u32;

	while (DataSize >= 64) {
		std::memcpy(BlockData, pSrc, 64);
		MD5Transform(MD5.Value32, BlockData);
		pSrc += 64;
		DataSize -= 64;
	}

	size_t PaddingSize;
	uint32_t PaddingData[16];
	uint8_t *p;

	PaddingSize = DataSize & 0x3F;
	std::memcpy(PaddingData, pSrc, DataSize);
	p = reinterpret_cast<uint8_t *>(PaddingData) + PaddingSize;
	*p++ = 0x80;
	PaddingSize = 64 - 1 - PaddingSize;
	if (PaddingSize < 8) {
		std::memset(p, 0, PaddingSize);
		MD5Transform(MD5.Value32, PaddingData);
		std::memset(PaddingData, 0, 56);
	} else {
		std::memset(p, 0, PaddingSize - 8);
	}
	std::memcpy(PaddingData + 14, &BitsSize, 8);
	MD5Transform(MD5.Value32, PaddingData);

	return MD5;
}


}	// namespace LibISDB
