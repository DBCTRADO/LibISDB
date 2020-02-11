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
 @file   Utilities.hpp
 @brief  ユーティリティ
 @author DBCTRADO
*/


#ifndef LIBISDB_UTILITIES_H
#define LIBISDB_UTILITIES_H


namespace LibISDB
{

#if defined(LIBISDB_X86) || defined(LIBISDB_X64)
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define LIBISDB_BYTE_SWAP_INTRINSICS
#pragma intrinsic(_byteswap_ushort, _byteswap_ulong, _byteswap_uint64)
	LIBISDB_FORCE_INLINE uint16_t ByteSwap16(uint16_t v) noexcept { return _byteswap_ushort(v); }
	LIBISDB_FORCE_INLINE uint32_t ByteSwap32(uint32_t v) noexcept { return _byteswap_ulong(v); }
	LIBISDB_FORCE_INLINE uint64_t ByteSwap64(uint64_t v) noexcept { return _byteswap_uint64(v); }
#define LIBISDB_ROTATE_INTRINSICS
#pragma intrinsic(_lrotl, _lrotr)
	LIBISDB_FORCE_INLINE uint32_t RotateLeft32(uint32_t v, int shift) noexcept { return _lrotl(v, shift); }
	LIBISDB_FORCE_INLINE uint32_t RotateRight32(uint32_t v, int shift) noexcept { return _lrotr(v, shift); }
#elif defined(__GNUC__) || defined(__clang__)
#define LIBISDB_BYTE_SWAP_INTRINSICS
	LIBISDB_FORCE_INLINE uint16_t ByteSwap16(uint16_t v) noexcept { return __builtin_bswap16(v); }
	LIBISDB_FORCE_INLINE uint32_t ByteSwap32(uint32_t v) noexcept { return __builtin_bswap32(v); }
	LIBISDB_FORCE_INLINE uint64_t ByteSwap64(uint64_t v) noexcept { return __builtin_bswap64(v); }
#endif
#endif

#ifndef LIBISDB_BYTE_SWAP_INTRINSICS

	LIBISDB_FORCE_INLINE uint16_t ByteSwap16(uint16_t v) noexcept
	{
		return static_cast<uint16_t>((v << 8) | (v >> 8));
	}

	LIBISDB_FORCE_INLINE uint32_t ByteSwap32(uint32_t v) noexcept
	{
		return ((v << 24))
		     | ((v <<  8) & 0x00FF0000_u32)
		     | ((v >>  8) & 0x0000FF00_u32)
		     | ((v >> 24));
	}

	LIBISDB_FORCE_INLINE uint64_t ByteSwap64(uint64_t v) noexcept
	{
		return ((v << 56))
		     | ((v << 40) & 0x00FF000000000000_u64)
		     | ((v << 24) & 0x0000FF0000000000_u64)
		     | ((v <<  8) & 0x000000FF00000000_u64)
		     | ((v >>  8) & 0x00000000FF000000_u64)
		     | ((v >> 24) & 0x0000000000FF0000_u64)
		     | ((v >> 40) & 0x000000000000FF00_u64)
		     | ((v >> 56));
	}

#endif	// ifndef LIBISDB_BYTE_SWAP_INTRINSICS

#ifndef LIBISDB_ROTATE_INTRINSICS

	LIBISDB_FORCE_INLINE uint32_t RotateLeft32(uint32_t v, int shift) noexcept
	{
		return (v << shift) | (v >> (32 - shift));
	}

	LIBISDB_FORCE_INLINE uint32_t RotateRight32(uint32_t v, int shift) noexcept
	{
		return (v >> shift) | (v << (32 - shift));
	}

#endif	// ifndef LIBISDB_ROTATE_INTRINSICS

	LIBISDB_FORCE_INLINE uint16_t Load16(const void *p)
	{
#if (defined(LIBISDB_X86) || defined(LIBISDB_X64)) && defined(LIBISDB_BYTE_SWAP_INTRINSICS)
		return ByteSwap16(*static_cast<const uint16_t *>(p));
#else
		const uint8_t *q = static_cast<const uint8_t *>(p);
		return static_cast<uint16_t>((q[0] << 8) | q[1]);
#endif
	}

	LIBISDB_FORCE_INLINE uint32_t Load24(const void *p)
	{
		const uint8_t *q = static_cast<const uint8_t *>(p);
		return (static_cast<uint32_t>(q[0]) << 16)
		     | (static_cast<uint32_t>(q[1]) <<  8)
		     | (static_cast<uint32_t>(q[2]));
	}

	LIBISDB_FORCE_INLINE uint32_t Load32(const void *p)
	{
#if (defined(LIBISDB_X86) || defined(LIBISDB_X64)) && defined(LIBISDB_BYTE_SWAP_INTRINSICS)
		return ByteSwap32(*static_cast<const uint32_t *>(p));
#else
		const uint8_t *q = static_cast<const uint8_t *>(p);
		return (static_cast<uint32_t>(q[0]) << 24)
		     | (static_cast<uint32_t>(q[1]) << 16)
		     | (static_cast<uint32_t>(q[2]) <<  8)
		     | (static_cast<uint32_t>(q[3]));
#endif
	}

	LIBISDB_FORCE_INLINE void Store16(void *p, uint16_t v)
	{
#if (defined(LIBISDB_X86) || defined(LIBISDB_X64)) && defined(LIBISDB_BYTE_SWAP_INTRINSICS)
		*static_cast<uint16_t *>(p) = ByteSwap16(v);
#else
		uint8_t *q = static_cast<uint8_t *>(p);
		q[0] = static_cast<uint8_t>(v >> 8);
		q[1] = static_cast<uint8_t>(v & 0xFF);
#endif
	}

	LIBISDB_FORCE_INLINE void Store24(void *p, uint32_t v)
	{
		uint8_t *q = static_cast<uint8_t *>(p);
		q[0] = static_cast<uint8_t>((v >> 16) & 0xFF);
		q[1] = static_cast<uint8_t>((v >>  8) & 0xFF);
		q[2] = static_cast<uint8_t>( v        & 0xFF);
	}

	LIBISDB_FORCE_INLINE void Store32(void *p, uint32_t v)
	{
#if (defined(LIBISDB_X86) || defined(LIBISDB_X64)) && defined(LIBISDB_BYTE_SWAP_INTRINSICS)
		*static_cast<uint32_t *>(p) = ByteSwap32(v);
#else
		uint8_t *q = static_cast<uint8_t *>(p);
		q[0] = static_cast<uint8_t>( v >> 24);
		q[1] = static_cast<uint8_t>((v >> 16) & 0xFF);
		q[2] = static_cast<uint8_t>((v >>  8) & 0xFF);
		q[3] = static_cast<uint8_t>( v        & 0xFF);
#endif
	}

	template<typename T> constexpr T RoundOff(const T &v, const T &r) { return (v + (r / 2)) / r * r; }
	template<typename T> constexpr T RoundUp(const T &v, const T &r) { return (v + (r - 1)) / r * r; }
	template<typename T> constexpr T RoundDown(const T &v, const T &r) { return v / r * r; }

	constexpr uint8_t MakeBCD(unsigned int Value) noexcept
	{
		return static_cast<uint8_t>(((Value / 10) << 4) | (Value % 10));
	}

	constexpr uint8_t GetBCD(uint8_t Value) noexcept
	{
		return ((Value >> 4) * 10) + (Value & 0x0F);
	}

	uint32_t GetBCD(const uint8_t *pData, size_t NibbleLength);

}	// namespace LibISDB


#endif	// ifndef LIBISDB_UTILITIES_H
