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
 @file   EnumFlags.hpp
 @brief  enum class をフラグに利用する
 @author DBCTRADO
*/


#ifndef LIBISDB_ENUM_FLAGS_H
#define LIBISDB_ENUM_FLAGS_H


namespace LibISDB
{

#define LIBISDB_ENUM_FLAGS_TRAILER_(mask) \
	AllFlags_ = (mask)
#define LIBISDB_ENUM_FLAGS_TRAILER \
	LastPlusOne_, \
	Last_ = LastPlusOne_ - 1, \
	LIBISDB_ENUM_FLAGS_TRAILER_(Last_ >= 1 ? Last_ | (Last_ - 1) : 0)

	namespace Concept
	{

		template<typename T> concept EnumFlags = requires {
			requires EnumClass<T>;
			requires std::same_as<T, decltype(T::AllFlags_)>;
		};

		template<typename T> concept EnumClassNotFlags = requires {
			requires EnumClass<T>;
			requires !EnumFlags<T>;
		};

	}	// namespace Concept

	inline namespace EnumFlags
	{

		template<Concept::Enum T> constexpr T EnumAnd(T v1, T v2) {
			return static_cast<T>(
				static_cast<std::underlying_type_t<T>>(v1) &
				static_cast<std::underlying_type_t<T>>(v2));
		}
		template<Concept::Enum T> constexpr T EnumOr(T v1, T v2) {
			return static_cast<T>(
				static_cast<std::underlying_type_t<T>>(v1) |
				static_cast<std::underlying_type_t<T>>(v2));
		}
		template<Concept::Enum T> constexpr T EnumXor(T v1, T v2) {
			return static_cast<T>(
				static_cast<std::underlying_type_t<T>>(v1) ^
				static_cast<std::underlying_type_t<T>>(v2));
		}
		template<Concept::Enum T> constexpr T EnumNot(T v) {
			return static_cast<T>(
				~static_cast<std::underlying_type_t<T>>(v));
		}

		template<Concept::EnumFlags T> constexpr T operator | (T lhs, T rhs) {
			return EnumOr(lhs, rhs);
		}
		template<Concept::EnumFlags T> constexpr T operator & (T lhs, T rhs) {
			return EnumAnd(lhs, rhs);
		}
		template<Concept::EnumFlags T> constexpr T operator ^ (T lhs, T rhs) {
			return EnumXor(lhs, rhs);
		}
		template<Concept::EnumFlags T> constexpr T operator ~ (T rhs) {
			return EnumNot(rhs);
		}
		template<Concept::EnumFlags T> constexpr T& operator |= (T &lhs, T rhs) {
			return lhs = EnumOr(lhs, rhs);
		}
		template<Concept::EnumFlags T> constexpr T& operator &= (T &lhs, T rhs) {
			return lhs = EnumAnd(lhs, rhs);
		}
		template<Concept::EnumFlags T> constexpr T& operator ^= (T &lhs, T rhs) {
			return lhs = EnumXor(lhs, rhs);
		}
		template<Concept::EnumFlags T> constexpr bool operator ! (T rhs) {
			return !static_cast<std::underlying_type_t<T>>(rhs);
		}

	}	// namespace EnumFlags

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ENUM_FLAGS_H
