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
 @file   Tables.cpp
 @brief  各種テーブル
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "Tables.hpp"
#include "../Base/ARIBTime.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


void PATTable::Reset()
{
	PSISingleTable::Reset();

	m_NITList.clear();
	m_PMTList.clear();
}


uint16_t PATTable::GetTransportStreamID() const
{
	return m_CurSection.GetTableIDExtension();
}


int PATTable::GetNITCount() const
{
	return static_cast<int>(m_NITList.size());
}


uint16_t PATTable::GetNITPID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_NITList.size())
		return PID_INVALID;
	return m_NITList[Index];
}


int PATTable::GetProgramCount() const
{
	return static_cast<int>(m_PMTList.size());
}


uint16_t PATTable::GetPMTPID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_PMTList.size())
		return PID_INVALID;
	return m_PMTList[Index].PID;
}


uint16_t PATTable::GetProgramNumber(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_PMTList.size())
		return 0;
	return m_PMTList[Index].ProgramNumber;
}


bool PATTable::IsPMTTablePID(uint16_t PID) const
{
	for (auto &e : m_PMTList) {
		if (e.PID == PID)
			return true;
	}

	return false;
}


bool PATTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize % 4 != 0)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_NITList.clear();
	m_PMTList.clear();

	for (size_t Pos = 0; Pos < DataSize; Pos += 4) {
		PATItem Item;

		Item.ProgramNumber = Load16(&pData[Pos + 0]);
		Item.PID           = Load16(&pData[Pos + 2]) & 0x1FFF;

		if (Item.ProgramNumber == 0) {
			// NITのPID
			m_NITList.push_back(Item.PID);
		} else {
			// PMTのPID
			m_PMTList.push_back(Item);
		}
	}

	return true;
}




void CATTable::Reset()
{
	PSISingleTable::Reset();

	m_DescriptorBlock.Reset();
}


const CADescriptor * CATTable::GetCADescriptorBySystemID(uint16_t SystemID) const
{
	for (int i = 0; i < m_DescriptorBlock.GetDescriptorCount(); i++) {
		const DescriptorBase *pDescriptor = m_DescriptorBlock.GetDescriptorByIndex(i);

		if ((pDescriptor != nullptr) && (pDescriptor->GetTag() == CADescriptor::TAG)) {
			const CADescriptor *pCADescriptor = dynamic_cast<const CADescriptor *>(pDescriptor);

			if ((pCADescriptor != nullptr) && (pCADescriptor->GetCASystemID() == SystemID))
				return pCADescriptor;
		}
	}

	return nullptr;
}


uint16_t CATTable::GetEMMPID() const
{
	const CADescriptor *pCADescriptor =
		m_DescriptorBlock.GetDescriptor<CADescriptor>();

	if (pCADescriptor != nullptr)
		return pCADescriptor->GetCAPID();

	return PID_INVALID;
}


uint16_t CATTable::GetEMMPID(uint16_t CASystemID) const
{
	const CADescriptor *pCADescriptor = GetCADescriptorBySystemID(CASystemID);

	if (pCADescriptor != nullptr)
		return pCADescriptor->GetCAPID();

	return PID_INVALID;
}


const DescriptorBlock * CATTable::GetCATDescriptorBlock() const
{
	return &m_DescriptorBlock;
}


bool CATTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	if ((pCurSection->GetTableID() != TABLE_ID)
			|| (pCurSection->GetSectionLength() > 1021)
			|| (pCurSection->GetSectionNumber() != 0x00)
			|| (pCurSection->GetLastSectionNumber() != 0x00))
		return false;

	m_DescriptorBlock.ParseBlock(pCurSection->GetPayloadData(), pCurSection->GetPayloadSize());

	return true;
}




PMTTable::PMTTable()
{
	Reset();
}


void PMTTable::Reset()
{
	PSISingleTable::Reset();

	m_PCRPID = PID_INVALID;
	m_DescriptorBlock.Reset();
	m_ESList.clear();
}


uint16_t PMTTable::GetProgramNumberID() const
{
	return m_CurSection.GetTableIDExtension();
}


uint16_t PMTTable::GetPCRPID() const
{
	return m_PCRPID;
}


const DescriptorBlock * PMTTable::GetPMTDescriptorBlock() const
{
	return &m_DescriptorBlock;
}


uint16_t PMTTable::GetECMPID() const
{
	const CADescriptor *pCADescriptor =
		m_DescriptorBlock.GetDescriptor<CADescriptor>();

	if (pCADescriptor != nullptr)
		return pCADescriptor->GetCAPID();

	return PID_INVALID;
}


uint16_t PMTTable::GetECMPID(uint16_t CASystemID) const
{
	// 指定された CA_system_id に対応する ECM の PID を返す
	for (int i = 0; i < m_DescriptorBlock.GetDescriptorCount(); i++) {
		const DescriptorBase *pDescriptor = m_DescriptorBlock.GetDescriptorByIndex(i);

		if ((pDescriptor != nullptr) && (pDescriptor->GetTag() == CADescriptor::TAG)) {
			const CADescriptor *pCADescriptor = dynamic_cast<const CADescriptor *>(pDescriptor);

			if ((pCADescriptor != nullptr) && (pCADescriptor->GetCASystemID() == CASystemID))
				return pCADescriptor->GetCAPID();
		}
	}

	return PID_INVALID;
}


int PMTTable::GetESCount() const
{
	return static_cast<int>(m_ESList.size());
}


uint8_t PMTTable::GetStreamType(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ESList.size())
		return STREAM_TYPE_INVALID;
	return m_ESList[Index].StreamType;
}


uint16_t PMTTable::GetESPID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ESList.size())
		return PID_INVALID;
	return m_ESList[Index].ESPID;
}


const DescriptorBlock * PMTTable::GetItemDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ESList.size())
		return nullptr;
	return &m_ESList[Index].Descriptors;
}


bool PMTTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 4)
		return false;

	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_ESList.clear();

	m_PCRPID = Load16(&pData[0]) & 0x1FFF;
	uint16_t DescriptorLength = Load16(&pData[2]) & 0x0FFF;
	if (4 + DescriptorLength > DataSize)
		return false;

	m_DescriptorBlock.ParseBlock(&pData[4], DescriptorLength);

	for (size_t Pos = 4 + DescriptorLength; Pos + 5 <= DataSize; Pos += 5 + DescriptorLength) {
		DescriptorLength = Load16(&pData[Pos + 3]) & 0x0FFF;
		if (Pos + 5 + DescriptorLength > DataSize)
			break;

		PMTItem &Item = m_ESList.emplace_back();

		Item.StreamType = pData[Pos];
		Item.ESPID      = Load16(&pData[Pos + 1]) & 0x1FFF;

		Item.Descriptors.ParseBlock(&pData[Pos + 5], DescriptorLength);
	}

	return true;
}




SDTTable::SDTTable(uint8_t TableID)
	: m_TableID(TableID)
{
	Reset();
}


void SDTTable::Reset()
{
	PSISingleTable::Reset();

	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_ServiceList.clear();
}


uint8_t SDTTable::GetTableID() const
{
	return m_TableID;
}


uint16_t SDTTable::GetTransportStreamID() const
{
	return m_CurSection.GetTableIDExtension();
}


uint16_t SDTTable::GetNetworkID() const
{
	return m_OriginalNetworkID;
}


uint16_t SDTTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}


int SDTTable::GetServiceCount() const
{
	return static_cast<int>(m_ServiceList.size());
}


int SDTTable::GetServiceIndexByID(uint16_t ServiceID) const
{
	// サービスIDからインデックスを返す
	for (int i = 0; i < GetServiceCount(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID) {
			return i;
		}
	}

	return -1;
}


uint16_t SDTTable::GetServiceID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return SERVICE_ID_INVALID;
	return m_ServiceList[Index].ServiceID;
}


bool SDTTable::GetHEITFlag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].HEITFlag;
}


bool SDTTable::GetMEITFlag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].MEITFlag;
}


bool SDTTable::GetLEITFlag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].LEITFlag;
}


bool SDTTable::GetEITScheduleFlag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].EITScheduleFlag;
}


bool SDTTable::GetEITPresentFollowingFlag(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].EITPresentFollowingFlag;
}


uint8_t SDTTable::GetRunningStatus(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return 0xFF_u8;
	return m_ServiceList[Index].RunningStatus;
}


bool SDTTable::GetFreeCAMode(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;
	return m_ServiceList[Index].FreeCAMode;
}


const DescriptorBlock * SDTTable::GetItemDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return nullptr;
	return &m_ServiceList[Index].Descriptors;
}


bool SDTTable::OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection)
{
	if (pSection->GetTableID() != m_TableID)
		return false;
	return PSISingleTable::OnPSISection(pSectionParser, pSection);
}


bool SDTTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 3)
		return false;
	if (pCurSection->GetTableID() != m_TableID)
		return false;

	m_OriginalNetworkID = Load16(&pData[0]);

	m_ServiceList.clear();

	for (size_t Pos = 3; Pos + 5 <= DataSize;) {
		SDTItem &Item = m_ServiceList.emplace_back();

		Item.ServiceID               = Load16(&pData[Pos + 0]);
		Item.HEITFlag                = (pData[Pos + 2] & 0x10) != 0;
		Item.MEITFlag                = (pData[Pos + 2] & 0x08) != 0;
		Item.LEITFlag                = (pData[Pos + 2] & 0x04) != 0;
		Item.EITScheduleFlag         = (pData[Pos + 2] & 0x02) != 0;
		Item.EITPresentFollowingFlag = (pData[Pos + 2] & 0x01) != 0;
		Item.RunningStatus           = pData[Pos + 3] >> 5;
		Item.FreeCAMode              = (pData[Pos + 3] & 0x10) != 0;

		const size_t DescriptorLength = ((pData[Pos + 3] & 0x0F) << 8) | pData[Pos + 4];
		Pos += 5;
		if (Pos + DescriptorLength > DataSize)
			break;

		Item.Descriptors.ParseBlock(&pData[Pos], DescriptorLength);
		Pos += DescriptorLength;
	}

	return true;
}




PSITableBase * SDTOtherTable::CreateSectionTable(const PSISection *pSection)
{
	return new SDTTable(SDTTable::TABLE_ID_OTHER);
}




SDTTableSet::SDTTableSet()
{
	MapTable(SDTTable::TABLE_ID_ACTUAL, new SDTTable(SDTTable::TABLE_ID_ACTUAL));
	MapTable(SDTTable::TABLE_ID_OTHER, new SDTOtherTable);
}


const SDTTable * SDTTableSet::GetActualSDTTable() const
{
	return dynamic_cast<const SDTTable *>(GetTableByID(SDTTable::TABLE_ID_ACTUAL));
}


const SDTOtherTable * SDTTableSet::GetOtherSDTTable() const
{
	return dynamic_cast<const SDTOtherTable *>(GetTableByID(SDTTable::TABLE_ID_OTHER));
}




NITTable::NITTable()
{
	Reset();
}


void NITTable::Reset()
{
	PSISingleTable::Reset();

	m_NetworkID = NETWORK_ID_INVALID;
	m_NetworkDescriptorBlock.Reset();
	m_TransportStreamList.clear();
}


uint16_t NITTable::GetNetworkID() const
{
	return m_NetworkID;
}


const DescriptorBlock * NITTable::GetNetworkDescriptorBlock() const
{
	return &m_NetworkDescriptorBlock;
}


bool NITTable::GetNetworkName(ReturnArg<ARIBString> Name) const
{
	const NetworkNameDescriptor *pNameDesc =
		m_NetworkDescriptorBlock.GetDescriptor<NetworkNameDescriptor>();
	if (pNameDesc == nullptr)
		return false;
	return pNameDesc->GetNetworkName(Name);
}


int NITTable::GetTransportStreamCount() const
{
	return static_cast<int>(m_TransportStreamList.size());
}


uint16_t NITTable::GetTransportStreamID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TransportStreamList.size())
		return TRANSPORT_STREAM_ID_INVALID;
	return m_TransportStreamList[Index].TransportStreamID;
}


uint16_t NITTable::GetOriginalNetworkID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TransportStreamList.size())
		return NETWORK_ID_INVALID;
	return m_TransportStreamList[Index].OriginalNetworkID;
}


const DescriptorBlock * NITTable::GetItemDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TransportStreamList.size())
		return nullptr;
	return &m_TransportStreamList[Index].Descriptors;
}


bool NITTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 2)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_TransportStreamList.clear();

	m_NetworkID = pCurSection->GetTableIDExtension();

	uint16_t DescriptorLength = Load16(&pData[0]) & 0x0FFF;
	size_t Pos = 2;
	if (Pos + DescriptorLength > DataSize)
		return false;
	m_NetworkDescriptorBlock.ParseBlock(&pData[Pos], DescriptorLength);
	Pos += DescriptorLength;

	uint16_t StreamLoopLength = Load16(&pData[Pos]) & 0x0FFF;
	Pos += 2;
	if (Pos + StreamLoopLength > DataSize)
		return false;

	for (size_t i = 0; i < StreamLoopLength; i += 6 + DescriptorLength) {
		DescriptorLength = Load16(&pData[Pos + 4]) & 0x0FFF;
		if (Pos + 6 + DescriptorLength > DataSize)
			return false;

		NITItem &Item = m_TransportStreamList.emplace_back();

		Item.TransportStreamID = Load16(&pData[Pos + 0]);
		Item.OriginalNetworkID = Load16(&pData[Pos + 2]);
		Pos += 6;
		Item.Descriptors.ParseBlock(&pData[Pos], DescriptorLength);
		Pos += DescriptorLength;
	}

	return true;
}




uint16_t NITMultiTable::GetNITSectionCount() const
{
	return GetSectionCount(0);
}


const NITTable * NITMultiTable::GetNITTable(uint16_t SectionNumber) const
{
	return dynamic_cast<const NITTable *>(GetSection(0, SectionNumber));
}


bool NITMultiTable::IsNITComplete() const
{
	return IsSectionComplete(0);
}


PSITableBase * NITMultiTable::CreateSectionTable(const PSISection *pSection)
{
	return new NITTable;
}




EITTable::EITTable()
{
	Reset();
}


void EITTable::Reset()
{
	PSISingleTable::Reset();

	m_ServiceID = SERVICE_ID_INVALID;
	m_TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_SegmentLastSectionNumber = 0;
	m_LastTableID = 0;
	m_EventList.clear();
}


uint16_t EITTable::GetServiceID() const
{
	return m_ServiceID;
}


uint16_t EITTable::GetTransportStreamID() const
{
	return m_TransportStreamID;
}


uint16_t EITTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}


uint8_t EITTable::GetSegmentLastSectionNumber() const
{
	return m_SegmentLastSectionNumber;
}


uint8_t EITTable::GetLastTableID() const
{
	return m_LastTableID;
}


int EITTable::GetEventCount() const
{
	return static_cast<int>(m_EventList.size());
}


const EITTable::EventInfo * EITTable::GetEventInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return nullptr;
	return &m_EventList[Index];
}


uint16_t EITTable::GetEventID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return 0;
	return m_EventList[Index].EventID;
}


const DateTime * EITTable::GetStartTime(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size()
			|| !m_EventList[Index].StartTime.IsValid())
		return nullptr;
	return &m_EventList[Index].StartTime;
}


uint32_t EITTable::GetDuration(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return 0;
	return m_EventList[Index].Duration;
}


uint8_t EITTable::GetRunningStatus(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return 0;
	return m_EventList[Index].RunningStatus;
}


bool EITTable::GetFreeCAMode(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return false;
	return m_EventList[Index].FreeCAMode;
}


const DescriptorBlock * EITTable::GetItemDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_EventList.size())
		return nullptr;
	return &m_EventList[Index].Descriptors;
}


bool EITTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint8_t TableID = pCurSection->GetTableID();
	if ((TableID < 0x4E) || (TableID > 0x6F))
		return false;
#if 0
	if ((TableID <= 0x4F) && (pCurSection->GetSectionNumber() > 0x01))
		return false;
#endif

	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 6)
		return false;

	m_ServiceID                = pCurSection->GetTableIDExtension();
	m_TransportStreamID        = Load16(&pData[0]);
	m_OriginalNetworkID        = Load16(&pData[2]);
	m_SegmentLastSectionNumber = pData[4];
	m_LastTableID              = pData[5];

	m_EventList.clear();

	size_t Pos = 6;

	while (Pos + 12 <= DataSize) {
		EventInfo &Info = m_EventList.emplace_back();

		Info.EventID        = Load16(&pData[Pos + 0]);
		MJDBCDTimeToDateTime(&pData[Pos + 2], &Info.StartTime);
		Info.Duration       = BCDTimeToSecond(&pData[Pos + 7]);
		Info.RunningStatus  = pData[Pos + 10] >> 5;
		Info.FreeCAMode     = (pData[Pos + 10] & 0x10) != 0;

		const size_t DescriptorLength = ((pData[Pos + 10] & 0x0F) << 8) | pData[Pos + 11];
		if ((DescriptorLength > 0) && (Pos + 12 + DescriptorLength <= DataSize))
			Info.Descriptors.ParseBlock(&pData[Pos + 12], DescriptorLength);

		Pos += 12 + DescriptorLength;
	}

	return true;
}




const EITTable * EITMultiTable::GetEITTableByServiceID(uint16_t ServiceID, uint16_t SectionNumber) const
{
	for (auto &Table : m_TableList) {
		if (static_cast<uint16_t>(Table.UniqueID & 0xFFFF) == ServiceID) {
			if (SectionNumber > Table.LastSectionNumber)
				return nullptr;
			return dynamic_cast<const EITTable *>(Table.SectionList[SectionNumber].Table.get());
		}
	}

	return nullptr;
}


bool EITMultiTable::IsEITSectionComplete(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID) const
{
	const int Index = GetTableIndexByUniqueID(MakeTableUniqueID(NetworkID, TransportStreamID, ServiceID));
	if (Index < 0)
		return false;

	const EITTable *pTable = dynamic_cast<const EITTable *>(GetSection(Index, 0));
	if (pTable == nullptr)
		return false;

	return IsSectionComplete(Index, pTable->GetSegmentLastSectionNumber());
}


uint16_t EITMultiTable::GetServiceID(int Index) const
{
	unsigned long long UniqueID;

	if (!GetTableUniqueID(Index, &UniqueID))
		return SERVICE_ID_INVALID;

	return static_cast<uint16_t>(UniqueID & 0xFFFF);
}


PSITableBase * EITMultiTable::CreateSectionTable(const PSISection *pSection)
{
	return new EITTable;
}


unsigned long long EITMultiTable::GetSectionTableUniqueID(const PSISection *pSection)
{
	const uint16_t DataSize = pSection->GetPayloadSize();

	if (DataSize < 6)
		return pSection->GetTableIDExtension();

	const uint8_t *pData = pSection->GetPayloadData();
	uint16_t TransportStreamID = Load16(&pData[0]);
	uint16_t NetworkID         = Load16(&pData[2]);

	return MakeTableUniqueID(NetworkID, TransportStreamID, pSection->GetTableIDExtension());
}


unsigned long long EITMultiTable::MakeTableUniqueID(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID)
{
	return ((unsigned long long)NetworkID << 32) |
	       ((unsigned long long)TransportStreamID << 16) |
	       ServiceID;
}




EITPfTable::EITPfTable()
{
	MapTable(EITTable::TABLE_ID_PF_ACTUAL, new EITMultiTable);
	MapTable(EITTable::TABLE_ID_PF_OTHER, new EITMultiTable);
}


const EITMultiTable * EITPfTable::GetPfActualTable() const
{
	return dynamic_cast<const EITMultiTable *>(GetTableByID(EITTable::TABLE_ID_PF_ACTUAL));
}


const EITTable * EITPfTable::GetPfActualTable(uint16_t ServiceID, bool Following) const
{
	const EITMultiTable *pTable = GetPfActualTable();
	if (pTable == nullptr)
		return nullptr;
	return pTable->GetEITTableByServiceID(ServiceID, Following ? 1 : 0);
}


const EITMultiTable * EITPfTable::GetPfOtherTable() const
{
	return dynamic_cast<const EITMultiTable *>(GetTableByID(EITTable::TABLE_ID_PF_OTHER));
}


const EITTable * EITPfTable::GetPfOtherTable(uint16_t ServiceID, bool Following) const
{
	const EITMultiTable *pTable = GetPfOtherTable();
	if (pTable == nullptr)
		return nullptr;
	return pTable->GetEITTableByServiceID(ServiceID, Following ? 1 : 0);
}




EITPfActualTable::EITPfActualTable()
{
	MapTable(EITTable::TABLE_ID_PF_ACTUAL, new EITMultiTable);
}


const EITMultiTable * EITPfActualTable::GetPfActualTable() const
{
	return dynamic_cast<const EITMultiTable *>(GetTableByID(EITTable::TABLE_ID_PF_ACTUAL));
}


const EITTable * EITPfActualTable::GetPfActualTable(uint16_t ServiceID, bool Following) const
{
	const EITMultiTable *pTable = GetPfActualTable();
	if (pTable == nullptr)
		return nullptr;
	return pTable->GetEITTableByServiceID(ServiceID, Following ? 1 : 0);
}




EITPfScheduleTable::EITPfScheduleTable()
{
	// 0x50 - 0x57 [schedule actual basic]
	// 0x58 - 0x5F [schedule actual extended]
	// 0x60 - 0x67 [schedule other basic]
	// 0x68 - 0x6F [schedule other extended]
	for (uint8_t TableID = 0x50_u8; TableID <= 0x6F_u8; TableID++)
		MapTable(TableID, new EITMultiTable);
}


const EITTable * EITPfScheduleTable::GetLastUpdatedEITTable() const
{
	const EITMultiTable *pTable = dynamic_cast<const EITMultiTable *>(GetLastUpdatedTable());
	if (pTable == nullptr)
		return nullptr;

	const int Index = pTable->GetTableIndexByUniqueID(m_LastUpdatedTableUniqueID);
	if (Index < 0)
		return nullptr;

	return dynamic_cast<const EITTable *>(pTable->GetSection(Index, m_LastUpdatedSectionNumber));
}


bool EITPfScheduleTable::ResetScheduleService(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID)
{
	bool ResetPerformed = false;

	for (auto &Map : m_TableMap) {
		EITMultiTable *pEITMultiTable = dynamic_cast<EITMultiTable *>(Map.second.get());

		if (pEITMultiTable != nullptr) {
			const int Index = pEITMultiTable->GetTableIndexByUniqueID(
				EITMultiTable::MakeTableUniqueID(NetworkID, TransportStreamID, ServiceID));

			if (Index >= 0) {
				if (pEITMultiTable->ResetTable(Index))
					ResetPerformed = true;
			}
		}
	}

	return ResetPerformed;
}




BITTable::BITTable()
{
	Reset();
}


void BITTable::Reset()
{
	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_BroadcastViewPropriety = false;
	m_DescriptorBlock.Reset();
	m_BroadcasterList.clear();
}


uint16_t BITTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}


bool BITTable::GetBroadcastViewPropriety() const
{
	return m_BroadcastViewPropriety;
}


const DescriptorBlock * BITTable::GetBITDescriptorBlock() const
{
	return &m_DescriptorBlock;
}


int BITTable::GetBroadcasterCount() const
{
	return static_cast<int>(m_BroadcasterList.size());
}


uint8_t BITTable::GetBroadcasterID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_BroadcasterList.size())
		return 0xFF_u8;
	return m_BroadcasterList[Index].BroadcasterID;
}


const DescriptorBlock * BITTable::GetBroadcasterDescriptorBlock(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_BroadcasterList.size())
		return nullptr;
	return &m_BroadcasterList[Index].Descriptors;
}


bool BITTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 2)
		return false;

	m_OriginalNetworkID      = pCurSection->GetTableIDExtension();
	m_BroadcastViewPropriety = (pData[0] & 0x10) != 0;

	size_t DescriptorsLength = ((pData[0] & 0x0F) << 8) | pData[1];
	if ((DescriptorsLength > 0) && (DataSize >= 2 + DescriptorsLength))
		m_DescriptorBlock.ParseBlock(&pData[2], DescriptorsLength);
	else
		m_DescriptorBlock.Reset();

	m_BroadcasterList.clear();

	for (size_t Pos = 2 + DescriptorsLength; Pos + 3 <= DataSize; Pos += DescriptorsLength) {
		BroadcasterInfo &Info = m_BroadcasterList.emplace_back();

		Info.BroadcasterID = pData[Pos + 0];
		DescriptorsLength = ((pData[Pos + 1] & 0x0F) << 8) | pData[Pos + 2];
		Pos += 3;
		if (Pos + DescriptorsLength > DataSize)
			break;

		Info.Descriptors.ParseBlock(&pData[Pos], DescriptorsLength);
	}

	return true;
}




uint16_t BITMultiTable::GetBITSectionCount() const
{
	return GetSectionCount(0);
}


const BITTable * BITMultiTable::GetBITTable(uint16_t SectionNumber) const
{
	return dynamic_cast<const BITTable *>(GetSection(0, SectionNumber));
}


bool BITMultiTable::IsBITComplete() const
{
	return IsSectionComplete(0);
}


PSITableBase * BITMultiTable::CreateSectionTable(const PSISection *pSection)
{
	return new BITTable;
}




TOTTable::TOTTable()
	: PSISingleTable(false)
{
}


void TOTTable::Reset()
{
	PSISingleTable::Reset();

	m_DateTime.Reset();
	m_DescriptorBlock.Reset();
}


bool TOTTable::GetDateTime(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	*Time = m_DateTime;

	return Time->IsValid();
}


bool TOTTable::GetOffsetDateTime(ReturnArg<DateTime> Time, uint32_t CountryCode, uint8_t CountryRegionID) const
{
	if (!Time)
		return false;

	*Time = m_DateTime;

	if (!Time->IsValid())
		return false;

	const int Offset = GetLocalTimeOffset(CountryCode, CountryRegionID);
	if (Offset != 0) {
		if (!Time->OffsetSeconds(Offset * 60))
			return false;
	}

	return true;
}


int TOTTable::GetLocalTimeOffset(uint32_t CountryCode, uint8_t CountryRegionID) const
{
	const LocalTimeOffsetDescriptor *pLocalTimeOffset =
		m_DescriptorBlock.GetDescriptor<LocalTimeOffsetDescriptor>();

	if ((pLocalTimeOffset == nullptr) || !pLocalTimeOffset->IsValid())
		return 0;

	for (int i = 0; i < pLocalTimeOffset->GetTimeOffsetInfoCount(); i++) {
		LocalTimeOffsetDescriptor::TimeOffsetInfo Info;

		if (pLocalTimeOffset->GetTimeOffsetInfo(i, &Info)
				&& (Info.CountryCode == CountryCode)
				&& (Info.CountryRegionID == CountryRegionID)) {
			return Info.LocalTimeOffsetPolarity ? -static_cast<int>(Info.LocalTimeOffset) : Info.LocalTimeOffset;
		}
	}

	return 0;
}


const DescriptorBlock * TOTTable::GetTOTDescriptorBlock() const
{
	return &m_DescriptorBlock;
}


bool TOTTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 7)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	MJDBCDTimeToDateTime(pData, &m_DateTime);

	const uint16_t DescriptorLength = Load16(&pData[5]) & 0x0FFF;
	if ((DescriptorLength > 0) && (DescriptorLength <= DataSize - 7))
		m_DescriptorBlock.ParseBlock(&pData[7], DescriptorLength);
	else
		m_DescriptorBlock.Reset();

	return true;
}




CDTTable::CDTTable()
{
	Reset();
}


void CDTTable::Reset()
{
	PSIStreamTable::Reset();

	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_DataType = DATA_TYPE_INVALID;
	m_DescriptorBlock.Reset();
	m_ModuleData.ClearSize();
}


uint16_t CDTTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}


uint8_t CDTTable::GetDataType() const
{
	return m_DataType;
}


const DescriptorBlock * CDTTable::GetDescriptorBlock() const
{
	return &m_DescriptorBlock;
}


uint16_t CDTTable::GetDataModuleSize() const
{
	return static_cast<uint16_t>(m_ModuleData.GetSize());
}


const uint8_t * CDTTable::GetDataModuleData() const
{
	if (m_ModuleData.GetSize() == 0)
		return nullptr;

	return m_ModuleData.GetData();
}


bool CDTTable::OnTableUpdate(const PSISection *pCurSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 5)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_OriginalNetworkID = Load16(&pData[0]);
	m_DataType          = pData[2];

	m_DescriptorBlock.Reset();
	m_ModuleData.ClearSize();

	const uint16_t DescriptorLength = Load16(&pData[3]) & 0x0FFF_u16;
	if (5 + DescriptorLength <= DataSize) {
		if (DescriptorLength > 0)
			m_DescriptorBlock.ParseBlock(&pData[5], DescriptorLength);
		m_ModuleData.SetData(&pData[5 + DescriptorLength], DataSize - (5 + DescriptorLength));
	}

	return true;
}




SDTTTable::SDTTTable()
{
	Reset();
}


void SDTTTable::Reset()
{
	PSIStreamTable::Reset();

	m_MakerID = 0;
	m_ModelID = 0;
	m_TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_ServiceID = SERVICE_ID_INVALID;
	m_ContentList.clear();
}


uint8_t SDTTTable::GetMakerID() const
{
	return m_MakerID;
}


uint8_t SDTTTable::GetModelID() const
{
	return m_ModelID;
}


bool SDTTTable::IsCommon() const
{
	return (m_MakerID == 0xFF) && (m_ModelID == 0xFE);
}


uint16_t SDTTTable::GetTransportStreamID() const
{
	return m_TransportStreamID;
}


uint16_t SDTTTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}


uint16_t SDTTTable::GetServiceID() const
{
	return m_ServiceID;
}


uint8_t SDTTTable::GetNumOfContents() const
{
	return (uint8_t)m_ContentList.size();
}


const SDTTTable::ContentInfo * SDTTTable::GetContentInfo(uint8_t Index) const
{
	if (Index >= m_ContentList.size())
		return nullptr;

	return &m_ContentList[Index];
}


bool SDTTTable::IsSchedule(uint32_t Index) const
{
	if (Index >= m_ContentList.size())
		return false;

	return !m_ContentList[Index].ScheduleList.empty();
}


const DescriptorBlock * SDTTTable::GetContentDescriptorBlock(uint32_t Index) const
{
	if (Index >= m_ContentList.size())
		return nullptr;

	return &m_ContentList[Index].Descriptors;
}


bool SDTTTable::OnTableUpdate(const PSISection *pCurSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (DataSize < 7)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_MakerID           = (uint8_t)(pCurSection->GetTableIDExtension() >> 8);
	m_ModelID           = (uint8_t)(pCurSection->GetTableIDExtension() & 0xFF);
	m_TransportStreamID = Load16(&pData[0]);
	m_OriginalNetworkID = Load16(&pData[2]);
	m_ServiceID         = Load16(&pData[4]);

	m_ContentList.clear();
	const int NumOfContents = pData[6];
	size_t Pos = 7;

	for (int i = 0; i < NumOfContents; i++) {
		if (Pos + 8 > DataSize)
			break;

		const size_t ContentDescLength  = (pData[Pos + 4] << 4) | (pData[Pos + 5] >> 4);
		const size_t ScheduleDescLength = (pData[Pos + 6] << 4) | (pData[Pos + 7] >> 4);
		if ((ContentDescLength < ScheduleDescLength) || (Pos + ContentDescLength > DataSize))
			break;

		ContentInfo &Content = m_ContentList.emplace_back();

		Content.GroupID          = pData[Pos] >> 4;
		Content.TargetVersion    = static_cast<uint16_t>(((pData[Pos] & 0x0F) << 8) | pData[Pos + 1]);
		Content.NewVersion       = static_cast<uint16_t>((pData[Pos + 2] << 4) | (pData[Pos + 3] >> 4));
		Content.DownloadLevel    = (pData[Pos + 3] >> 2) & 0x03;
		Content.VersionIndicator = pData[Pos + 3] & 0x03;

		Content.ScheduleTimeShiftInformation = pData[Pos + 7] & 0x0F;

		Pos += 8;

		if (ScheduleDescLength > 0) {
			for (size_t j = 0; j + 8 <= ScheduleDescLength; j += 8) {
				ScheduleDescription Schedule;

				MJDBCDTimeToDateTime(&pData[Pos + j], &Schedule.StartTime);
				Schedule.Duration = BCDTimeToSecond(&pData[Pos + j + 5]);

				Content.ScheduleList.push_back(Schedule);
			}

			Pos += ScheduleDescLength;
		}

		const size_t DescLength = ContentDescLength - ScheduleDescLength;
		if (DescLength > 0) {
			Content.Descriptors.ParseBlock(&pData[Pos], DescLength);
			Pos += DescLength;
		}
	}

	return true;
}




PCRTable::PCRTable()
	 : m_PCR(PCR_INVALID)
{
}


bool PCRTable::StorePacket(const TSPacket *pPacket)
{
	if (pPacket == nullptr)
		return false;

	if (pPacket->GetPCRFlag()) {
		if (pPacket->GetOptionSize() < 5)
			return false;

		const uint8_t *pOptionData = pPacket->GetOptionData();
		m_PCR =
			(static_cast<uint64_t>(pOptionData[0]) << 25) |
			(static_cast<uint64_t>(pOptionData[1]) << 17) |
			(static_cast<uint64_t>(pOptionData[2]) <<  9) |
			(static_cast<uint64_t>(pOptionData[3]) <<  1) |
			(static_cast<uint64_t>(pOptionData[4]) >>  7);
	}

	return true;
}


uint64_t PCRTable::GetPCRTimeStamp() const
{
	return m_PCR;
}


}	// namespace LibISDB
