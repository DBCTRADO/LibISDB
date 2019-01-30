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
 @file   AlignedAlloc.hpp
 @brief  アラインメント指定メモリ確保
 @author DBCTRADO
*/


#ifndef LIBISDB_ALIGNED_ALLOC_H
#define LIBISDB_ALIGNED_ALLOC_H


namespace LibISDB
{

	[[nodiscard]] void * AlignedAlloc(size_t Size, size_t Align, size_t Offset = 0);
	[[nodiscard]] void * AlignedRealloc(void *pBuffer, size_t Size, size_t Align, size_t Offset = 0);
	void AlignedFree(void *pBuffer);

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ALIGNED_ALLOC_H
