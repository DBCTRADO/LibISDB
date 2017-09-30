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
 @file   TeeFilter.hpp
 @brief  2分配フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_TEE_FILTER_H
#define LIBISDB_TEE_FILTER_H


#include "FilterBase.hpp"


namespace LibISDB
{

	/** 2分配フィルタクラス */
	class TeeFilter
		: public MultiOutputFilter<2>
	{
	public:
	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TeeFilter"); }

	// FilterSink
		bool ReceiveData(DataStream *pData) override;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TEE_FILTER_H
