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
 @file   BitstreamReader.cpp
 @brief  ビット列の読み込み
 @author DBCTRADO
*/


#include "../LibISDBBase.hpp"
#include "BitstreamReader.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


BitstreamReader::BitstreamReader(const uint8_t *pBits, size_t ByteSize)
	: m_pBits(pBits)
	, m_BitSize(ByteSize << 3)
	, m_BitPos(0)
	, m_IsOverrun(false)
{
}


uint32_t BitstreamReader::GetBits(size_t Bits) noexcept
{
	if (m_BitSize - m_BitPos < Bits) {
		MarkOverrun();
		return 0;
	}

	const uint8_t *p = &m_pBits[m_BitPos >> 3];
	int Shift = static_cast<int>(7 - (m_BitPos & 7));
	uint32_t Value = 0;

	m_BitPos += Bits;

	while (Bits-- > 0) {
		Value <<= 1;
		Value |= (*p >> Shift) & 0x01;
		Shift--;
		if (Shift < 0) {
			Shift = 7;
			p++;
		}
	}

	return Value;
}


bool BitstreamReader::GetFlag() noexcept
{
	return GetBits(1) != 0;
}


int BitstreamReader::GetUE_V() noexcept
{
	int Info;
	int Length = GetVLCSymbol(&Info);

	if (Length < 0)
		return -1;
	return (1 << (Length >> 1)) + Info - 1;
}


int BitstreamReader::GetSE_V() noexcept
{
	int Info;
	int Length = GetVLCSymbol(&Info);

	if (Length < 0)
		return -1;
	unsigned int n = (1U << (Length >> 1)) + Info - 1;
	int Value = (n + 1) >> 1;
	return (n & 0x01) != 0 ? Value : -Value;
}


bool BitstreamReader::Skip(size_t Bits) noexcept
{
	if (m_BitSize - m_BitPos < Bits) {
		MarkOverrun();
		return false;
	}

	m_BitPos += Bits;

	return true;
}


int BitstreamReader::GetVLCSymbol(int *pInfo) noexcept
{
	const uint8_t *p = &m_pBits[m_BitPos >> 3];
	const uint8_t *pEnd = m_pBits + (m_BitSize >> 3);
	int Shift = static_cast<int>(7 - (m_BitPos & 7));
	size_t BitCount = 1;
	int Length = 0;

	while (((*p >> Shift) & 0x01) == 0) {
		Length++;
		BitCount++;
		Shift--;
		if (Shift < 0) {
			Shift = 7;
			p++;
			if (p == pEnd) {
				MarkOverrun();
				return -1;
			}
		}
	}
	BitCount += Length;
	if (m_BitSize - m_BitPos < BitCount) {
		MarkOverrun();
		return -1;
	}

	int Info = 0;
	while (Length-- > 0) {
		Shift--;
		if (Shift < 0) {
			Shift = 7;
			p++;
		}
		Info <<= 1;
		Info |= (*p >> Shift) & 0x01;
	}

	*pInfo = Info;
	m_BitPos += BitCount;

	return static_cast<int>(BitCount);
}


void BitstreamReader::MarkOverrun() noexcept
{
	m_BitPos = m_BitSize;
	m_IsOverrun = true;
}


}	// namespace LibISDB
