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
 @file   BitTable.hpp
 @brief  ビットテーブル
 @author DBCTRADO
*/


#ifndef LIBISDB_BIT_TABLE_H
#define LIBISDB_BIT_TABLE_H


#include "../Base/Debug.hpp"
#include <memory>


namespace LibISDB
{

	/** ビットテーブルクラス */
	class BitTable
	{
	public:
		typedef uint_fast32_t ScalarType;
		static constexpr size_t ScalarBits = std::numeric_limits<ScalarType>::digits;

		BitTable() = default;

		BitTable(size_t Size)
		{
			SetSize(Size);
		}

		BitTable(const BitTable &Src)
		{
			*this = Src;
		}

		BitTable(BitTable &&Src) noexcept
		{
			*this = std::move(Src);
		}

		BitTable & operator = (const BitTable &rhs)
		{
			if (&rhs != this) {
				SetSize(rhs.m_Size);
				if (m_Table)
					std::memcpy(m_Table.get(), rhs.m_Table.get(), (m_Size + 7) >> 3);
			}
			return *this;
		}

		BitTable & operator = (BitTable &&rhs) noexcept
		{
			if (&rhs != this) {
				m_Size = rhs.m_Size;
				m_Bits = rhs.m_Bits;
				m_Table = std::move(rhs.m_Table);
				rhs.m_Size = 0;
			}
			return *this;
		}

		void SetSize(size_t Size)
		{
			Clear();
			if (Size > ScalarBits)
				m_Table.reset(new uint8_t[(Size + 7) >> 3]);
			m_Size = Size;
		}

		void Clear() noexcept
		{
			m_Size = 0;
			m_Table.reset();
		}

		bool operator [] (size_t Index) const
		{
			LIBISDB_ASSERT(Index < m_Size);

			if (m_Size <= ScalarBits)
				return ((m_Bits >> Index) & 1) != 0;
			else
				return ((m_Table[Index >> 3] >> (Index & 0x07)) & 1) != 0;
		}

		void Set() noexcept { Fill(true); }

		void Set(size_t Index)
		{
			LIBISDB_ASSERT(Index < m_Size);

			if (m_Size <= ScalarBits)
				m_Bits |= 1_u32 << Index;
			else
				m_Table[Index >> 3] |= 1 << (Index & 0x07);
		}

		void Reset() noexcept { Fill(false); }

		void Reset(size_t Index)
		{
			LIBISDB_ASSERT(Index < m_Size);

			if (m_Size <= ScalarBits)
				m_Bits &= ~(1_u32 << Index);
			else
				m_Table[Index >> 3] &= ~(1 << (Index & 0x07));
		}

	private:
		void Fill(bool Bit) noexcept
		{
			if (m_Size <= ScalarBits)
				m_Bits = Bit ? ~(ScalarType)0 : (ScalarType)0;
			else
				std::memset(m_Table.get(), Bit ? 0xFF : 0x00, (m_Size + 7) >> 3);
		}

		size_t m_Size = 0;
		uint_fast32_t m_Bits;
		std::unique_ptr<uint8_t[]> m_Table;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_BIT_TABLE_H
