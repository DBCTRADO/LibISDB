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
 @file   StreamSelector.cpp
 @brief  ストリーム選択
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamSelector.hpp"
#include "Tables.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/CRC.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


StreamSelector::StreamSelector()
	: m_TargetServiceID(SERVICE_ID_INVALID)
	, m_TargetStreamTypeEnabled(false)
	, m_GeneratePAT(true)

	, m_TargetPMTPID(PID_INVALID)
	, m_LastTSID(TRANSPORT_STREAM_ID_INVALID)
	, m_LastPMTPID(PID_INVALID)
	, m_LastVersion(0)
	, m_Version(0)
{
	m_PATPacket.SetSize(TS_PACKET_SIZE);
	Reset();
}


void StreamSelector::Reset()
{
	m_PIDMapManager.UnmapAllTargets();

	// PATテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_PAT, PSITableBase::CreateWithHandler<PATTable>(&StreamSelector::OnPATSection, this));
	// CATテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_CAT, PSITableBase::CreateWithHandler<CATTable>(&StreamSelector::OnCATSection, this));

	m_PMTPIDList.clear();
	m_EMMPIDList.clear();
	m_TargetPIDTable.fill(false);

	m_TargetPMTPID = PID_INVALID;
	m_LastTSID = TRANSPORT_STREAM_ID_INVALID;
	m_LastPMTPID = PID_INVALID;
	m_LastVersion = 0;
	m_Version = 0;
}


TSPacket * StreamSelector::InputPacket(TSPacket *pPacket)
{
	m_PIDMapManager.StorePacket(pPacket);

	if ((m_TargetServiceID == SERVICE_ID_INVALID) && !m_TargetStreamTypeEnabled) {
		return pPacket;
	} else {
		const uint16_t PID = pPacket->GetPID();

		if ((PID < 0x0030) || m_TargetPIDTable[PID]) {
			if ((PID == PID_PAT)
					&& m_GeneratePAT
					&& (m_TargetPMTPID != PID_INVALID)
					&& MakePAT(pPacket, &m_PATPacket)) {
				return &m_PATPacket;
			} else {
				return pPacket;
			}
		}
	}

	return nullptr;
}


bool StreamSelector::SetTarget(uint16_t ServiceID, const StreamTypeTable *pStreamType)
{
	m_TargetServiceID = ServiceID;
	if (pStreamType != nullptr) {
		m_TargetStreamTypeEnabled = true;
		m_TargetStreamType = *pStreamType;
	} else {
		m_TargetStreamTypeEnabled = false;
	}

	m_TargetPMTPID = PID_INVALID;
	if (ServiceID != SERVICE_ID_INVALID) {
		const int ServiceIndex = GetServiceIndexByID(ServiceID);
		if (ServiceIndex >= 0)
			m_TargetPMTPID = m_PMTPIDList[ServiceIndex].PMTPID;
	}

	MakeTargetPIDTable();

	return true;
}


bool StreamSelector::SetTarget(uint16_t ServiceID, StreamFlag StreamFlags)
{
	if (StreamFlags == StreamFlag::All)
		return SetTarget(ServiceID, nullptr);

	StreamTypeTable StreamTable(StreamFlags);

	return SetTarget(ServiceID, &StreamTable);
}


void StreamSelector::SetGeneratePAT(bool Generate)
{
	m_GeneratePAT = Generate;
}


void StreamSelector::MakeTargetPIDTable()
{
	if (m_PMTPIDList.empty()) {
		m_TargetPIDTable.fill(m_TargetServiceID == SERVICE_ID_INVALID);
		return;
	}

	m_TargetPIDTable.fill(false);

	for (auto const &PMT : m_PMTPIDList) {
		if ((m_TargetServiceID == SERVICE_ID_INVALID) || (m_TargetServiceID == PMT.ServiceID)) {
			m_TargetPIDTable[PMT.PMTPID] = true;

			if (PMT.PCRPID != PID_INVALID)
				m_TargetPIDTable[PMT.PCRPID] = true;

			for (uint16_t ECMPID : PMT.ECMPIDList)
				m_TargetPIDTable[ECMPID] = true;

			for (ESInfo ES : PMT.ESList) {
				if (!m_TargetStreamTypeEnabled || m_TargetStreamType[ES.StreamType])
					m_TargetPIDTable[ES.PID] = true;
			}
		}
	}

	for (uint16_t EMMPID : m_EMMPIDList)
		m_TargetPIDTable[EMMPID] = true;
}


int StreamSelector::GetServiceIndexByID(uint16_t ServiceID) const
{
	int Index;

	for (Index = static_cast<int>(m_PMTPIDList.size()) - 1; Index >= 0; Index--) {
		if (m_PMTPIDList[Index].ServiceID == ServiceID)
			break;
	}

	return Index;
}


void StreamSelector::OnPATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PATが更新された
	const PATTable *pPATTable = static_cast<const PATTable *>(pTable);

	for (auto const &e : m_PMTPIDList)
		m_PIDMapManager.UnmapTarget(e.PMTPID);

	m_TargetPMTPID = PID_INVALID;

	std::vector<PMTPIDInfo> PMTPIDList;

	PMTPIDList.resize(pPATTable->GetProgramCount());

	for (int i = 0; i < pPATTable->GetProgramCount(); i++) {
		const uint16_t ServiceID = pPATTable->GetProgramNumber(i);
		const uint16_t PMTPID = pPATTable->GetPMTPID(i);

		if (m_TargetServiceID == ServiceID)
			m_TargetPMTPID = PMTPID;

		const int ServiceIndex = GetServiceIndexByID(ServiceID);

		if (ServiceIndex < 0) {
			PMTPIDList[i].ServiceID = ServiceID;
			PMTPIDList[i].PCRPID = PID_INVALID;
		} else {
			PMTPIDList[i] = m_PMTPIDList[ServiceIndex];
		}

		PMTPIDList[i].PMTPID = PMTPID;

		m_PIDMapManager.MapTarget(PMTPID, PSITableBase::CreateWithHandler<PMTTable>(&StreamSelector::OnPMTSection, this));
	}

	m_PMTPIDList = std::move(PMTPIDList);

	MakeTargetPIDTable();
}


void StreamSelector::OnPMTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PMTが更新された
	const PMTTable *pPMTTable = dynamic_cast<const PMTTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPMTTable == nullptr))
		return;

	const int ServiceIndex = GetServiceIndexByID(pPMTTable->GetProgramNumberID());
	if (ServiceIndex < 0)
		return;
	PMTPIDInfo &PIDInfo = m_PMTPIDList[ServiceIndex];

	// PCRのPID追加
	if (const uint16_t PCRPID = pPMTTable->GetPCRPID(); PCRPID < 0x1FFF) {
		PIDInfo.PCRPID = PCRPID;
	} else {
		PIDInfo.PCRPID = PID_INVALID;
	}

	// ECMのPID追加
	PIDInfo.ECMPIDList.clear();
	const DescriptorBlock *pDescBlock = pPMTTable->GetPMTDescriptorBlock();
	pDescBlock->EnumDescriptors<CADescriptor>(
		[&](const CADescriptor *pCADesc) {
			if (pCADesc->GetCAPID() < 0x1FFF)
				PIDInfo.ECMPIDList.push_back(pCADesc->GetCAPID());
		});

	// ESのPID追加
	PIDInfo.ESList.clear();
	for (int i = 0; i < pPMTTable->GetESCount(); i++) {
		ESInfo ES;
		ES.StreamType = pPMTTable->GetStreamType(i);
		ES.PID = pPMTTable->GetESPID(i);
		PIDInfo.ESList.push_back(ES);
	}

	MakeTargetPIDTable();
}


void StreamSelector::OnCATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// CATが更新された
	const CATTable *pCATTable = dynamic_cast<const CATTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pCATTable == nullptr))
		return;

	// EMMのPID追加
	m_EMMPIDList.clear();
	const DescriptorBlock *pDescBlock = pCATTable->GetCATDescriptorBlock();
	pDescBlock->EnumDescriptors<CADescriptor>(
		[this](const CADescriptor *pCADesc) {
			if (pCADesc->GetCAPID() < 0x1FFF)
				m_EMMPIDList.push_back(pCADesc->GetCAPID());
		});

	MakeTargetPIDTable();
}


bool StreamSelector::MakePAT(const TSPacket *pSrcPacket, TSPacket *pDstPacket)
{
	const uint8_t *pPayloadData = pSrcPacket->GetPayloadData();
	if (pPayloadData == nullptr)
		return false;
	const uint8_t *pSrcData = pSrcPacket->GetData();
	uint8_t *pDstData = pDstPacket->GetData();
	size_t HeaderSize = pPayloadData - pSrcData;

	if (!pSrcPacket->GetPayloadUnitStartIndicator())
		return false;
	size_t UnitStartPos = pPayloadData[0] + 1;
	pPayloadData += UnitStartPos;
	HeaderSize += UnitStartPos;
	if (HeaderSize >= TS_PACKET_SIZE)
		return false;

	std::memcpy(pDstData, pSrcData, HeaderSize);
	pDstData += HeaderSize;
	std::memset(pDstData, 0xFF, TS_PACKET_SIZE - HeaderSize);

	if (pPayloadData[0] != 0)	// table_id 不正
		return false;

	uint16_t SectionLength = static_cast<uint16_t>(((pPayloadData[1] & 0x0F) << 8) | pPayloadData[2]);
	if (SectionLength > TS_PACKET_SIZE - HeaderSize - 3 - 4 )
		return false;

	uint32_t CRC = Load32(&pPayloadData[3 + SectionLength - 4]);
	if (CRC32MPEG2::Calc(pPayloadData, 3 + SectionLength - 4) != CRC)
		return false;

	uint16_t TSID = Load16(&pPayloadData[3]);
	uint8_t Version = (pPayloadData[5] & 0x3E) >> 1;
	if (TSID != m_LastTSID) {
		m_Version = 0;
	} else if ((m_TargetPMTPID != m_LastPMTPID) || (Version != m_LastVersion)) {
		m_Version = (m_Version + 1) & 0x1F;
	}
	m_LastTSID = TSID;
	m_LastPMTPID = m_TargetPMTPID;
	m_LastVersion = Version;

	const uint8_t *pProgramData = pPayloadData + 8;
	size_t Pos = 0;
	uint32_t NewProgramListSize = 0;
	bool HasPMTPID = false;
	while (Pos < static_cast<size_t>(SectionLength) - (5 + 4)) {
		//uint16_t ProgramNumber = Load16(&pProgramData[Pos]);
		uint16_t PID = Load16(&pProgramData[Pos + 2]) & 0x1FFF_u16;

		if ((PID == 0x0010) || (PID == m_TargetPMTPID)) {
			std::memcpy(pDstData + 8 + NewProgramListSize, pProgramData + Pos, 4);
			NewProgramListSize += 4;
			if (PID == m_TargetPMTPID)
				HasPMTPID = true;
		}
		Pos += 4;
	}
	if (!HasPMTPID)
		return false;

	pDstData[0] = 0;
	pDstData[1] = static_cast<uint8_t>((pPayloadData[1] & 0xF0) | ((NewProgramListSize + (5 + 4)) >> 8));
	pDstData[2] = static_cast<uint8_t>((NewProgramListSize + (5 + 4)) & 0xFF);
	pDstData[3] = static_cast<uint8_t>(TSID >> 8);
	pDstData[4] = static_cast<uint8_t>(TSID & 0xFF);
	pDstData[5] = static_cast<uint8_t>((pPayloadData[5] & 0xC1) | (m_Version << 1));
	pDstData[6] = pPayloadData[6];
	pDstData[7] = pPayloadData[7];
	Store32(&pDstData[8 + NewProgramListSize], CRC32MPEG2::Calc(pDstData, 8 + NewProgramListSize));

#ifdef LIBISDB_DEBUG
	TSPacket::ParseResult Result = pDstPacket->ParsePacket();
	LIBISDB_ASSERT(Result == TSPacket::ParseResult::OK);
#else
	pDstPacket->ParsePacket();
#endif

	return true;
}




StreamSelector::StreamTypeTable::StreamTypeTable()
{
	Set();
}


StreamSelector::StreamTypeTable::StreamTypeTable(StreamFlag Flags)
{
	FromStreamFlags(Flags);
}


void StreamSelector::StreamTypeTable::FromStreamFlags(StreamFlag Flags)
{
	static const uint8_t StreamTypeList[] = {
		STREAM_TYPE_MPEG1_VIDEO,
		STREAM_TYPE_MPEG2_VIDEO,
		STREAM_TYPE_MPEG1_AUDIO,
		STREAM_TYPE_MPEG2_AUDIO,
		STREAM_TYPE_AAC,
		STREAM_TYPE_MPEG4_VISUAL,
		STREAM_TYPE_MPEG4_AUDIO,
		STREAM_TYPE_H264,
		STREAM_TYPE_H265,
		STREAM_TYPE_AC3,
		STREAM_TYPE_DTS,
		STREAM_TYPE_TRUEHD,
		STREAM_TYPE_DOLBY_DIGITAL_PLUS,
		STREAM_TYPE_CAPTION,
		STREAM_TYPE_DATA_CARROUSEL,
	};

	Set();

	for (size_t i = 0; i < std::size(StreamTypeList); i++) {
		if (!(Flags & static_cast<StreamFlag>(1UL << i)))
			Reset(StreamTypeList[i]);
	}
}


}	// namespace LibISDB
