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
 @file   EnumFlags.hpp
 @brief  enum class をフラグに利用する
 @author DBCTRADO
*/


#ifndef LIBISDB_ENUM_FLAGS_H
#define LIBISDB_ENUM_FLAGS_H


namespace LibISDB
{

	inline namespace EnumFlags
	{

		template<typename T> constexpr T EnumAnd(T v1, T v2) {
			return static_cast<T>(
				static_cast<typename std::underlying_type<T>::type>(v1) &
				static_cast<typename std::underlying_type<T>::type>(v2));
		}
		template<typename T> constexpr T EnumOr(T v1, T v2) {
			return static_cast<T>(
				static_cast<typename std::underlying_type<T>::type>(v1) |
				static_cast<typename std::underlying_type<T>::type>(v2));
		}
		template<typename T> constexpr T EnumXor(T v1, T v2) {
			return static_cast<T>(
				static_cast<typename std::underlying_type<T>::type>(v1) ^
				static_cast<typename std::underlying_type<T>::type>(v2));
		}
		template<typename T> constexpr T EnumNot(T v) {
			return static_cast<T>(
				~static_cast<typename std::underlying_type<T>::type>(v));
		}

		template<typename T> constexpr bool EnableEnumFlags(T) { return false; }
		template<typename T> constexpr std::enable_if_t<EnableEnumFlags(T()), T> operator | (T lhs, T rhs) {
			return EnumOr(lhs, rhs);
		}
		template<typename T> constexpr std::enable_if_t<EnableEnumFlags(T()), T> operator & (T lhs, T rhs) {
			return EnumAnd(lhs, rhs);
		}
		template<typename T> constexpr std::enable_if_t<EnableEnumFlags(T()), T> operator ^ (T lhs, T rhs) {
			return EnumXor(lhs, rhs);
		}
		template<typename T> constexpr std::enable_if_t<EnableEnumFlags(T()), T> operator ~ (T rhs) {
			return EnumNot(rhs);
		}
		template<typename T> constexpr std::enable_if_t<EnableEnumFlags(T()), T&> operator |= (T &lhs, T rhs) {
			return lhs = EnumOr(lhs, rhs);
		}
		template<typename T> constexpr std::enable_if_t<EnableEnumFlags(T()), T&> operator &= (T &lhs, T rhs) {
			return lhs = EnumAnd(lhs, rhs);
		}
		template<typename T> constexpr std::enable_if_t<EnableEnumFlags(T()), T&> operator ^= (T &lhs, T rhs) {
			return lhs = EnumXor(lhs, rhs);
		}
		template<typename T, typename T2 = std::enable_if_t<EnableEnumFlags(T())>> constexpr bool operator ! (T rhs) {
			return !static_cast<typename std::underlying_type<T>::type>(rhs);
		}

#define LIBISDB_ENUM_FLAGS(type) constexpr bool EnableEnumFlags(type) { return true; }

	}	// namespace EnumFlags

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ENUM_FLAGS_H
