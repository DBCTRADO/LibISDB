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
 @file   Hasher.hpp
 @brief  ハッシュ計算
 @author DBCTRADO
*/


#ifndef LIBISDB_HASHER_H
#define LIBISDB_HASHER_H


namespace LibISDB
{

	/** ハッシュ計算クラス */
	template<typename T> class Hasher
	{
	public:
		void Reset() noexcept { m_Hash = T::InitialValue; }
		typename T::ValueType Get() const noexcept { return m_Hash; }
		typename T::ValueType Calc(const uint8_t *pData, size_t DataSize) noexcept {
			return m_Hash = T::Calc(pData, DataSize, m_Hash);
		}

	private:
		typename T::ValueType m_Hash = T::InitialValue;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_HASHER_H
