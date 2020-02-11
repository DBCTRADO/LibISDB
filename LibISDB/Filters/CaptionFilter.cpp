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
 @file   CaptionFilter.cpp
 @brief  字幕フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "CaptionFilter.hpp"
#include "../TS/Tables.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{


class CaptionStream : public PIDMapTarget
{
// PIDMapTarget
	bool StorePacket(const TSPacket *pPacket) override
	{
		m_CaptionParser.StorePacket(pPacket);
		return false;
	}

	void OnPIDUnmapped(uint16_t PID) override
	{
		delete this;
	}

	CaptionParser m_CaptionParser;

public:
	CaptionStream(bool OneSeg)
		: m_CaptionParser(OneSeg)
	{
	}

	void SetCaptionHandler(CaptionParser::CaptionHandler *pHandler)
	{
		m_CaptionParser.SetCaptionHandler(pHandler);
	}

	void SetDRCSMap(CaptionParser::DRCSMap *pDRCSMap)
	{
		m_CaptionParser.SetDRCSMap(pDRCSMap);
	}

	CaptionParser * GetParser() { return &m_CaptionParser; }
};


}




CaptionFilter::CaptionFilter()
	: m_FollowActiveService(true)
	, m_TargetServiceID(SERVICE_ID_INVALID)
	, m_TargetComponentTag(0xFF)
	, m_TargetESPID(PID_INVALID)

	, m_pCaptionHandler(nullptr)
	, m_pDRCSMap(nullptr)
{
	Reset();
}


void CaptionFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_ServiceList.clear();

	m_TargetESPID = PID_INVALID;

	m_PIDMapManager.UnmapAllTargets();

	m_PIDMapManager.MapTarget(
		PID_PAT, PSITableBase::CreateWithHandler<PATTable>(&CaptionFilter::OnPATSection, this));
}


void CaptionFilter::SetActiveServiceID(uint16_t ServiceID)
{
	if (m_FollowActiveService)
		SetTargetStream(ServiceID);
}


bool CaptionFilter::ProcessData(DataStream *pData)
{
	if (pData->Is<TSPacket>())
		m_PIDMapManager.StorePacketStream(pData);

	return true;
}


bool CaptionFilter::SetTargetStream(uint16_t ServiceID, uint8_t ComponentTag)
{
	BlockLock Lock(m_FilterLock);

	LIBISDB_TRACE(LIBISDB_STR("Select caption : service_id %04X / component_tag %02X\n"), ServiceID, ComponentTag);

	if (m_TargetESPID != PID_INVALID) {
		CaptionStream *pStream = m_PIDMapManager.GetMapTarget<CaptionStream>(m_TargetESPID);

		if (pStream != nullptr) {
			pStream->SetCaptionHandler(nullptr);
			pStream->SetDRCSMap(nullptr);
		}

		m_TargetESPID = PID_INVALID;
	}

	const int Index = GetServiceIndexByID(ServiceID);
	if (Index >= 0) {
		uint16_t TargetESPID = PID_INVALID;

		if (ComponentTag == 0xFF) {
			if (!m_ServiceList[Index].CaptionESList.empty())
				TargetESPID = m_ServiceList[Index].CaptionESList[0].PID;
		} else {
			for (auto const &e : m_ServiceList[Index].CaptionESList) {
				if (e.ComponentTag == ComponentTag) {
					TargetESPID = e.PID;
					break;
				}
			}
		}

		if (TargetESPID != PID_INVALID) {
			CaptionStream *pStream = m_PIDMapManager.GetMapTarget<CaptionStream>(TargetESPID);

			if (pStream != nullptr) {
				pStream->SetCaptionHandler(this);
				pStream->SetDRCSMap(m_pDRCSMap);
				m_TargetESPID = TargetESPID;
			}
		}
	}

	m_TargetServiceID = ServiceID;
	m_TargetComponentTag = ComponentTag;

	return true;
}


void CaptionFilter::SetFollowActiveService(bool Follow)
{
	BlockLock Lock(m_FilterLock);

	m_FollowActiveService = Follow;
}


void CaptionFilter::SetCaptionHandler(Handler *pHandler)
{
	BlockLock Lock(m_FilterLock);

	m_pCaptionHandler = pHandler;
}


void CaptionFilter::SetDRCSMap(DRCSMap *pDRCSMap)
{
	BlockLock Lock(m_FilterLock);

	m_pDRCSMap = pDRCSMap;

	CaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser != nullptr)
		pParser->SetDRCSMap(pDRCSMap);
}


int CaptionFilter::GetLanguageCount() const
{
	BlockLock Lock(m_FilterLock);

	const CaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser == nullptr)
		return 0;

	return pParser->GetLanguageCount();
}


uint32_t CaptionFilter::GetLanguageCode(uint8_t LanguageTag) const
{
	BlockLock Lock(m_FilterLock);

	const CaptionParser *pParser = GetCurrentCaptionParser();
	if (pParser == nullptr)
		return 0;

	return pParser->GetLanguageCodeByTag(LanguageTag);
}


void CaptionFilter::OnLanguageUpdate(CaptionParser *pParser)
{
	if (m_pCaptionHandler != nullptr)
		m_pCaptionHandler->OnLanguageUpdate(this, pParser);
}


void CaptionFilter::OnCaption(
	CaptionParser *pParser, uint8_t Language, const CharType *pText,
	const ARIBStringDecoder::FormatList *pFormatList)
{
	if (m_pCaptionHandler != nullptr)
		m_pCaptionHandler->OnCaption(this, pParser, Language, pText, pFormatList);
}


int CaptionFilter::GetServiceIndexByID(uint16_t ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return static_cast<int>(i);
	}
	return -1;
}


CaptionParser * CaptionFilter::GetCurrentCaptionParser() const
{
	if (m_TargetESPID != PID_INVALID) {
		CaptionStream *pStream = m_PIDMapManager.GetMapTarget<CaptionStream>(m_TargetESPID);

		if (pStream != nullptr)
			return pStream->GetParser();
	}

	return nullptr;
}


void CaptionFilter::OnPATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	const PATTable *pPATTable = dynamic_cast<const PATTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPATTable == nullptr))
		return;

	// 現PMTのPIDをアンマップする
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		m_PIDMapManager.UnmapTarget(m_ServiceList[i].PMTPID);
		for (size_t j = 0; j < m_ServiceList[i].CaptionESList.size(); j++)
			m_PIDMapManager.UnmapTarget(m_ServiceList[i].CaptionESList[j].PID);
	}

	m_TargetESPID = PID_INVALID;

	m_ServiceList.resize(pPATTable->GetProgramCount());

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		const uint16_t PMTPID = pPATTable->GetPMTPID((uint16_t)i);

		m_ServiceList[i].ServiceID = pPATTable->GetProgramNumber((uint16_t)i);
		m_ServiceList[i].PMTPID = PMTPID;
		m_ServiceList[i].CaptionESList.clear();

		m_PIDMapManager.MapTarget(PMTPID, PSITableBase::CreateWithHandler<PMTTable>(&CaptionFilter::OnPMTSection, this));
	}
}


void CaptionFilter::OnPMTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	const PMTTable *pPMTTable = dynamic_cast<const PMTTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPMTTable == nullptr))
		return;

	const int ServiceIndex = GetServiceIndexByID(pPMTTable->GetProgramNumberID());
	if (ServiceIndex < 0)
		return;

	ServiceInfo &Info = m_ServiceList[ServiceIndex];

	Info.CaptionESList.clear();

	for (int ESIndex = 0; ESIndex < pPMTTable->GetESCount(); ESIndex++) {
		const uint8_t StreamType = pPMTTable->GetStreamType(ESIndex);

		if (StreamType == STREAM_TYPE_CAPTION) {
			CaptionESInfo CaptionInfo;

			CaptionInfo.PID = pPMTTable->GetESPID(ESIndex);
			CaptionInfo.ComponentTag = 0xFF;

			const DescriptorBlock *pDescBlock = pPMTTable->GetItemDescriptorBlock(ESIndex);
			if (pDescBlock != nullptr) {
				const StreamIDDescriptor *pStreamIDDesc = pDescBlock->GetDescriptor<StreamIDDescriptor>();

				if (pStreamIDDesc != nullptr)
					CaptionInfo.ComponentTag = pStreamIDDesc->GetComponentTag();
			}

			Info.CaptionESList.push_back(CaptionInfo);

			CaptionStream *pStream = new CaptionStream(Is1SegPMTPID(Info.PMTPID));
			m_PIDMapManager.MapTarget(CaptionInfo.PID, pStream);
		}
	}

	SetTargetStream(m_TargetServiceID, m_TargetComponentTag);
}


}	// namespace LibISDB
