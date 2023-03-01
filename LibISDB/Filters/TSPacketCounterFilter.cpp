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
 @file   TSPacketCounterFilter.cpp
 @brief  TSパケットカウンタフィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "TSPacketCounterFilter.hpp"
#include "../TS/Tables.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


TSPacketCounterFilter::TSPacketCounterFilter()
	: m_ESPIDMapTarget(this)
	, m_TargetServiceID(SERVICE_ID_INVALID)
{
	Reset();
}


void TSPacketCounterFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_PIDMapManager.UnmapAllTargets();

	m_PIDMapManager.MapTarget(PID_PAT, PSITableBase::CreateWithHandler<PATTable>(&TSPacketCounterFilter::OnPATSection, this));

	m_ServiceList.clear();
	m_TargetServiceID = SERVICE_ID_INVALID;

	m_InputPacketCount = 0;
	m_ScrambledPacketCount = 0;

	m_VideoPID = PID_INVALID;
	m_AudioPID = PID_INVALID;
	m_VideoBitRate.Initialize();
	m_AudioBitRate.Initialize();
}


void TSPacketCounterFilter::SetActiveServiceID(uint16_t ServiceID)
{
	BlockLock Lock(m_FilterLock);

	if (m_TargetServiceID != ServiceID) {
		m_TargetServiceID = ServiceID;

		for (size_t i = 0; i < m_ServiceList.size(); i++) {
			if (m_ServiceList[i].ServiceID != ServiceID)
				UnmapServiceESs(i);
		}
		for (size_t i = 0; i < m_ServiceList.size(); i++) {
			if (m_ServiceList[i].ServiceID == ServiceID)
				MapServiceESs(i);
		}
	}
}


void TSPacketCounterFilter::SetActiveVideoPID(uint16_t PID, bool ServiceChanged)
{
	SetVideoPID(PID);
}


void TSPacketCounterFilter::SetActiveAudioPID(uint16_t PID, bool ServiceChanged)
{
	SetAudioPID(PID);
}


bool TSPacketCounterFilter::ProcessData(DataStream *pData)
{
	if (pData->Is<TSPacket>()) {
		do {
			TSPacket *pPacket = pData->Get<TSPacket>();

			++m_InputPacketCount;

			m_PIDMapManager.StorePacket(pPacket);

			if ((m_TargetServiceID == SERVICE_ID_INVALID) && pPacket->IsScrambled())
				++m_ScrambledPacketCount;
		} while (pData->Next());
	}

	return true;
}


unsigned long long TSPacketCounterFilter::GetInputPacketCount() const
{
	return m_InputPacketCount;
}


void TSPacketCounterFilter::ResetInputPacketCount()
{
	m_InputPacketCount = 0;
}


unsigned long long TSPacketCounterFilter::GetScrambledPacketCount() const
{
	return m_ScrambledPacketCount;
}


void TSPacketCounterFilter::ResetScrambledPacketCount()
{
	m_ScrambledPacketCount = 0;
}


void TSPacketCounterFilter::SetVideoPID(uint16_t PID)
{
	BlockLock Lock(m_FilterLock);

	m_VideoPID = PID;
}


void TSPacketCounterFilter::SetAudioPID(uint16_t PID)
{
	BlockLock Lock(m_FilterLock);

	m_AudioPID = PID;
}


unsigned long TSPacketCounterFilter::GetVideoBitRate() const
{
	BlockLock Lock(m_FilterLock);

	return m_VideoBitRate.GetBitRate();
}


unsigned long TSPacketCounterFilter::GetAudioBitRate() const
{
	BlockLock Lock(m_FilterLock);

	return m_AudioBitRate.GetBitRate();
}


int TSPacketCounterFilter::GetServiceIndexByID(uint16_t ServiceID) const
{
	int Index;

	for (Index = (int)m_ServiceList.size() - 1; Index >= 0; Index--) {
		if (m_ServiceList[Index].ServiceID == ServiceID)
			break;
	}

	return Index;
}


void TSPacketCounterFilter::MapServiceESs(size_t Index)
{
	for (const uint16_t PID : m_ServiceList[Index].ESPIDList)
		m_PIDMapManager.MapTarget(PID, &m_ESPIDMapTarget);
}


void TSPacketCounterFilter::UnmapServiceESs(size_t Index)
{
	for (const uint16_t PID : m_ServiceList[Index].ESPIDList)
		m_PIDMapManager.UnmapTarget(PID);
}


void TSPacketCounterFilter::OnPATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PATが更新された
	const PATTable *pPATTable = dynamic_cast<const PATTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPATTable == nullptr))
		return;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		UnmapServiceESs(i);
		m_PIDMapManager.UnmapTarget(m_ServiceList[i].PMTPID);
	}

	const int ProgramCount = pPATTable->GetProgramCount();
	m_ServiceList.clear();
	m_ServiceList.reserve(ProgramCount);

	for (int i = 0; i < ProgramCount; i++) {
		ServiceInfo Info;

		Info.ServiceID = pPATTable->GetProgramNumber(i);
		Info.PMTPID = pPATTable->GetPMTPID(i);

		m_ServiceList.push_back(Info);

		m_PIDMapManager.MapTarget(Info.PMTPID, PSITableBase::CreateWithHandler<PMTTable>(&TSPacketCounterFilter::OnPMTSection, this));
	}
}


void TSPacketCounterFilter::OnPMTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PMTが更新された
	const PMTTable *pPMTTable = dynamic_cast<const PMTTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPMTTable == nullptr))
		return;

	const uint16_t ServiceID = pPMTTable->GetProgramNumberID();
	const int ServiceIndex = GetServiceIndexByID(ServiceID);
	if (ServiceIndex < 0)
		return;

	ServiceInfo &Info = m_ServiceList[ServiceIndex];

	const int ESCount = pPMTTable->GetESCount();
	std::vector<uint16_t> ESPIDList;
	ESPIDList.resize(ESCount);
	for (int i = 0; i < ESCount; i++)
		ESPIDList[i] = pPMTTable->GetESPID(i);
	if (ESPIDList != Info.ESPIDList) {
		const bool IsTarget = (ServiceID == m_TargetServiceID);
		if (IsTarget)
			UnmapServiceESs(ServiceIndex);
		Info.ESPIDList = std::move(ESPIDList);
		if (IsTarget)
			MapServiceESs(ServiceIndex);
	}
}




TSPacketCounterFilter::ESPIDMapTarget::ESPIDMapTarget(TSPacketCounterFilter *pTSPacketCounter)
	: m_pTSPacketCounter(pTSPacketCounter)
{
}


bool TSPacketCounterFilter::ESPIDMapTarget::StorePacket(const TSPacket *pPacket)
{
	if (pPacket->IsScrambled())
		++(m_pTSPacketCounter->m_ScrambledPacketCount);

	const uint16_t PID = pPacket->GetPID();
	if (PID == m_pTSPacketCounter->m_VideoPID)
		m_pTSPacketCounter->m_VideoBitRate.Update(pPacket->GetPayloadSize());
	else if (PID == m_pTSPacketCounter->m_AudioPID)
		m_pTSPacketCounter->m_AudioBitRate.Update(pPacket->GetPayloadSize());

	return true;
}


}	// namespace LibISDB
