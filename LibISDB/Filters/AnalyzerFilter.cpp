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
 @file   AnalyzerFilter.cpp
 @brief  解析フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "AnalyzerFilter.hpp"
#include "../Utilities/Sort.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


AnalyzerFilter::AnalyzerFilter()
{
	Reset();
}


void AnalyzerFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	// 全テーブルアンマップ
	m_PIDMapManager.UnmapAllTargets();

	m_TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_NetworkID = NETWORK_ID_INVALID;

	m_PATUpdated = false;
	m_SDTUpdated = false;
	m_NITUpdated = false;
#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
	m_EITUpdated = false;
	m_SendEITUpdatedEvent = false;
#endif

	m_TOTInterpolation.PCRPID = PID_INVALID;

	m_ServiceList.clear();
	m_SDTServiceList.clear();
	m_SDTStreamMap.clear();
	m_NetworkStreamList.clear();
	m_NITInfo.Reset();
	m_EMMPIDList.clear();

	// PATテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_PAT, PSITableBase::CreateWithHandler<PATTable>(&AnalyzerFilter::OnPATSection, this));
	// NITテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_NIT, PSITableBase::CreateWithHandler<NITMultiTable>(&AnalyzerFilter::OnNITSection, this));
	// SDTテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_SDT, PSITableBase::CreateWithHandler<SDTTableSet>(&AnalyzerFilter::OnSDTSection, this));
#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
	// H-EITテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_HEIT, PSITableBase::CreateWithHandler<EITPfActualTable>(&AnalyzerFilter::OnEITSection, this));
#ifdef LIBISDB_ANALYZER_FILTER_L_EIT_SUPPORT
	// L-EITテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_LEIT, PSITableBase::CreateWithHandler<EITPfActualTable>(&AnalyzerFilter::OnEITSection, this));
#endif
#endif
	// CATテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_CAT, PSITableBase::CreateWithHandler<CATTable>(&AnalyzerFilter::OnCATSection, this));
	// TOTテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_TOT, PSITableBase::CreateWithHandler<TOTTable>(&AnalyzerFilter::OnTOTSection, this));
}


bool AnalyzerFilter::ReceiveData(DataStream *pData)
{
	{
		BlockLock Lock(m_FilterLock);

		if (pData->Is<TSPacket>())
			m_PIDMapManager.StorePacketStream(pData);

		OutputData(pData);
	}

#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
	// 保留されていたEIT更新イベントを通知する
	if (m_SendEITUpdatedEvent) {
		m_SendEITUpdatedEvent = false;
		m_EventListenerList.CallEventListener(&EventListener::OnEITUpdated, this);
	}
#endif

	return true;
}


int AnalyzerFilter::GetServiceCount() const
{
	BlockLock Lock(m_FilterLock);

	return static_cast<int>(m_ServiceList.size());
}


uint16_t AnalyzerFilter::GetServiceID(int Index) const
{
	BlockLock Lock(m_FilterLock);

	uint16_t ServiceID = SERVICE_ID_INVALID;

	if (Index < 0) {
		if (Is1SegStream()) {
			uint16_t MinPID = 0xFFFF;
			size_t MinIndex;
			for (size_t i = 0; i < m_ServiceList.size(); i++) {
				if (Is1SegPMTPID(m_ServiceList[i].PMTPID)
						&& (m_ServiceList[i].PMTPID < MinPID)) {
					MinPID = m_ServiceList[i].PMTPID;
					MinIndex = i;
				}
			}
			if ((MinPID == 0xFFFF) || !m_ServiceList[MinIndex].IsPMTAcquired)
				return SERVICE_ID_INVALID;
			ServiceID = m_ServiceList[MinIndex].ServiceID;
		} else {
			if (m_ServiceList.empty() || !m_ServiceList[0].IsPMTAcquired)
				return SERVICE_ID_INVALID;
			ServiceID = m_ServiceList[0].ServiceID;
		}
	} else if ((Index >= 0) && (static_cast<size_t>(Index) < m_ServiceList.size())) {
		ServiceID = m_ServiceList[Index].ServiceID;
	}

	return ServiceID;
}


int AnalyzerFilter::GetServiceIndexByID(uint16_t ServiceID) const
{
	BlockLock Lock(m_FilterLock);

	for (size_t Index = 0; Index < m_ServiceList.size(); Index++) {
		if (m_ServiceList[Index].ServiceID == ServiceID)
			return static_cast<int>(Index);
	}

	return -1;
}


bool AnalyzerFilter::GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const
{
	BlockLock Lock(m_FilterLock);

	if (!Info || (static_cast<unsigned int>(Index) >= m_ServiceList.size()))
		return false;

	*Info = m_ServiceList[Index];

	return true;
}


bool AnalyzerFilter::GetServiceInfoByID(uint16_t ServiceID, ReturnArg<ServiceInfo> Info) const
{
	BlockLock Lock(m_FilterLock);

	return GetServiceInfo(GetServiceIndexByID(ServiceID), Info);
}


bool AnalyzerFilter::IsServicePMTAcquired(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	return m_ServiceList[Index].IsPMTAcquired;
}


bool AnalyzerFilter::Is1SegService(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	return Is1SegPMTPID(m_ServiceList[Index].PMTPID);
}


uint16_t AnalyzerFilter::GetPMTPID(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return PID_INVALID;

	return m_ServiceList[Index].PMTPID;
}


int AnalyzerFilter::GetVideoESCount(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return 0;

	return static_cast<int>(m_ServiceList[Index].VideoESList.size());
}


bool AnalyzerFilter::GetVideoESList(int Index, ReturnArg<ESInfoList> ESList) const
{
	if (!ESList)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	*ESList = m_ServiceList[Index].VideoESList;

	return true;
}


bool AnalyzerFilter::GetVideoESInfo(int Index, int VideoIndex, ReturnArg<ESInfo> ESInfo) const
{
	if (!ESInfo)
		return false;

	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(VideoIndex) >= m_ServiceList[Index].VideoESList.size()))
		return false;

	*ESInfo = m_ServiceList[Index].VideoESList[VideoIndex];

	return true;
}


uint16_t AnalyzerFilter::GetVideoESPID(int Index, int VideoIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(VideoIndex) >= m_ServiceList[Index].VideoESList.size()))
		return PID_INVALID;

	return m_ServiceList[Index].VideoESList[VideoIndex].PID;
}


uint8_t AnalyzerFilter::GetVideoStreamType(int Index, int VideoIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(VideoIndex) >= m_ServiceList[Index].VideoESList.size()))
		return STREAM_TYPE_INVALID;

	return m_ServiceList[Index].VideoESList[VideoIndex].StreamType;
}


uint8_t AnalyzerFilter::GetVideoComponentTag(int Index, int VideoIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(VideoIndex) >= m_ServiceList[Index].VideoESList.size()))
		return COMPONENT_TAG_INVALID;

	return m_ServiceList[Index].VideoESList[VideoIndex].ComponentTag;
}


int AnalyzerFilter::GetVideoIndexByComponentTag(int Index, uint8_t ComponentTag) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return -1;

	for (size_t i = 0; i < m_ServiceList[Index].VideoESList.size(); i++) {
		if (m_ServiceList[Index].VideoESList[i].ComponentTag == ComponentTag)
			return (int)i;
	}

	return -1;
}


int AnalyzerFilter::GetAudioESCount(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return 0;

	return static_cast<int>(m_ServiceList[Index].AudioESList.size());
}


bool AnalyzerFilter::GetAudioESList(int Index, ReturnArg<ESInfoList> ESList) const
{
	if (!ESList)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	*ESList = m_ServiceList[Index].AudioESList;

	return true;
}


bool AnalyzerFilter::GetAudioESInfo(int Index, int AudioIndex, ReturnArg<ESInfo> ESInfo) const
{
	if (!ESInfo)
		return false;

	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(AudioIndex) >= m_ServiceList[Index].AudioESList.size()))
		return false;

	*ESInfo = m_ServiceList[Index].AudioESList[AudioIndex];

	return true;
}


uint16_t AnalyzerFilter::GetAudioESPID(int Index, int AudioIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(AudioIndex) >= m_ServiceList[Index].AudioESList.size()))
		return PID_INVALID;

	return m_ServiceList[Index].AudioESList[AudioIndex].PID;
}


uint8_t AnalyzerFilter::GetAudioStreamType(int Index, int AudioIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(AudioIndex) >= m_ServiceList[Index].AudioESList.size()))
		return STREAM_TYPE_INVALID;

	return m_ServiceList[Index].AudioESList[AudioIndex].StreamType;
}


uint8_t AnalyzerFilter::GetAudioComponentTag(int Index, int AudioIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(AudioIndex) >= m_ServiceList[Index].AudioESList.size()))
		return COMPONENT_TAG_INVALID;

	return m_ServiceList[Index].AudioESList[AudioIndex].ComponentTag;
}


int AnalyzerFilter::GetAudioIndexByComponentTag(int Index, uint8_t ComponentTag) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) < m_ServiceList.size()) {
		for (size_t i = 0; i < m_ServiceList[Index].AudioESList.size(); i++) {
			if (m_ServiceList[Index].AudioESList[i].ComponentTag == ComponentTag)
				return static_cast<int>(i);
		}
	}

	return -1;
}


#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT


uint8_t AnalyzerFilter::GetVideoComponentType(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) < m_ServiceList.size()) {
		const DescriptorBlock *pDescBlock = GetHEITItemDesc(Index);

		if (pDescBlock != nullptr) {
			const ComponentDescriptor *pComponentDesc = pDescBlock->GetDescriptor<ComponentDescriptor>();

			if (pComponentDesc != nullptr)
				return pComponentDesc->GetComponentType();
		}
	}

	return COMPONENT_TYPE_INVALID;
}


uint8_t AnalyzerFilter::GetAudioComponentType(int Index, int AudioIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) < m_ServiceList.size())
			&& (static_cast<unsigned int>(AudioIndex) < m_ServiceList[Index].AudioESList.size())) {
		const DescriptorBlock *pDescBlock = GetHEITItemDesc(Index);

		if (pDescBlock != nullptr) {
			const AudioComponentDescriptor *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[Index].AudioESList[AudioIndex].ComponentTag);
			if (pAudioDesc != nullptr)
				return pAudioDesc->GetComponentType();
		}
	}

	return COMPONENT_TYPE_INVALID;
}


bool AnalyzerFilter::GetAudioComponentText(int Index, int AudioIndex, ReturnArg<String> Text) const
{
	if (!Text)
		return false;

	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) < m_ServiceList.size())
			&& (static_cast<unsigned int>(AudioIndex) < m_ServiceList[Index].AudioESList.size())) {
		const DescriptorBlock *pDescBlock = GetHEITItemDesc(Index);

		if (pDescBlock) {
			const AudioComponentDescriptor *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[Index].AudioESList[AudioIndex].ComponentTag);

			if (pAudioDesc != nullptr) {
				ARIBString String;

				if (pAudioDesc->GetText(&String))
					return m_StringDecoder.Decode(String, Text);
			}
		}
	}

	Text->clear();

	return false;
}


#endif	// LIBISDB_ANALYZER_FILTER_EIT_SUPPORT


int AnalyzerFilter::GetCaptionESCount(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return 0;
	return (int)m_ServiceList[Index].CaptionESList.size();
}


uint16_t AnalyzerFilter::GetCaptionESPID(int Index, int CaptionIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(CaptionIndex) >= m_ServiceList[Index].CaptionESList.size()))
		return PID_INVALID;

	return m_ServiceList[Index].CaptionESList[CaptionIndex].PID;
}


int AnalyzerFilter::GetDataCarrouselESCount(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return 0;

	return static_cast<int>(m_ServiceList[Index].DataCarrouselESList.size());
}


uint16_t AnalyzerFilter::GetDataCarrouselESPID(int Index, int DataCarrouselIndex) const
{
	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(Index) >= m_ServiceList.size())
			|| (static_cast<unsigned int>(DataCarrouselIndex) >= m_ServiceList[Index].DataCarrouselESList.size()))
		return PID_INVALID;

	return m_ServiceList[Index].DataCarrouselESList[DataCarrouselIndex].PID;
}


uint16_t AnalyzerFilter::GetPCRPID(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return PID_INVALID;

	return m_ServiceList[Index].PCRPID;
}


uint64_t AnalyzerFilter::GetPCRTimeStamp(int Index) const
{
	BlockLock Lock(m_FilterLock);

	const uint16_t PCRPID = GetPCRPID(Index);

	if (PCRPID != PID_INVALID) {
		const PCRTable *pPCRTable =
			dynamic_cast<const PCRTable *>(m_PIDMapManager.GetMapTarget(PCRPID));
		if (pPCRTable != nullptr) {
			return pPCRTable->GetPCRTimeStamp();
		}
	}

	return PCR_INVALID;
}


bool AnalyzerFilter::GetServiceName(int Index, ReturnArg<String> Name) const
{
	if (!Name)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	*Name = m_ServiceList[Index].ServiceName;

	return true;
}


uint8_t AnalyzerFilter::GetServiceType(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return SERVICE_TYPE_INVALID;

	return m_ServiceList[Index].ServiceType;
}


uint16_t AnalyzerFilter::GetLogoID(int Index) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return LOGO_ID_INVALID;

	return m_ServiceList[Index].LogoID;
}


uint16_t AnalyzerFilter::GetTransportStreamID() const
{
	return m_TransportStreamID;
}


uint16_t AnalyzerFilter::GetNetworkID() const
{
	return m_NetworkID;
}


uint8_t AnalyzerFilter::GetBroadcastingID() const
{
	return m_NITInfo.BroadcastingID;
}


bool AnalyzerFilter::GetNetworkName(ReturnArg<String> Name) const
{
	if (!Name)
		return false;

	BlockLock Lock(m_FilterLock);

	*Name = m_NITInfo.NetworkName;

	return true;
}


uint8_t AnalyzerFilter::GetRemoteControlKeyID() const
{
	BlockLock Lock(m_FilterLock);

	return m_NITInfo.RemoteControlKeyID;
}


bool AnalyzerFilter::GetTSName(ReturnArg<String> Name) const
{
	if (!Name)
		return false;

	BlockLock Lock(m_FilterLock);

	*Name = m_NITInfo.TSName;

	return true;
}


bool AnalyzerFilter::GetServiceList(ReturnArg<ServiceList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_ServiceList;

	return true;
}


bool AnalyzerFilter::GetSDTServiceList(ReturnArg<SDTServiceList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_SDTServiceList;

	return true;
}


bool AnalyzerFilter::GetSDTStreamList(ReturnArg<SDTStreamList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	List->clear();
	List->reserve(m_SDTStreamMap.size());

	for (auto const &e : m_SDTStreamMap)
		List->push_back(e.second);

	return true;
}


bool AnalyzerFilter::GetNetworkStreamList(ReturnArg<NetworkStreamList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_NetworkStreamList;

	return true;
}


bool AnalyzerFilter::IsPATUpdated() const
{
	return m_PATUpdated;
}


bool AnalyzerFilter::IsSDTUpdated() const
{
	return m_SDTUpdated;
}


bool AnalyzerFilter::IsNITUpdated() const
{
	return m_NITUpdated;
}


#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
bool AnalyzerFilter::IsEITUpdated() const
{
	return m_EITUpdated;
}
#endif


bool AnalyzerFilter::IsSDTComplete() const
{
	BlockLock Lock(m_FilterLock);

	if (!m_SDTUpdated || !m_NITUpdated)
		return false;

	for (auto const &e : m_NetworkStreamList) {
		if (m_SDTStreamMap.find(SDTStreamMapKey(e.OriginalNetworkID, e.TransportStreamID)) == m_SDTStreamMap.end())
			return false;
	}

	return true;
}


bool AnalyzerFilter::Is1SegStream() const
{
	BlockLock Lock(m_FilterLock);

	if (m_ServiceList.empty())
		return false;

	for (auto const &e : m_ServiceList) {
		if (!Is1SegPMTPID(e.PMTPID))
			return false;
	}

	return true;
}


bool AnalyzerFilter::Has1SegService() const
{
	BlockLock Lock(m_FilterLock);

	for (auto const &e : m_ServiceList) {
		if (Is1SegPMTPID(e.PMTPID))
			return true;
	}

	return false;
}


uint16_t AnalyzerFilter::GetFirst1SegServiceID() const
{
	BlockLock Lock(m_FilterLock);

	uint16_t MinPID = 0xFFFF;
	size_t MinIndex;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (Is1SegPMTPID(m_ServiceList[i].PMTPID)
				&& (m_ServiceList[i].PMTPID < MinPID)) {
			MinPID = m_ServiceList[i].PMTPID;
			MinIndex = i;
		}
	}

	if (MinPID == 0xFFFF_u16)
		return PID_INVALID;

	return m_ServiceList[MinIndex].ServiceID;
}


uint16_t AnalyzerFilter::Get1SegServiceIDByIndex(int Index) const
{
	if ((Index < 0) || (Index >= ONESEG_PMT_PID_COUNT))
		return SERVICE_ID_INVALID;

	BlockLock Lock(m_FilterLock);

	uint16_t ServiceList[ONESEG_PMT_PID_COUNT] = {};

	for (auto &e : m_ServiceList) {
		if (Is1SegPMTPID(e.PMTPID))
			ServiceList[e.PMTPID - ONESEG_PMT_PID_FIRST] = e.ServiceID;
	}

	int ServiceCount = 0;
	for (int i = 0; i < ONESEG_PMT_PID_COUNT; i++) {
		if (ServiceList[i] != 0) {
			if (ServiceCount == Index)
				return ServiceList[i];
			ServiceCount++;
		}
	}

	return SERVICE_ID_INVALID;
}


#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT


uint16_t AnalyzerFilter::GetEventID(int ServiceIndex, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size()) {
		const EITTable *pEITTable = GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next);
		if (pEITTable != nullptr)
			return pEITTable->GetEventID();
	}

	return EVENT_ID_INVALID;
}


bool AnalyzerFilter::GetEventStartTime(int ServiceIndex, ReturnArg<DateTime> StartTime, bool Next) const
{
	if (!StartTime)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size()) {
		const EITTable *pEITTable = GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next);
		if (pEITTable != nullptr) {
			const DateTime *pStartTime = pEITTable->GetStartTime();
			if (pStartTime != nullptr) {
				*StartTime = *pStartTime;
				return true;
			}
		}
	}

	return false;
}


uint32_t AnalyzerFilter::GetEventDuration(int ServiceIndex, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size()) {
		const EITTable *pEITTable = GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next);
		if (pEITTable != nullptr) {
			return pEITTable->GetDuration();
		}
	}

	return 0;
}


bool AnalyzerFilter::GetEventTime(int ServiceIndex, OptionalReturnArg<DateTime> Time, OptionalReturnArg<uint32_t> Duration, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size()) {
		const EITTable *pEITTable = GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next);
		if (pEITTable != nullptr) {
			const DateTime *pStartTime = pEITTable->GetStartTime();
			if (pStartTime == nullptr)
				return false;
			if (Time)
				*Time = *pStartTime;
			if (Duration)
				*Duration = pEITTable->GetDuration();
			return true;
		}
	}

	return false;
}


bool AnalyzerFilter::GetEventName(int ServiceIndex, ReturnArg<String> Name, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);
	if (pDescBlock != nullptr) {
		const ShortEventDescriptor *pShortEvent = pDescBlock->GetDescriptor<ShortEventDescriptor>();

		if (pShortEvent != nullptr) {
			ARIBString Str;
			if (pShortEvent->GetEventName(&Str))
				return m_StringDecoder.Decode(Str, Name);
		}
	}

#ifdef LIBISDB_ANALYZER_FILTER_L_EIT_SUPPORT
	pDescBlock = GetLEITItemDesc(ServiceIndex, Next);
	if (pDescBlock != nullptr) {
		const ShortEventDescriptor *pShortEvent = pDescBlock->GetDescriptor<ShortEventDescriptor>();

		if (pShortEvent != nullptr) {
			ARIBString Str;
			if (pShortEvent->GetEventName(&Str))
				return m_StringDecoder.Decode(Str, Name);
		}
	}
#endif

	return false;
}


bool AnalyzerFilter::GetEventText(int ServiceIndex, ReturnArg<String> Text, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);
	if (pDescBlock != nullptr) {
		const ShortEventDescriptor *pShortEvent = pDescBlock->GetDescriptor<ShortEventDescriptor>();

		if (pShortEvent != nullptr) {
			ARIBString Str;
			if (pShortEvent->GetEventDescription(&Str))
				return m_StringDecoder.Decode(Str, Text);
		}
	}

#ifdef LIBISDB_ANALYZER_FILTER_L_EIT_SUPPORT
	pDescBlock = GetLEITItemDesc(ServiceIndex, Next);
	if (pDescBlock != nullptr) {
		const ShortEventDescriptor *pShortEvent = pDescBlock->GetDescriptor<ShortEventDescriptor>();

		if (pShortEvent != nullptr) {
			ARIBString Str;
			if (pShortEvent->GetEventDescription(&Str))
				return m_StringDecoder.Decode(Str, Text);
		}
	}
#endif

	return false;
}


const DescriptorBlock * AnalyzerFilter::GetExtendedEventDescriptor(int ServiceIndex, bool UseEventGroup, bool Next) const
{
	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);
	if (pDescBlock == nullptr)
		return nullptr;

	if (pDescBlock->GetDescriptorByTag(ExtendedEventDescriptor::TAG) == nullptr) {
		if (!UseEventGroup)
			return nullptr;

		// イベント共有の参照先から情報を取得する
		const EventGroupDescriptor *pEventGroup = pDescBlock->GetDescriptor<EventGroupDescriptor>();
		if ((pEventGroup == nullptr)
				|| (pEventGroup->GetGroupType() != EventGroupDescriptor::GROUP_TYPE_COMMON)
				|| (pEventGroup->GetEventCount() < 1))
			return nullptr;

		// 自己の記述がない場合は参照元
		const uint16_t EventID = GetEventID(ServiceIndex, Next);
		for (int i = 0; i < pEventGroup->GetEventCount(); i++) {
			EventGroupDescriptor::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)
					&& (EventInfo.ServiceID == m_ServiceList[ServiceIndex].ServiceID)
					&& (EventInfo.EventID == EventID))
				return nullptr;
		}

		const EITPfActualTable *pEITPfTable =
			m_PIDMapManager.GetMapTarget<EITPfActualTable>(PID_HEIT);
		int i;

		for (i = 0; i < pEventGroup->GetEventCount(); i++) {
			EventGroupDescriptor::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)) {
				int Index = GetServiceIndexByID(EventInfo.ServiceID);
				if (Index >= 0) {
					const EITTable *pEITTable = pEITPfTable->GetPfActualTable(EventInfo.ServiceID, Next);
					if ((pEITTable == nullptr)
							|| (pEITTable->GetEventID() != EventInfo.EventID)
							|| ((pDescBlock = GetHEITItemDesc(Index, Next)) == nullptr)
							|| (pDescBlock->GetDescriptorByTag(ExtendedEventDescriptor::TAG) == nullptr))
						return nullptr;
					break;
				}
			}
		}

		if (i == pEventGroup->GetEventCount())
			return nullptr;
	}

	return pDescBlock;
}


bool AnalyzerFilter::GetEventExtendedText(int ServiceIndex, ReturnArg<String> Text, bool UseEventGroup, bool Next) const
{
	if (!Text)
		return false;

	Text->clear();

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetExtendedEventDescriptor(ServiceIndex, UseEventGroup, Next);
	if (pDescBlock == nullptr)
		return false;

	return LibISDB::GetEventExtendedText(pDescBlock, m_StringDecoder, ARIBStringDecoder::DecodeFlag::UseCharSize, Text);
}


bool AnalyzerFilter::GetEventExtendedText(int ServiceIndex, ReturnArg<EventInfo::ExtendedTextInfoList> List, bool UseEventGroup, bool Next) const
{
	if (!List)
		return false;

	List->clear();

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetExtendedEventDescriptor(ServiceIndex, UseEventGroup, Next);
	if (pDescBlock == nullptr)
		return false;

	return GetEventExtendedTextList(pDescBlock, m_StringDecoder, ARIBStringDecoder::DecodeFlag::UseCharSize, List);
}


bool AnalyzerFilter::GetEventVideoInfo(int ServiceIndex, int VideoIndex, ReturnArg<EventVideoInfo> Info, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size())
			&& (static_cast<unsigned int>(VideoIndex) < m_ServiceList[ServiceIndex].VideoESList.size())) {
		const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

		if (pDescBlock != nullptr) {
			const ComponentDescriptor *pComponentDesc =
				GetComponentDescByComponentTag(pDescBlock, m_ServiceList[ServiceIndex].VideoESList[VideoIndex].ComponentTag);
			if (pComponentDesc != nullptr) {
				ComponentDescToVideoInfo(pComponentDesc, Info);
				return true;
			}
		}
	}

	return false;
}


bool AnalyzerFilter::GetEventVideoList(int ServiceIndex, ReturnArg<EventVideoList> List, bool Next) const
{
	if (!List)
		return false;

	List->clear();

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		pDescBlock->EnumDescriptors<ComponentDescriptor>(
			[&](const ComponentDescriptor *pComponentDesc) {
				EventVideoInfo VideoInfo;

				ComponentDescToVideoInfo(pComponentDesc, &VideoInfo);
				List->push_back(VideoInfo);
			});
		return true;
	}

	return false;
}


bool AnalyzerFilter::GetEventAudioInfo(int ServiceIndex, int AudioIndex, ReturnArg<EventAudioInfo> Info, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if ((static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size())
			&& (static_cast<unsigned int>(AudioIndex) < m_ServiceList[ServiceIndex].AudioESList.size())) {
		const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

		if (pDescBlock != nullptr) {
			const AudioComponentDescriptor *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[ServiceIndex].AudioESList[AudioIndex].ComponentTag);
			if (pAudioDesc != nullptr) {
				AudioComponentDescToAudioInfo(pAudioDesc, Info);
				return true;
			}
		}
	}

	return false;
}


bool AnalyzerFilter::GetEventAudioList(int ServiceIndex, ReturnArg<EventAudioList> List, bool Next) const
{
	if (!List)
		return false;

	List->clear();

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		pDescBlock->EnumDescriptors<AudioComponentDescriptor>(
			[&](const AudioComponentDescriptor *pAudioComponent) {
				EventAudioInfo AudioInfo;

				AudioComponentDescToAudioInfo(pAudioComponent, &AudioInfo);
				List->push_back(AudioInfo);
			});
		return true;
	}

	return false;
}


bool AnalyzerFilter::GetEventContentNibble(int ServiceIndex, ReturnArg<EventContentNibble> Info, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		const ContentDescriptor *pContentDesc = pDescBlock->GetDescriptor<ContentDescriptor>();

		if (pContentDesc != nullptr) {
			Info->NibbleCount = pContentDesc->GetNibbleCount();
			for (int i = 0; i < Info->NibbleCount; i++)
				pContentDesc->GetNibble(i, &Info->NibbleList[i]);
			return true;
		}
	}

	return false;
}


bool AnalyzerFilter::GetEventSeriesInfo(int ServiceIndex, ReturnArg<EventSeriesInfo> Info, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		const SeriesDescriptor *pSeriesDesc = pDescBlock->GetDescriptor<SeriesDescriptor>();

		if (pSeriesDesc != nullptr) {
			Info->SeriesID = pSeriesDesc->GetSeriesID();
			Info->RepeatLabel = pSeriesDesc->GetRepeatLabel();
			Info->ProgramPattern = pSeriesDesc->GetProgramPattern();
			if (pSeriesDesc->IsExpireDateValid())
				pSeriesDesc->GetExpireDate(&Info->ExpireDate);
			else
				Info->ExpireDate.Reset();
			Info->EpisodeNumber = pSeriesDesc->GetEpisodeNumber();
			Info->LastEpisodeNumber = pSeriesDesc->GetLastEpisodeNumber();
			ARIBString Name;
			pSeriesDesc->GetSeriesName(&Name);
			m_StringDecoder.Decode(Name, &Info->SeriesName);

			return true;
		}
	}

	return false;
}


bool AnalyzerFilter::GetEventInfo(int ServiceIndex, ReturnArg<EventInfo> Info, bool UseEventGroup, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	if (static_cast<unsigned int>(ServiceIndex) >= m_ServiceList.size())
		return false;

	const EITTable *pEITTable =
		GetEITPfTableByServiceID(m_ServiceList[ServiceIndex].ServiceID, Next);
	if (pEITTable == nullptr)
		return false;

	Info->NetworkID = pEITTable->GetOriginalNetworkID();
	Info->TransportStreamID = pEITTable->GetTransportStreamID();
	Info->ServiceID = pEITTable->GetServiceID();
	Info->EventID = pEITTable->GetEventID();
	if (const DateTime *pStartTime = pEITTable->GetStartTime(); pStartTime != nullptr)
		Info->StartTime = *pStartTime;
	else
		Info->StartTime.Reset();
	Info->Duration = pEITTable->GetDuration();
	Info->RunningStatus = pEITTable->GetRunningStatus();
	Info->FreeCAMode = pEITTable->GetFreeCAMode();

	if (!GetEventName(ServiceIndex, &Info->EventName, Next))
		Info->EventName.clear();
	if (!GetEventText(ServiceIndex, &Info->EventText, Next))
		Info->EventText.clear();
	if (!GetEventExtendedText(ServiceIndex, &Info->ExtendedText, UseEventGroup, Next))
		Info->ExtendedText.clear();

	GetEventVideoList(ServiceIndex, &Info->VideoList, Next);
	GetEventAudioList(ServiceIndex, &Info->AudioList, Next);

	if (!GetEventContentNibble(ServiceIndex, &Info->ContentNibble, Next))
		Info->ContentNibble.NibbleCount = 0;

	Info->Type = EventInfo::TypeFlag::Basic | EventInfo::TypeFlag::Extended
		| (Next ? EventInfo::TypeFlag::Following : EventInfo::TypeFlag::Present);

	return true;
}


int AnalyzerFilter::GetEventComponentGroupCount(int ServiceIndex, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		const ComponentGroupDescriptor *pComponentGroupDesc = pDescBlock->GetDescriptor<ComponentGroupDescriptor>();

		if (pComponentGroupDesc != nullptr)
			return pComponentGroupDesc->GetGroupCount();
	}

	return 0;
}


bool AnalyzerFilter::GetEventComponentGroupInfo(int ServiceIndex, int GroupIndex, ReturnArg<EventComponentGroupInfo> Info, bool Next) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		const ComponentGroupDescriptor *pComponentGroupDesc = pDescBlock->GetDescriptor<ComponentGroupDescriptor>();

		if (pComponentGroupDesc != nullptr) {
			const ComponentGroupDescriptor::GroupInfo *pGroup = pComponentGroupDesc->GetGroupInfo(GroupIndex);

			if (pGroup != nullptr) {
				Info->ComponentGroupID = pGroup->ComponentGroupID;
				Info->NumOfCAUnit = pGroup->NumOfCAUnit;
				std::memcpy(Info->CAUnitList, pGroup->CAUnitList, pGroup->NumOfCAUnit * sizeof(ComponentGroupDescriptor::CAUnitInfo));
				Info->TotalBitRate = pGroup->TotalBitRate;
				m_StringDecoder.Decode(pGroup->Text, &Info->Text);

				return true;
			}
		}
	}

	return false;
}


bool AnalyzerFilter::GetEventComponentGroupList(int ServiceIndex, ReturnArg<EventComponentGroupList> List, bool Next) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		const ComponentGroupDescriptor *pComponentGroupDesc = pDescBlock->GetDescriptor<ComponentGroupDescriptor>();

		if (pComponentGroupDesc != nullptr) {
			const int GroupCount = pComponentGroupDesc->GetGroupCount();

			List->clear();
			List->reserve(GroupCount);

			for (int i = 0; i < GroupCount; i++) {
				const ComponentGroupDescriptor::GroupInfo *pGroup = pComponentGroupDesc->GetGroupInfo(i);

				if (pGroup != nullptr) {
					List->emplace_back();
					EventComponentGroupInfo &Info = List->back();

					Info.ComponentGroupID = pGroup->ComponentGroupID;
					Info.NumOfCAUnit = pGroup->NumOfCAUnit;
					std::memcpy(Info.CAUnitList, pGroup->CAUnitList, pGroup->NumOfCAUnit * sizeof(ComponentGroupDescriptor::CAUnitInfo));
					Info.TotalBitRate = pGroup->TotalBitRate;
					m_StringDecoder.Decode(pGroup->Text, &Info.Text);
				}
			}

			return true;
		}
	}

	return false;
}


int AnalyzerFilter::GetEventComponentGroupIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag, bool Next) const
{
	BlockLock Lock(m_FilterLock);

	const DescriptorBlock *pDescBlock = GetHEITItemDesc(ServiceIndex, Next);

	if (pDescBlock != nullptr) {
		const ComponentGroupDescriptor *pComponentGroupDesc = pDescBlock->GetDescriptor<ComponentGroupDescriptor>();

		if (pComponentGroupDesc != nullptr) {
			const int GroupCount = pComponentGroupDesc->GetGroupCount();

			for (int i = 0; i < GroupCount; i++) {
				const ComponentGroupDescriptor::GroupInfo *pInfo = pComponentGroupDesc->GetGroupInfo(i);

				if (pInfo != nullptr) {
					for (int j = 0; j < pInfo->NumOfCAUnit; j++) {
						for (int k = 0; k < pInfo->CAUnitList[j].NumOfComponent; k++) {
							if (pInfo->CAUnitList[j].ComponentTag[k] == ComponentTag)
								return i;
						}
					}
				}
			}
		}
	}

	return -1;
}


const EITTable *AnalyzerFilter::GetEITPfTableByServiceID(uint16_t ServiceID, bool Next) const
{
	const EITPfActualTable *pEITPfTable =
		m_PIDMapManager.GetMapTarget<EITPfActualTable>(PID_HEIT);
	if (pEITPfTable != nullptr) {
		const EITTable *pEITTable = pEITPfTable->GetPfActualTable(ServiceID, Next);
		if (pEITTable != nullptr)
			return pEITTable;
	}

#ifdef LIBISDB_ANALYZER_FILTER_L_EIT_SUPPORT
	pEITPfTable = m_PIDMapManager.GetMapTarget<EITPfActualTable>(PID_LEIT);
	if (pEITPfTable != nullptr) {
		const EITTable *pEITTable = pEITPfTable->GetPfActualTable(ServiceID, Next);
		if (pEITTable != nullptr)
			return pEITTable;
	}
#endif

	return nullptr;
}


const DescriptorBlock *AnalyzerFilter::GetHEITItemDesc(int ServiceIndex, bool Next) const
{
	if (static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size()) {
		const EITPfActualTable *pEITPfTable =
			m_PIDMapManager.GetMapTarget<EITPfActualTable>(PID_HEIT);

		if (pEITPfTable != nullptr) {
			const EITTable *pEITTable =
				pEITPfTable->GetPfActualTable(m_ServiceList[ServiceIndex].ServiceID, Next);
			if (pEITTable != nullptr)
				return pEITTable->GetItemDescriptorBlock();
		}
	}

	return nullptr;
}


#ifdef LIBISDB_ANALYZER_FILTER_L_EIT_SUPPORT
const DescriptorBlock *AnalyzerFilter::GetLEITItemDesc(int ServiceIndex, bool Next) const
{
	if (static_cast<unsigned int>(ServiceIndex) < m_ServiceList.size()) {
		const EITPfActualTable *pEITPfTable =
			m_PIDMapManager.GetMapTarget<EITPfActualTable>(PID_LEIT);

		if (pEITPfTable != nullptr) {
			const EITTable *pEITTable =
				pEITPfTable->GetPfActualTable(m_ServiceList[ServiceIndex].ServiceID, Next);
			if (pEITTable != nullptr)
				return pEITTable->GetItemDescriptorBlock();
		}
	}

	return nullptr;
}
#endif


const ComponentDescriptor *AnalyzerFilter::GetComponentDescByComponentTag(const DescriptorBlock *pDescBlock, uint8_t ComponentTag) const
{
	if (pDescBlock != nullptr) {
		for (int i = 0; i < pDescBlock->GetDescriptorCount(); i++) {
			const DescriptorBase *pDesc = pDescBlock->GetDescriptorByIndex(i);

			if (pDesc->GetTag() == ComponentDescriptor::TAG) {
				const ComponentDescriptor *pComponentDesc = dynamic_cast<const ComponentDescriptor *>(pDesc);

				if ((pComponentDesc != nullptr)
						&& (pComponentDesc->GetComponentTag() == ComponentTag))
					return pComponentDesc;
			}
		}
	}

	return nullptr;
}


const AudioComponentDescriptor *AnalyzerFilter::GetAudioComponentDescByComponentTag(const DescriptorBlock *pDescBlock, uint8_t ComponentTag) const
{
	if (pDescBlock != nullptr) {
		for (int i = 0; i < pDescBlock->GetDescriptorCount(); i++) {
			const DescriptorBase *pDesc = pDescBlock->GetDescriptorByIndex(i);

			if (pDesc->GetTag() == AudioComponentDescriptor::TAG) {
				const AudioComponentDescriptor *pAudioDesc = dynamic_cast<const AudioComponentDescriptor *>(pDesc);

				if ((pAudioDesc != nullptr)
						&& (pAudioDesc->GetComponentTag() == ComponentTag))
					return pAudioDesc;
			}
		}
	}

	return nullptr;
}


void AnalyzerFilter::ComponentDescToVideoInfo(const ComponentDescriptor *pComponentDesc, ReturnArg<EventVideoInfo> Info) const
{
	Info->StreamContent = pComponentDesc->GetStreamContent();
	Info->ComponentType = pComponentDesc->GetComponentType();
	Info->ComponentTag = pComponentDesc->GetComponentTag();
	Info->LanguageCode = pComponentDesc->GetLanguageCode();
	ARIBString Text;
	pComponentDesc->GetText(&Text);
	m_StringDecoder.Decode(Text, &Info->Text);
}


void AnalyzerFilter::AudioComponentDescToAudioInfo(const AudioComponentDescriptor *pAudioDesc, ReturnArg<EventAudioInfo> Info) const
{
	Info->StreamContent = pAudioDesc->GetStreamContent();
	Info->ComponentType = pAudioDesc->GetComponentType();
	Info->ComponentTag = pAudioDesc->GetComponentTag();
	Info->SimulcastGroupTag = pAudioDesc->GetSimulcastGroupTag();
	Info->ESMultiLingualFlag = pAudioDesc->GetESMultiLingualFlag();
	Info->MainComponentFlag = pAudioDesc->GetMainComponentFlag();
	Info->QualityIndicator = pAudioDesc->GetQualityIndicator();
	Info->SamplingRate = pAudioDesc->GetSamplingRate();
	Info->LanguageCode = pAudioDesc->GetLanguageCode();
	Info->LanguageCode2 = pAudioDesc->GetLanguageCode2();
	ARIBString Text;
	pAudioDesc->GetText(&Text);
	m_StringDecoder.Decode(Text, &Info->Text);
}


#endif	// LIBISDB_ANALYZER_FILTER_EIT_SUPPORT


bool AnalyzerFilter::GetTOTTime(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	BlockLock Lock(m_FilterLock);

	const TOTTable *pTOTTable = m_PIDMapManager.GetMapTarget<TOTTable>(PID_TOT);
	if (pTOTTable == nullptr)
		return false;

	return pTOTTable->GetDateTime(Time);
}


bool AnalyzerFilter::GetInterpolatedTOTTime(ReturnArg<DateTime> Time, OptionalReturnArg<bool> Interpolated) const
{
	// PCRで補間したTOT時刻を取得する
	BlockLock Lock(m_FilterLock);

	if (!GetTOTTime(Time))
		return false;

	Interpolated = false;

	if (m_TOTInterpolation.PCRPID != PID_INVALID) {
		for (size_t i = 0; i < m_ServiceList.size(); i++) {
			if (m_ServiceList[i].PCRPID == m_TOTInterpolation.PCRPID) {
				uint64_t PCRTime = GetPCRTimeStamp(static_cast<int>(i));

				if (PCRTime != PCR_INVALID) {
					long long Diff;

					if (PCRTime >= m_TOTInterpolation.PCRTime)
						Diff = PCRTime - m_TOTInterpolation.PCRTime;
					else
						Diff = (0x200000000ULL - m_TOTInterpolation.PCRTime) + PCRTime;
					if (Diff <= 15 * 90000LL) {	// 最大15秒
						Time->OffsetMilliseconds(Diff / 90LL);
						Interpolated = true;
					}
				}
				break;
			}
		}
	}

	return true;
}


bool AnalyzerFilter::GetSatelliteDeliverySystemList(ReturnArg<SatelliteDeliverySystemList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	List->clear();

	const NITMultiTable *pNITMultiTable =
		m_PIDMapManager.GetMapTarget<NITMultiTable>(PID_NIT);
	if ((pNITMultiTable == nullptr) || !pNITMultiTable->IsNITComplete())
		return false;

	for (uint16_t SectionNo = 0; SectionNo < pNITMultiTable->GetNITSectionCount(); SectionNo++) {
		const NITTable *pNITTable = pNITMultiTable->GetNITTable(SectionNo);
		if (pNITTable == nullptr)
			continue;

		for (int i = 0; i < pNITTable->GetTransportStreamCount(); i++) {
			const DescriptorBlock *pDescBlock = pNITTable->GetItemDescriptorBlock(i);

			if (pDescBlock != nullptr) {
				const SatelliteDeliverySystemDescriptor *pSatelliteDesc =
					pDescBlock->GetDescriptor<SatelliteDeliverySystemDescriptor>();

				if (pSatelliteDesc != nullptr) {
					SatelliteDeliverySystemInfo Info;

					Info.TransportStreamID = pNITTable->GetTransportStreamID(i);
					Info.Frequency = pSatelliteDesc->GetFrequency();
					Info.OrbitalPosition = pSatelliteDesc->GetOrbitalPosition();
					Info.WestEastFlag = pSatelliteDesc->GetWestEastFlag();
					Info.Polarization = pSatelliteDesc->GetPolarization();
					Info.Modulation = pSatelliteDesc->GetModulation();
					Info.SymbolRate = pSatelliteDesc->GetSymbolRate();
					Info.FECInner = pSatelliteDesc->GetFECInner();

					List->push_back(Info);
				}
			}
		}
	}

	return !List->empty();
}


bool AnalyzerFilter::GetTerrestrialDeliverySystemList(ReturnArg<TerrestrialDeliverySystemList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	List->clear();

	const NITMultiTable *pNITMultiTable =
		dynamic_cast<const NITMultiTable*>(m_PIDMapManager.GetMapTarget(PID_NIT));
	if ((pNITMultiTable == nullptr) || !pNITMultiTable->IsNITComplete())
		return false;

	for (uint16_t SectionNo = 0; SectionNo < pNITMultiTable->GetNITSectionCount(); SectionNo++) {
		const NITTable *pNITTable = pNITMultiTable->GetNITTable(SectionNo);
		if (pNITTable == nullptr)
			continue;

		for (int i = 0; i < pNITTable->GetTransportStreamCount(); i++) {
			const DescriptorBlock *pDescBlock = pNITTable->GetItemDescriptorBlock(i);
			if (pDescBlock != nullptr) {
				const TerrestrialDeliverySystemDescriptor *pTerrestrialDesc =
					pDescBlock->GetDescriptor<TerrestrialDeliverySystemDescriptor>();
				if (pTerrestrialDesc != nullptr) {
					TerrestrialDeliverySystemInfo Info;

					Info.TransportStreamID = pNITTable->GetTransportStreamID(i);
					Info.AreaCode = pTerrestrialDesc->GetAreaCode();
					Info.GuardInterval = pTerrestrialDesc->GetGuardInterval();
					Info.TransmissionMode = pTerrestrialDesc->GetTransmissionMode();
					Info.Frequency.resize(pTerrestrialDesc->GetFrequencyCount());
					for (int j = 0; j < static_cast<int>(Info.Frequency.size()); j++)
						Info.Frequency[j] = pTerrestrialDesc->GetFrequency(j);

					List->push_back(Info);
				}
			}
		}
	}

	return !List->empty();
}


bool AnalyzerFilter::GetEMMPIDList(ReturnArg<EMMPIDList> List) const
{
	if (!List)
		return false;

	BlockLock Lock(m_FilterLock);

	*List = m_EMMPIDList;

	return true;
}


bool AnalyzerFilter::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool AnalyzerFilter::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


void AnalyzerFilter::OnPATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PAT が更新された
	LIBISDB_TRACE(LIBISDB_STR("AnalyzerFilter::OnPATSection()\n"));

	const PATTable *pPATTable = dynamic_cast<const PATTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPATTable == nullptr))
		return;

	// ここで引っ掛かる場合、SDT と PAT で transport_stream_id が違っている
	LIBISDB_ASSERT((m_TransportStreamID == TRANSPORT_STREAM_ID_INVALID) || (m_TransportStreamID == pPATTable->GetTransportStreamID()));

	// トランスポートストリームID更新
	m_TransportStreamID = pPATTable->GetTransportStreamID();

	// 現 PMT/PCR の PID をアンマップする
	for (auto const &e : m_ServiceList) {
		m_PIDMapManager.UnmapTarget(e.PMTPID);
		m_PIDMapManager.UnmapTarget(e.PCRPID);
	}

	// 新 PMT をストアする
	const int ServiceCount = pPATTable->GetProgramCount();
	m_ServiceList.resize(ServiceCount);

	for (int i = 0; i < ServiceCount; i++) {
		// サービスリスト更新
		ServiceInfo &Info = m_ServiceList[i];

		Info.IsPMTAcquired = false;
		Info.ServiceID = pPATTable->GetProgramNumber(i);
		Info.PMTPID = pPATTable->GetPMTPID(i);
		Info.ESList.clear();
		Info.VideoESList.clear();
		Info.AudioESList.clear();
		Info.CaptionESList.clear();
		Info.DataCarrouselESList.clear();
		Info.OtherESList.clear();
		Info.PCRPID = PID_INVALID;
		Info.ECMList.clear();
		Info.RunningStatus = 0xFF;
		Info.FreeCAMode = false;
		Info.ProviderName.clear();
		Info.ServiceName.clear();
		Info.ServiceType = SERVICE_TYPE_INVALID;
		Info.LogoID = LOGO_ID_INVALID;

		// PMT の PID をマップ
		m_PIDMapManager.MapTarget(
			Info.PMTPID,
			PSITableBase::CreateWithHandler<PMTTable>(&AnalyzerFilter::OnPMTSection, this));
	}

#ifdef LIBISDB_ENABLE_TRACE
	LIBISDB_TRACE(LIBISDB_STR("transport_stream_id : %04X\n"), m_TransportStreamID);
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		const ServiceInfo &Info = m_ServiceList[i];
		LIBISDB_TRACE(
			LIBISDB_STR("Service[%2zu] : service_id %04X / PMT PID  %04X\n"),
			i, Info.ServiceID, Info.PMTPID);
	}
#endif

#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
	// PAT が来る前に EIT が来ていた場合ここで通知する
	if (!m_PATUpdated && m_EITUpdated)
		m_SendEITUpdatedEvent = true;
#endif

	m_PATUpdated = true;

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnPATUpdated, this);
	m_FilterLock.Lock();
}


void AnalyzerFilter::GetSDTServiceInfo(ServiceInfo *pServiceInfo, const SDTTable *pSDTTable, int SDTIndex)
{
	// SDTからサービス情報を取得

	// サービス情報更新
	pServiceInfo->RunningStatus = pSDTTable->GetRunningStatus(SDTIndex);
	pServiceInfo->FreeCAMode = pSDTTable->GetFreeCAMode(SDTIndex);
	pServiceInfo->ProviderName.clear();
	pServiceInfo->ServiceName.clear();
	pServiceInfo->ServiceType = SERVICE_TYPE_INVALID;
	pServiceInfo->LogoID = LOGO_ID_INVALID;

	const DescriptorBlock *pDescBlock = pSDTTable->GetItemDescriptorBlock(SDTIndex);

	if (pDescBlock != nullptr) {
		// サービス名更新
		const ServiceDescriptor *pServiceDesc = pDescBlock->GetDescriptor<ServiceDescriptor>();
		if (pServiceDesc != nullptr) {
			ARIBString Name;
			if (pServiceDesc->GetProviderName(&Name))
				m_StringDecoder.Decode(Name, &pServiceInfo->ProviderName);
			if (pServiceDesc->GetServiceName(&Name))
				m_StringDecoder.Decode(Name, &pServiceInfo->ServiceName);
			pServiceInfo->ServiceType = pServiceDesc->GetServiceType();
		}

		// ロゴID更新
		const LogoTransmissionDescriptor *pLogoDesc = pDescBlock->GetDescriptor<LogoTransmissionDescriptor>();
		if (pLogoDesc != nullptr) {
			pServiceInfo->LogoID = pLogoDesc->GetLogoID();
		}
	}
}


void AnalyzerFilter::OnPMTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PMTが更新された
	LIBISDB_TRACE(LIBISDB_STR("AnalyzerFilter::OnPMTSection()\n"));

	const PMTTable *pPMTTable = dynamic_cast<const PMTTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pPMTTable == nullptr))
		return;

	// サービスインデックスを検索
	const uint16_t ServiceID = pPMTTable->GetProgramNumberID();
	const int ServiceIndex = GetServiceIndexByID(ServiceID);
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = m_ServiceList[ServiceIndex];

	// ESのPIDをストア
	Info.ESList.clear();
	Info.VideoESList.clear();
	Info.AudioESList.clear();
	Info.CaptionESList.clear();
	Info.DataCarrouselESList.clear();
	Info.OtherESList.clear();

	for (int ESIndex = 0; ESIndex < pPMTTable->GetESCount(); ESIndex++) {
		ESInfo ES;

		ES.PID = pPMTTable->GetESPID(ESIndex);
		ES.StreamType = pPMTTable->GetStreamType(ESIndex);

		const DescriptorBlock *pDescBlock = pPMTTable->GetItemDescriptorBlock(ESIndex);
		if (pDescBlock != nullptr) {
			const StreamIDDescriptor *pStreamIDDesc = pDescBlock->GetDescriptor<StreamIDDescriptor>();
			if (pStreamIDDesc != nullptr)
				ES.ComponentTag = pStreamIDDesc->GetComponentTag();

			const HierarchicalTransmissionDescriptor *pHierarchicalDesc =
				pDescBlock->GetDescriptor<HierarchicalTransmissionDescriptor>();
			if (pHierarchicalDesc != nullptr) {
				ES.QualityLevel = pHierarchicalDesc->GetQualityLevel();
				ES.HierarchicalReferencePID = pHierarchicalDesc->GetReferencePID();
			}
		}

		Info.ESList.push_back(ES);

		switch (ES.StreamType) {
		case STREAM_TYPE_MPEG1_VIDEO:
		case STREAM_TYPE_MPEG2_VIDEO:
		case STREAM_TYPE_MPEG4_VISUAL:
		case STREAM_TYPE_H264:
		case STREAM_TYPE_H265:
			Info.VideoESList.push_back(ES);
			break;

		case STREAM_TYPE_MPEG1_AUDIO:
		case STREAM_TYPE_MPEG2_AUDIO:
		case STREAM_TYPE_AAC:
		case STREAM_TYPE_MPEG4_AUDIO:
		case STREAM_TYPE_AC3:
		case STREAM_TYPE_DTS:
		case STREAM_TYPE_TRUEHD:
		case STREAM_TYPE_DOLBY_DIGITAL_PLUS:
			Info.AudioESList.push_back(ES);
			break;

		case STREAM_TYPE_CAPTION:
			Info.CaptionESList.push_back(ES);
			break;

		case STREAM_TYPE_DATA_CARROUSEL:
			Info.DataCarrouselESList.push_back(ES);
			break;

		default:
			Info.OtherESList.push_back(ES);
			break;
		}
	}

	// component_tag 順にソート
	auto ComponentTagCmp =
		[](const ESInfo &Info1, const ESInfo &Info2) -> bool {
			return Info1.ComponentTag < Info2.ComponentTag;
		};
	InsertionSort(Info.VideoESList, ComponentTagCmp);
	InsertionSort(Info.AudioESList, ComponentTagCmp);
	InsertionSort(Info.CaptionESList, ComponentTagCmp);
	InsertionSort(Info.DataCarrouselESList, ComponentTagCmp);

	if (uint16_t PCRPID = pPMTTable->GetPCRPID(); PCRPID < 0x1FFF) {
		Info.PCRPID = PCRPID;
		if (m_PIDMapManager.GetMapTarget(PCRPID) == nullptr)
			m_PIDMapManager.MapTarget(PCRPID, new PCRTable);
	}

	// ECM
	if (const DescriptorBlock *pPMTDesc = pPMTTable->GetPMTDescriptorBlock(); pPMTDesc != nullptr) {
		pPMTDesc->EnumDescriptors<CADescriptor>(
			[&Info](const CADescriptor *pCADesc) {
				ECMInfo ECM;

				ECM.CASystemID = pCADesc->GetCASystemID();
				ECM.PID = pCADesc->GetCAPID();

				Info.ECMList.push_back(ECM);
			});
	}

	// 更新済みマーク
	Info.IsPMTAcquired = true;

	// SDTからサービス情報を取得
	const SDTTableSet *pSDTTableSet = dynamic_cast<const SDTTableSet *>(m_PIDMapManager.GetMapTarget(PID_SDT));
	if (pSDTTableSet != nullptr) {
		const SDTTable *pSDTTable = pSDTTableSet->GetActualSDTTable();
		if (pSDTTable != nullptr) {
			const int SDTIndex = pSDTTable->GetServiceIndexByID(Info.ServiceID);
			if (SDTIndex >= 0)
				GetSDTServiceInfo(&Info, pSDTTable, SDTIndex);
		}
	}

#ifdef LIBISDB_ENABLE_TRACE
	LIBISDB_TRACE(LIBISDB_STR("service_id : %04X\n"), Info.ServiceID);
	for (size_t i = 0; i < Info.ESList.size(); i++) {
		const ESInfo &ES = Info.ESList[i];
		LIBISDB_TRACE(LIBISDB_STR("ES[%2zu] : PID %04X / stream_type %02X\n"), i, ES.PID, ES.StreamType);
	}
#endif

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnPMTUpdated, this, ServiceID);
	m_FilterLock.Lock();
}


void AnalyzerFilter::UpdateSDTServiceList(const SDTTable *pSDTTable, ReturnArg<SDTServiceList> List)
{
	ARIBString Name;

	List->resize(pSDTTable->GetServiceCount());

	for (int SDTIndex = 0; SDTIndex < pSDTTable->GetServiceCount(); SDTIndex++) {
		SDTServiceInfo &Service = (*List)[SDTIndex];

		Service.ServiceID = pSDTTable->GetServiceID(SDTIndex);
		Service.RunningStatus = pSDTTable->GetRunningStatus(SDTIndex);
		Service.FreeCAMode = pSDTTable->GetFreeCAMode(SDTIndex);

		Service.ProviderName.clear();
		Service.ServiceName.clear();
		Service.ServiceType = SERVICE_TYPE_INVALID;
		Service.LogoID = LOGO_ID_INVALID;

		const DescriptorBlock *pDescBlock = pSDTTable->GetItemDescriptorBlock(SDTIndex);
		if (pDescBlock != nullptr) {
			const ServiceDescriptor *pServiceDesc = pDescBlock->GetDescriptor<ServiceDescriptor>();
			if (pServiceDesc != nullptr) {
				if (pServiceDesc->GetProviderName(&Name))
					m_StringDecoder.Decode(Name, &Service.ProviderName);
				if (pServiceDesc->GetServiceName(&Name))
					m_StringDecoder.Decode(Name, &Service.ServiceName);
				Service.ServiceType = pServiceDesc->GetServiceType();
			}

			const LogoTransmissionDescriptor *pLogoDesc = pDescBlock->GetDescriptor<LogoTransmissionDescriptor>();
			if (pLogoDesc != nullptr) {
				Service.LogoID = pLogoDesc->GetLogoID();
			}
		}
	}
}


void AnalyzerFilter::UpdateSDTStreamMap(const SDTTable *pSDTTable, SDTStreamMap *pStreamMap)
{
	const uint16_t TransportStreamID = pSDTTable->GetTransportStreamID();
	const uint16_t NetworkID = pSDTTable->GetNetworkID();

	auto Result = pStreamMap->emplace(
		std::piecewise_construct,
		std::forward_as_tuple(SDTStreamMapKey(NetworkID, TransportStreamID)),
		std::forward_as_tuple());

	SDTStreamInfo &Info = Result.first->second;

	Info.TransportStreamID = TransportStreamID;
	Info.OriginalNetworkID = NetworkID;

	UpdateSDTServiceList(pSDTTable, &Info.ServiceList);
}


void AnalyzerFilter::OnSDTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// SDT が更新された
	LIBISDB_TRACE(LIBISDB_STR("AnalyzerFilter::OnSDTSection()\n"));

	const SDTTableSet *pTableSet = dynamic_cast<const SDTTableSet *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pTableSet == nullptr))
		return;

	const uint8_t TableID = pTableSet->GetLastUpdatedTableID();

	if (TableID == SDTTable::TABLE_ID_ACTUAL) {
		// 現在の TS の SDT
		const SDTTable *pSDTTable = pTableSet->GetActualSDTTable();
		if (pSDTTable == nullptr)
			return;

		// ここで引っ掛かる場合、PAT と SDT で transport_stream_id が違っている
		LIBISDB_ASSERT((m_TransportStreamID == TRANSPORT_STREAM_ID_INVALID) || (m_TransportStreamID == pSDTTable->GetTransportStreamID()));
		// ここで引っ掛かる場合、NIT と SDT で network_id が違っている
		LIBISDB_ASSERT((m_NetworkID == NETWORK_ID_INVALID) || (m_NetworkID == pSDTTable->GetNetworkID()));

		m_TransportStreamID = pSDTTable->GetTransportStreamID();
		m_NetworkID = pSDTTable->GetNetworkID();

		LIBISDB_TRACE(LIBISDB_STR("transport_stream_id : %04X\n"), m_TransportStreamID);
		LIBISDB_TRACE(LIBISDB_STR("network_id          : %04X\n"), m_NetworkID);

		UpdateSDTServiceList(pSDTTable, &m_SDTServiceList);

		for (int SDTIndex = 0; SDTIndex < pSDTTable->GetServiceCount(); SDTIndex++) {
			// サービスIDを検索
			const int ServiceIndex = GetServiceIndexByID(pSDTTable->GetServiceID(SDTIndex));
			if (ServiceIndex >= 0)
				GetSDTServiceInfo(&m_ServiceList[ServiceIndex], pSDTTable, SDTIndex);
		}

		UpdateSDTStreamMap(pSDTTable, &m_SDTStreamMap);

		m_SDTUpdated = true;

		m_FilterLock.Unlock();
		m_EventListenerList.CallEventListener(&EventListener::OnSDTUpdated, this);
		m_FilterLock.Lock();
	} else if (TableID == SDTTable::TABLE_ID_OTHER) {
		// 他のTSのSDT
		const SDTOtherTable *pSDTOtherTable = pTableSet->GetOtherSDTTable();
		if (pSDTOtherTable == nullptr)
			return;

		for (int Table = 0; Table < pSDTOtherTable->GetTableCount(); Table++) {
			if (!pSDTOtherTable->IsSectionComplete(Table))
				continue;

			for (uint16_t Section = 0; Section < pSDTOtherTable->GetSectionCount(Table); Section++) {
				const SDTTable *pSDTTable = dynamic_cast<const SDTTable *>(pSDTOtherTable->GetSection(Table, Section));

				if (pSDTTable != nullptr)
					UpdateSDTStreamMap(pSDTTable, &m_SDTStreamMap);
			}
		}
	}
}


void AnalyzerFilter::OnNITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// NIT が更新された
	LIBISDB_TRACE(LIBISDB_STR("AnalyzerFilter::OnNITSection()\n"));

	const NITMultiTable *pNITMultiTable = dynamic_cast<const NITMultiTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pNITMultiTable == nullptr)
			|| !pNITMultiTable->IsNITComplete())
		return;

	const NITTable *pNITTable = pNITMultiTable->GetNITTable(0);
	if (pNITTable == nullptr)
		return;

	// ここで引っ掛かる場合、SDT と NIT で network_id が違っている
	LIBISDB_ASSERT((m_NetworkID == NETWORK_ID_INVALID) || (m_NetworkID == pNITTable->GetNetworkID()));

	m_NetworkID = pNITTable->GetNetworkID();

	LIBISDB_TRACE(LIBISDB_STR("network_id : %04X\n"), m_NetworkID);

	// ネットワーク情報取得
	{
		m_NITInfo.Reset();

		ARIBString Name;
		if (pNITTable->GetNetworkName(&Name))
			m_StringDecoder.Decode(Name, &m_NITInfo.NetworkName);

		const DescriptorBlock *pDescBlock = pNITTable->GetNetworkDescriptorBlock();
		if (pDescBlock != nullptr) {
			const SystemManagementDescriptor *pSysManageDesc =
				pDescBlock->GetDescriptor<SystemManagementDescriptor>();
			if (pSysManageDesc != nullptr) {
				m_NITInfo.BroadcastingFlag = pSysManageDesc->GetBroadcastingFlag();
				m_NITInfo.BroadcastingID   = pSysManageDesc->GetBroadcastingID();
			}
		}
	}

	// TS リスト取得
	m_NetworkStreamList.clear();

	for (uint16_t SectionNo = 0; SectionNo < pNITMultiTable->GetNITSectionCount(); SectionNo++) {
		if (SectionNo > 0) {
			pNITTable = pNITMultiTable->GetNITTable(SectionNo);
			if (pNITTable == nullptr)
				break;
		}

		for (int i = 0; i < pNITTable->GetTransportStreamCount(); i++) {
			const DescriptorBlock *pDescBlock = pNITTable->GetItemDescriptorBlock(i);

			if (pDescBlock != nullptr) {
				m_NetworkStreamList.emplace_back();
				NetworkStreamInfo &StreamInfo = m_NetworkStreamList.back();

				StreamInfo.TransportStreamID = pNITTable->GetTransportStreamID(i);
				StreamInfo.OriginalNetworkID = pNITTable->GetOriginalNetworkID(i);

				// サービスリスト取得
				const ServiceListDescriptor *pServiceListDesc =
					pDescBlock->GetDescriptor<ServiceListDescriptor>();
				if (pServiceListDesc != nullptr) {
					for (int j = 0; j < pServiceListDesc->GetServiceCount(); j++) {
						ServiceListDescriptor::ServiceInfo Info;

						if (pServiceListDesc->GetServiceInfo(j, &Info)) {
							StreamInfo.ServiceList.push_back(Info);

							int Index = GetServiceIndexByID(Info.ServiceID);
							if (Index >= 0) {
								ServiceInfo &Service = m_ServiceList[Index];
								if (Service.ServiceType == SERVICE_TYPE_INVALID)
									Service.ServiceType = Info.ServiceType;
							}
						}
					}
				}

				if ((SectionNo == 0) && (i == 0)) {
					const TSInformationDescriptor *pTSInfoDesc =
						pDescBlock->GetDescriptor<TSInformationDescriptor>();
					if (pTSInfoDesc != nullptr) {
						ARIBString Name;
						if (pTSInfoDesc->GetTSName(&Name))
							m_StringDecoder.Decode(Name, &m_NITInfo.TSName);
						m_NITInfo.RemoteControlKeyID = pTSInfoDesc->GetRemoteControlKeyID();
					}
				}
			}
		}
	}

	m_NITUpdated = true;

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnNITUpdated, this);
	m_FilterLock.Lock();
}


#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
void AnalyzerFilter::OnEITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// EITが更新された
	LIBISDB_TRACE(LIBISDB_STR("AnalyzerFilter::OnEITSection()\n"));

	m_EITUpdated = true;

	// 通知イベント設定
	// (PATがまだ来ていない場合は番組情報の取得関数が失敗するため保留にする)
	if (m_PATUpdated) {
		m_FilterLock.Unlock();
		m_EventListenerList.CallEventListener(&EventListener::OnEITUpdated, this);
		m_FilterLock.Lock();
	}
}
#endif


void AnalyzerFilter::OnCATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// CATが更新された
	LIBISDB_TRACE(LIBISDB_STR("AnalyzerFilter::OnCATSection()\n"));

	const CATTable *pCATTable = dynamic_cast<const CATTable *>(pTable);
	if (LIBISDB_TRACE_ERROR_IF(pCATTable == nullptr))
		return;

	// EMMのPIDリストを取得
	m_EMMPIDList.clear();
	const DescriptorBlock *pDescBlock = pCATTable->GetCATDescriptorBlock();
	pDescBlock->EnumDescriptors<CADescriptor>(
		[this](const CADescriptor *pCADesc) {
			if (pCADesc->GetCAPID() < 0x1FFF)
				m_EMMPIDList.push_back(pCADesc->GetCAPID());
		});

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnCATUpdated, this);
	m_FilterLock.Lock();
}


void AnalyzerFilter::OnTOTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// TOTが更新された

	// 現在のPCRを記憶
	uint16_t PCRPID = PID_INVALID;

	if (!m_ServiceList.empty()) {
		int Index = -1;

		if (m_TOTInterpolation.PCRPID != PID_INVALID) {
			for (size_t i = 0; i < m_ServiceList.size(); i++) {
				if (m_ServiceList[i].PCRPID == m_TOTInterpolation.PCRPID) {
					Index = static_cast<int>(i);
					break;
				}
			}
		}

		if (Index < 0) {
			for (size_t i = 0; i < m_ServiceList.size(); i++) {
				if (m_ServiceList[i].PCRPID != PID_INVALID) {
					Index = static_cast<int>(i);
					break;
				}
			}
		}

		if (Index >= 0) {
			uint64_t PCRTime = GetPCRTimeStamp(Index);
			if (PCRTime != PCR_INVALID) {
				PCRPID = m_ServiceList[Index].PCRPID;
				m_TOTInterpolation.PCRTime = PCRTime;
			}
		}
	}

	m_TOTInterpolation.PCRPID = PCRPID;

	m_FilterLock.Unlock();
	m_EventListenerList.CallEventListener(&EventListener::OnTOTUpdated, this);
	m_FilterLock.Lock();
}


}	// namespace LibISDB
