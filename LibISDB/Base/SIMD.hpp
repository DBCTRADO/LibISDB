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
 @file   SIMD.hpp
 @brief  SIMD
 @author DBCTRADO
*/


#ifndef LIBISDB_SIMD_H
#define LIBISDB_SIMD_H


#include "DataBuffer.hpp"
#include "../Utilities/AlignedAlloc.hpp"

#ifdef LIBISDB_SSE_SUPPORT
#include <xmmintrin.h>
#endif
#ifdef LIBISDB_SSE2_SUPPORT
#include <emmintrin.h>
#endif


namespace LibISDB
{

	/** アラインメント指定データバッファクラス */
	template<size_t Align> class AlignedDataBuffer
		: public DataBuffer
	{
	public:
		~AlignedDataBuffer()
		{
			FreeBuffer();
		}

	protected:
		void * Allocate(size_t Size) override
		{
			return AlignedAlloc(Size, Align);
		}

		void Free(void *pBuffer) noexcept override
		{
			AlignedFree(pBuffer);
		}

		void * ReAllocate(void *pBuffer, size_t Size) override
		{
			return AlignedRealloc(pBuffer, Size, Align);
		}
	};

#if defined(LIBISDB_X86) || defined(LIBISDB_X64)
	typedef AlignedDataBuffer<16> SSEDataBuffer;

#if defined(LIBISDB_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || defined(__SSE2__)
#define LIBISDB_NATIVE_SSE2
#endif

#ifdef LIBISDB_NATIVE_SSE2
	constexpr bool IsSSE2Available() { return true; }
	constexpr bool IsSSE2Enabled() { return true; }
#else
	bool IsSSE2Available();
	bool IsSSE2Enabled();
#endif
	void SetSSE2Enabled(bool Enabled);
#endif

}	// namespace LibISDB


#endif	// ifndef LIBISDB_SIMD_H
