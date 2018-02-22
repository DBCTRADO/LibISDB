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
 @file   EPGDatabaseFilter.hpp
 @brief  番組情報フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_EPG_DATABASE_FILTER_H
#define LIBISDB_EPG_DATABASE_FILTER_H


#include "FilterBase.hpp"
#include "../EPG/EPGDatabase.hpp"
#include "../TS/PIDMap.hpp"
#include "../TS/Tables.hpp"
#include <memory>


namespace LibISDB
{

	/** 番組情報フィルタクラス */
	class EPGDatabaseFilter
		: public SingleIOFilter
		, protected EPGDatabase::EventListener
	{
	public:
		EPGDatabaseFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("EPGDatabaseFilter"); }

	// FilterBase
		void Reset() override;

	// SingleIOFilter
		bool ProcessData(DataStream *pData) override;

	// EPGDatabaseFilter
		void SetEPGDatabase(EPGDatabase *pDatabase);
		EPGDatabase * GetEPGDatabase() const;

	protected:
		PIDMapManager m_PIDMapManager;
		EPGDatabase *m_pEPGDatabase;
		bool m_ResetTable;

	private:
	// EPGDatabase::EventListener
		void OnScheduleStatusReset(
			EPGDatabase *pEPGDatabase,
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID) override;

		void OnEITSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnTOTSection(const PSITableBase *pTable, const PSISection *pSection);
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_EPG_DATABASE_FILTER_H
