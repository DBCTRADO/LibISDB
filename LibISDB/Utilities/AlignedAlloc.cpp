/*
  LibISDB
  Copyright(c) 2017-2019 DBCTRADO

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
 @file   AlignedAlloc.cpp
 @brief  アラインメント指定メモリ確保
 @author DBCTRADO
*/


#include "../LibISDBBase.hpp"
#include "AlignedAlloc.hpp"
#include "../Base/Debug.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


#if defined(_MSC_VER)


void * AlignedAlloc(size_t Size, size_t Align, size_t Offset)
{
	return ::_aligned_offset_malloc(Size, Align, Offset);
}


void * AlignedRealloc(void *pBuffer, size_t Size, size_t Align, size_t Offset)
{
	return ::_aligned_offset_realloc(pBuffer, Size, Align, Offset);
}


void AlignedFree(void *pBuffer)
{
	::_aligned_free(pBuffer);
}


#else	// _MSC_VER


namespace
{

const size_t MAX_ALIGNMENT = 256;
const unsigned long ALIGNED_MEMORY_SIGNATURE = 0x416C496EUL;

struct AlignedMemoryInfo {
	unsigned long Signature;
	size_t Size;
	void *pBase;
};

inline AlignedMemoryInfo * GetAlignedMemoryInfo(void *pBuffer)
{
	return reinterpret_cast<AlignedMemoryInfo *>(
		reinterpret_cast<uintptr_t>(static_cast<uint8_t *>(pBuffer) - sizeof(AlignedMemoryInfo)) & ~(alignof(AlignedMemoryInfo) - 1));
}

}


void * AlignedAlloc(size_t Size, size_t Align, size_t Offset)
{
	if (LIBISDB_TRACE_ERROR_IF((Size == 0) || (Offset >= Size) || (Offset >= Align)))
		return nullptr;

	if (LIBISDB_TRACE_ERROR_IF((Align == 0) || (Align > MAX_ALIGNMENT) || ((Align & (Align -1)) != 0)))
		return nullptr;

	if (Align < alignof(std::max_align_t))
		Align = alignof(std::max_align_t);

	if (LIBISDB_TRACE_ERROR_IF(Size > std::numeric_limits<size_t>::max() - sizeof(AlignedMemoryInfo) - Align))
		return nullptr;

	void *pBase = std::malloc(Size + Align + sizeof(AlignedMemoryInfo));
	if (pBase == nullptr)
		return nullptr;

	void *pAligned =
		reinterpret_cast<void *>(
			(reinterpret_cast<uintptr_t>(static_cast<uint8_t *>(pBase) + Align + Offset + sizeof(AlignedMemoryInfo)) & ~(Align - 1)) - Offset);

	AlignedMemoryInfo *pInfo = GetAlignedMemoryInfo(pAligned);
	pInfo->Signature = ALIGNED_MEMORY_SIGNATURE;
	pInfo->Size = Size;
	pInfo->pBase = pBase;

	return pAligned;
}


void * AlignedRealloc(void *pBuffer, size_t Size, size_t Align, size_t Offset)
{
	if (pBuffer == nullptr)
		return AlignedAlloc(Size, Align, Offset);

	AlignedMemoryInfo *pInfo = GetAlignedMemoryInfo(pBuffer);

	if (pInfo->Signature != ALIGNED_MEMORY_SIGNATURE) {
		LIBISDB_TRACE_ERROR(LIBISDB_STR("AlignedRealloc() : Memory not allocated by AlignedAlloc() [%p]\n"), pBuffer);
		return nullptr;
	}

	if (Size == 0) {
		AlignedFree(pBuffer);
		return nullptr;
	}

	void *pNewBuffer = AlignedAlloc(Size, Align, Offset);
	if (pNewBuffer == nullptr)
		return nullptr;

	std::memcpy(pNewBuffer, pBuffer, std::min(pInfo->Size, Size));
	AlignedFree(pBuffer);

	return pNewBuffer;
}


void AlignedFree(void *pBuffer)
{
	if (pBuffer == nullptr)
		return;

	AlignedMemoryInfo *pInfo = GetAlignedMemoryInfo(pBuffer);
	if (pInfo->Signature != ALIGNED_MEMORY_SIGNATURE) {
		LIBISDB_TRACE_ERROR(LIBISDB_STR("AlignedFree() : Memory not allocated by AlignedAlloc() [%p]\n"), pBuffer);
		return;
	}

#ifdef LIBISDB_DEBUG
	pInfo->Signature = 0;
#endif

	std::free(pInfo->pBase);
}


#endif	// !def _MSC_VER


}	// namespace LibISDB
