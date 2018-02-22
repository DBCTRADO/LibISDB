/*
  LibISDB
  Copyright(c) 2017-2018 DBCTRADO

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
 @file   DataStorageManager.hpp
 @brief  データストレージの管理
 @author DBCTRADO
*/


#ifndef LIBISDB_DATA_STORAGE_MANAGER_H
#define LIBISDB_DATA_STORAGE_MANAGER_H


#include "DataStorage.hpp"


namespace LibISDB
{

	/** データストレージ管理基底クラス */
	class DataStorageManager
	{
	public:
		virtual ~DataStorageManager() = default;

		virtual DataStorage * CreateDataStorage() = 0;
	};

	/** メモリデータストレージ管理クラス */
	class MemoryDataStorageManager : public DataStorageManager
	{
	public:
	// DataStorageManager
		DataStorage * CreateDataStorage() override;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DATA_STORAGE_MANAGER_H
