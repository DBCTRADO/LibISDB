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
 @file   BitstreamReader.hpp
 @brief  ビット列の読み込み
 @author DBCTRADO
*/


#ifndef LIBISDB_BITSTREAM_READER_H
#define LIBISDB_BITSTREAM_READER_H


namespace LibISDB
{

	/** ビット列読み込みクラス */
	class BitstreamReader
	{
	public:
		BitstreamReader(const uint8_t *pBits, size_t ByteSize);

		size_t GetPos() const noexcept { return m_BitPos; }
		uint32_t GetBits(size_t Bits) noexcept;
		bool GetFlag() noexcept;
		int GetUE_V() noexcept;
		int GetSE_V() noexcept;
		bool Skip(size_t Bits) noexcept;
		bool IsOverrun() const noexcept { return m_IsOverrun; }

	protected:
		int GetVLCSymbol(int *pInfo) noexcept;
		void MarkOverrun() noexcept;

		const uint8_t *m_pBits;
		size_t m_BitSize;
		size_t m_BitPos;
		bool m_IsOverrun;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_BITSTREAM_READER_H
