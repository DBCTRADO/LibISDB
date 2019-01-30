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
 @file   EPGDatabaseFilter.cpp
 @brief  番組情報フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "EPGDatabaseFilter.hpp"
#include <algorithm>
#include "../TS/Tables.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


EPGDatabaseFilter::EPGDatabaseFilter()
	: m_pEPGDatabase(nullptr)
{
	Reset();
}


void EPGDatabaseFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_PIDMapManager.UnmapAllTargets();

	// H-EIT
	m_PIDMapManager.MapTarget(PID_HEIT, PSITableBase::CreateWithHandler<EITPfScheduleTable>(&EPGDatabaseFilter::OnEITSection, this));

	// L-EIT
	m_PIDMapManager.MapTarget(PID_LEIT, PSITableBase::CreateWithHandler<EITPfScheduleTable>(&EPGDatabaseFilter::OnEITSection, this));

	// TOT
	m_PIDMapManager.MapTarget(PID_TOT, PSITableBase::CreateWithHandler<TOTTable>(&EPGDatabaseFilter::OnTOTSection, this));

	if (m_pEPGDatabase != nullptr)
		m_pEPGDatabase->ResetTOTTime();
}


bool EPGDatabaseFilter::ProcessData(DataStream *pData)
{
	if (pData->Is<TSPacket>())
		m_PIDMapManager.StorePacketStream(pData);

	return true;
}


void EPGDatabaseFilter::SetEPGDatabase(EPGDatabase *pDatabase)
{
	BlockLock Lock(m_FilterLock);

	if (m_pEPGDatabase != nullptr)
		m_pEPGDatabase->RemoveEventListener(this);

	m_pEPGDatabase = pDatabase;

	if (m_pEPGDatabase != nullptr)
		m_pEPGDatabase->AddEventListener(this);
}


EPGDatabase * EPGDatabaseFilter::GetEPGDatabase() const
{
	return m_pEPGDatabase;
}


void EPGDatabaseFilter::OnScheduleStatusReset(
	EPGDatabase *pEPGDatabase,
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID)
{
	m_ResetTable = true;
}


void EPGDatabaseFilter::OnEITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	if (m_pEPGDatabase != nullptr) {
		const EITPfScheduleTable *pEITScheduleTable = dynamic_cast<const EITPfScheduleTable *>(pTable);

		if (pEITScheduleTable != nullptr) {
			const EITTable *pEITTable = pEITScheduleTable->GetLastUpdatedEITTable();

			if (pEITTable != nullptr) {
				m_ResetTable = false;

				m_pEPGDatabase->UpdateSection(pEITScheduleTable, pEITTable);

				if (m_ResetTable) {
					const uint16_t NetworkID = pEITTable->GetOriginalNetworkID();
					const uint16_t TransportStreamID = pEITTable->GetTransportStreamID();
					const uint16_t ServiceID = pEITTable->GetServiceID();
					EITPfScheduleTable *pTable;

					pTable = m_PIDMapManager.GetMapTarget<EITPfScheduleTable>(PID_HEIT);
					if (pTable != nullptr)
						pTable->ResetScheduleService(NetworkID, TransportStreamID, ServiceID);

					pTable = m_PIDMapManager.GetMapTarget<EITPfScheduleTable>(PID_LEIT);
					if (pTable != nullptr)
						pTable->ResetScheduleService(NetworkID, TransportStreamID, ServiceID);
				}
			}
		}
	}
}


void EPGDatabaseFilter::OnTOTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	if (m_pEPGDatabase != nullptr) {
		const TOTTable *pTOTTable = dynamic_cast<const TOTTable *>(pTable);

		if (pTOTTable != nullptr)
			m_pEPGDatabase->UpdateTOT(pTOTTable);
	}
}


}	// namespace LibISDB
