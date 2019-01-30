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
 @file   Sort.hpp
 @brief  整列
 @author DBCTRADO
*/


#ifndef LIBISDB_SORT_H
#define LIBISDB_SORT_H


#include <functional>
#include <iterator>
#include <utility>


namespace LibISDB
{

	// 挿入ソート
	template<typename T, typename TCompare>
	void InsertionSort(T Begin, T End, TCompare Compare)
	{
		if (Begin == End)
			return;

		T it = Begin;
		for (++it; it != End; ++it) {
			T itPrev = it;
			--itPrev;
			if (Compare(*it, *itPrev)) {
				auto Temp = std::move(*it);
				T it2 = it;
				do {
					*it2 = std::move(*itPrev);
					--it2;
				} while ((it2 != Begin) && Compare(Temp, *--itPrev));
				*it2 = std::move(Temp);
			}
		}
	}

	template<typename T>
	inline void InsertionSort(T Begin, T End)
	{
		InsertionSort(Begin, End, std::less<typename std::iterator_traits<T>::value_type>());
	}

	template<typename T, typename TCompare>
	inline void InsertionSort(T &Array, TCompare Compare)
	{
		InsertionSort(std::begin(Array), std::end(Array), Compare);
	}

	template<typename T>
	inline void InsertionSort(T &Array)
	{
		InsertionSort(std::begin(Array), std::end(Array));
	}

}	// namespace LibISDB


#endif	// ifndef LIBISDB_SORT_H
