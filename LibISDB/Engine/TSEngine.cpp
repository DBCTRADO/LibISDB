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
 @file   TSEngine.cpp
 @brief  TSエンジン
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "TSEngine.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


TSEngine::TSEngine() noexcept
	: m_IsBuilt(false)

	, m_pSource(nullptr)
	, m_pAnalyzer(nullptr)

	, m_CurTransportStreamID(TRANSPORT_STREAM_ID_INVALID)
	, m_CurServiceID(SERVICE_ID_INVALID)

	, m_VideoStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_AudioStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_CurVideoStream(-1)
	, m_CurVideoComponentTag(COMPONENT_TAG_INVALID)
	, m_CurAudioStream(-1)
	, m_CurAudioComponentTag(COMPONENT_TAG_INVALID)
	, m_CurEventID(EVENT_ID_INVALID)

	, m_StartStreamingOnSourceOpen(false)
{
}


TSEngine::~TSEngine()
{
	CloseEngine();
}


bool TSEngine::BuildEngine(const FilterGraph::ConnectionInfo *pConnectionList, size_t ConnectionCount)
{
	if (LIBISDB_TRACE_ERROR_IF(m_IsBuilt))
		return true;

	Log(Logger::LogType::Information, LIBISDB_STR("Building filter graph..."));

	if (!m_FilterGraph.ConnectFilters(pConnectionList, ConnectionCount))
		return false;

	m_FilterGraph.WalkGraph(
		[this](FilterBase *pFilter) {
			pFilter->SetLogger(this->m_pLogger);
			pFilter->Initialize();
		});

	m_IsBuilt = true;

	return true;
}


bool TSEngine::BuildEngine(std::initializer_list<FilterBase *> FilterList)
{
	if (LIBISDB_TRACE_ERROR_IF(FilterList.size() < 2))
		return false;

	std::vector<FilterGraph::ConnectionInfo> ConnectionList(FilterList.size() - 1);

	auto it = FilterList.begin();
	FilterGraph::IDType PrevID = RegisterFilter(*it);
	size_t i = 0;

	for (++it; it != FilterList.end(); ++it) {
		FilterGraph::IDType ID = RegisterFilter(*it);

		ConnectionList[i].UpstreamFilterID = PrevID;
		ConnectionList[i].DownstreamFilterID = ID;
		PrevID = ID;
		i++;
	}

	return BuildEngine(ConnectionList.data(), ConnectionList.size());
}


bool TSEngine::CloseEngine()
{
	if (!m_IsBuilt)
		return true;

	Log(Logger::LogType::Information, LIBISDB_STR("Closing TSEngine..."));

	OnEngineClose();

	CloseSource();

	m_FilterGraph.WalkGraph(
		[this](FilterBase *pFilter) {
			pFilter->Finalize();
			if (pFilter->GetLogger() == this->m_pLogger)
				pFilter->SetLogger(nullptr);
		});

	m_FilterGraph.DisconnectFilters();

	m_IsBuilt = false;

	OnEngineClosed();

	Log(Logger::LogType::Information, LIBISDB_STR("TSEngine closed"));

	return true;
}


bool TSEngine::ResetEngine()
{
	if (!m_IsBuilt)
		return false;

	FilterBase *pRoot = m_FilterGraph.GetRootFilter();
	if (pRoot != nullptr)
		pRoot->ResetGraph();

	return true;
}


FilterGraph::IDType TSEngine::RegisterFilter(FilterBase *pFilter)
{
	if (LIBISDB_TRACE_ERROR_IF(pFilter == nullptr))
		return 0;

	FilterGraph::IDType ID = m_FilterGraph.RegisterFilter(pFilter);
	if (ID == 0)
		return 0;

	OnFilterRegistered(pFilter, ID);

	return ID;
}


bool TSEngine::OpenSource(const CStringView &Name)
{
	CloseSource();

	if (LIBISDB_TRACE_ERROR_IF(m_pSource == nullptr)) {
		return false;
	}

	const FilterGraph::IDType SourceFilterID = m_FilterGraph.GetFilterID(m_pSource);

	m_FilterGraph.DisconnectFilter(SourceFilterID, FilterGraph::ConnectDirection::Downstream);

	// ソースフィルタを開く
	Log(Logger::LogType::Information, LIBISDB_STR("Opening source..."));
	bool OK = m_pSource->OpenSource(Name);
	if (!OK) {
		SetError(m_pSource->GetLastErrorDescription());
	}

	m_FilterGraph.ConnectFilter(SourceFilterID, FilterGraph::ConnectDirection::Downstream);

	if (!OK)
		return false;

	if (m_StartStreamingOnSourceOpen) {
		Log(Logger::LogType::Information, LIBISDB_STR("Starting streaming..."));
		if (!m_pSource->StartStreaming()) {
			SetError(m_pSource->GetLastErrorDescription());
			return false;
		}
	}

	//ResetEngine();
	ResetStatus();

	return true;
}


bool TSEngine::CloseSource()
{
	// ソースフィルタを開放する
	if (m_pSource == nullptr)
		return false;

	m_pSource->StopStreaming();

	if (m_pSource->IsSourceOpen()) {
		Log(Logger::LogType::Information, LIBISDB_STR("Closing source..."));
		m_pSource->CloseSource();
	}

	return true;
}


bool TSEngine::IsSourceOpen() const
{
	return (m_pSource != nullptr) && m_pSource->IsSourceOpen();
}


bool TSEngine::SetServiceSelectInfo(const ServiceSelectInfo *pServiceSelInfo)
{
	BlockLock Lock(m_EngineLock);

	if (pServiceSelInfo != nullptr) {
		m_SetChannelServiceSel = *pServiceSelInfo;
	} else {
		m_SetChannelServiceSel.Reset();
	}

	return true;
}


bool TSEngine::SetService(const ServiceSelectInfo &ServiceSelInfo, bool Reserve)
{
	BlockLock Lock(m_EngineLock);

	if (LIBISDB_TRACE_ERROR_IF(m_pAnalyzer == nullptr))
		return false;

	uint16_t ServiceID = ServiceSelInfo.ServiceID;

	LIBISDB_TRACE(LIBISDB_STR("TSEngine::SetService() : service_id %04X\n"), ServiceID);

	// Reserve == true の場合、まだPATが来ていなくてもエラーにしない

	bool SetService = true, OneSeg = false;

	if (ServiceSelInfo.OneSegSelect == OneSegSelectType::HighPriority) {
		// ワンセグ優先
		uint16_t SID;
		if ((ServiceSelInfo.PreferredServiceIndex >= 0)
				&& (SID = m_pAnalyzer->Get1SegServiceIDByIndex(
						ServiceSelInfo.PreferredServiceIndex)) != SERVICE_ID_INVALID) {
			OneSeg = true;
			ServiceID = SID;
		} else if ((SID = m_pAnalyzer->GetFirst1SegServiceID()) != SERVICE_ID_INVALID) {
			OneSeg = true;
			ServiceID = SID;
		}
	}

	if (!OneSeg && (ServiceID != SERVICE_ID_INVALID)) {
		int Index = GetSelectableServiceIndexByID(ServiceID);
		if (Index < 0) {
			if (!Reserve || (m_CurTransportStreamID != TRANSPORT_STREAM_ID_INVALID))
				return false;
			SetService = false;
		}
	}

	if (SetService)
		SelectService(ServiceID, ServiceSelInfo.OneSegSelect == OneSegSelectType::Refuse);

	m_ServiceSel = ServiceSelInfo;

	return true;
}


bool TSEngine::SelectService(uint16_t ServiceID, bool No1Seg)
{
	LIBISDB_TRACE(LIBISDB_STR("TSEngine::SelectService(%04X)\n"), ServiceID);

	BlockLock Lock(m_EngineLock);

	// サービス変更(ServiceID==SERVICE_ID_INVALIDならPAT先頭サービス)

	if (ServiceID == SERVICE_ID_INVALID) {
		LIBISDB_TRACE(LIBISDB_STR("Select default service\n"));
		// 先頭PMTが到着するまで失敗にする
		ServiceID = GetDefaultServiceID();
		if (ServiceID == SERVICE_ID_INVALID) {
			LIBISDB_TRACE(LIBISDB_STR("No viewable service\n"));
			return false;
		}
		if (No1Seg && m_pAnalyzer->Is1SegService(m_pAnalyzer->GetServiceIndexByID(ServiceID))) {
			return false;
		}
	} else {
		if (GetSelectableServiceIndexByID(ServiceID) < 0) {
			LIBISDB_TRACE(LIBISDB_STR("Service %04X not found\n"), ServiceID);
			return false;
		}
	}

	const bool ServiceChanged = (ServiceID != m_CurServiceID);
	const int CurServiceIndex = m_pAnalyzer->GetServiceIndexByID(ServiceID);

	m_CurServiceID = ServiceID;

	LIBISDB_TRACE(LIBISDB_STR("Select service : [%d] (service_id %04X)\n"), CurServiceIndex, ServiceID);

	int VideoIndex;
	if (m_CurVideoComponentTag != COMPONENT_TAG_INVALID
			/* && !ServiceChanged */) {
		VideoIndex = m_pAnalyzer->GetVideoIndexByComponentTag(CurServiceIndex, m_CurVideoComponentTag);
		if (VideoIndex < 0)
			VideoIndex = 0;
	} else {
		VideoIndex = 0;
	}
	uint16_t VideoPID = m_pAnalyzer->GetVideoESPID(CurServiceIndex, VideoIndex);
	if ((VideoPID == PID_INVALID) && (VideoIndex != 0)) {
		VideoPID = m_pAnalyzer->GetVideoESPID(CurServiceIndex, 0);
		VideoIndex = 0;
	}
	m_CurVideoStream = VideoIndex;
	m_CurVideoComponentTag = m_pAnalyzer->GetVideoComponentTag(CurServiceIndex, VideoIndex);
	const uint8_t VideoStreamType = m_pAnalyzer->GetVideoStreamType(CurServiceIndex, VideoIndex);
	LIBISDB_TRACE(LIBISDB_STR("Select video : [%d] (component_tag %02X)\n"), m_CurVideoStream, m_CurVideoComponentTag);

	int AudioIndex;
	if (m_CurAudioComponentTag != COMPONENT_TAG_INVALID
			/* && !ServiceChanged */) {
		AudioIndex = m_pAnalyzer->GetAudioIndexByComponentTag(CurServiceIndex, m_CurAudioComponentTag);
		if (AudioIndex < 0)
			AudioIndex = 0;
	} else {
		AudioIndex = 0;
	}
	uint16_t AudioPID = m_pAnalyzer->GetAudioESPID(CurServiceIndex, AudioIndex);
	if ((AudioPID == PID_INVALID) && (AudioIndex != 0)) {
		AudioPID = m_pAnalyzer->GetAudioESPID(CurServiceIndex, 0);
		AudioIndex = 0;
	}
	m_CurAudioStream = AudioIndex;
	m_CurAudioComponentTag = m_pAnalyzer->GetAudioComponentTag(CurServiceIndex, AudioIndex);
	const uint8_t AudioStreamType = m_pAnalyzer->GetAudioStreamType(CurServiceIndex, AudioIndex);
	LIBISDB_TRACE(LIBISDB_STR("Select audio : [%d] (component_tag %02X)\n"), m_CurAudioStream, m_CurAudioComponentTag);

	if (m_VideoStreamType != VideoStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Video stream_type changed (%02X -> %02X)\n"), m_VideoStreamType, VideoStreamType);
		m_VideoStreamType = VideoStreamType;
		OnVideoStreamTypeChanged(VideoStreamType);
	}

	if (m_AudioStreamType != AudioStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Audio stream_type changed (%02X -> %02X)\n"), m_AudioStreamType, AudioStreamType);
		m_AudioStreamType = AudioStreamType;
		OnAudioStreamTypeChanged(AudioStreamType);
	}

	m_FilterGraph.WalkGraph(
		[ServiceID](FilterBase *pFilter) { pFilter->SetActiveServiceID(ServiceID); });

	SetVideoPID(VideoPID, true);
	SetAudioPID(AudioPID, true);

//	if (m_WriteCurServiceOnly)
//		SetWriteStream(ServiceID, m_WriteStream);

	if (ServiceChanged)
		OnServiceChanged(ServiceID);

	const uint16_t EventID = m_pAnalyzer->GetEventID(CurServiceIndex);
	if (ServiceChanged || (m_CurEventID != EventID)) {
		m_CurEventID = EventID;
		OnEventChanged(EventID);
	}

	return true;
}


uint16_t TSEngine::GetServiceID() const
{
	return m_CurServiceID;
}


int TSEngine::GetServiceIndex() const
{
	if (m_pAnalyzer == nullptr)
		return -1;
	return m_pAnalyzer->GetServiceIndexByID(m_CurServiceID);
}


bool TSEngine::SetOneSegSelectType(OneSegSelectType Type)
{
	BlockLock Lock(m_EngineLock);

	m_ServiceSel.OneSegSelect = Type;
	m_ServiceSel.PreferredServiceIndex = -1;

	return true;
}


uint16_t TSEngine::GetTransportStreamID() const
{
	return m_CurTransportStreamID;
}


uint16_t TSEngine::GetNetworkID() const
{
	if (m_pAnalyzer == nullptr)
		return NETWORK_ID_INVALID;
	return m_pAnalyzer->GetNetworkID();
}


uint16_t TSEngine::GetEventID() const
{
	return m_CurEventID;
}


uint8_t TSEngine::GetVideoStreamType() const
{
	return m_VideoStreamType;
}


int TSEngine::GetVideoStreamCount(int ServiceIndex) const
{
	BlockLock Lock(m_EngineLock);

	if (m_pAnalyzer == nullptr)
		return 0;

	uint16_t ServiceID;

	if (ServiceIndex < 0) {
		ServiceID = m_CurServiceID;
	} else {
		ServiceID = GetSelectableServiceID(ServiceIndex);
	}

	if (ServiceID == SERVICE_ID_INVALID)
		return 0;

	return m_pAnalyzer->GetVideoESCount(m_pAnalyzer->GetServiceIndexByID(ServiceID));
}


bool TSEngine::SetVideoStream(int StreamIndex)
{
	BlockLock Lock(m_EngineLock);

	if (m_pAnalyzer == nullptr)
		return false;

	const int ServiceIndex = GetServiceIndex();
	if (ServiceIndex < 0)
		return false;

	const uint16_t VideoPID = m_pAnalyzer->GetVideoESPID(ServiceIndex, StreamIndex);
	if (VideoPID == PID_INVALID)
		return false;

	m_CurVideoStream = StreamIndex;
	m_CurVideoComponentTag = m_pAnalyzer->GetVideoComponentTag(ServiceIndex, StreamIndex);

	LIBISDB_TRACE(LIBISDB_STR("Select video : [%d] (component_tag %02X)\n"), m_CurVideoStream, m_CurVideoComponentTag);

	const uint8_t VideoStreamType = m_pAnalyzer->GetVideoStreamType(ServiceIndex, StreamIndex);

	if (m_VideoStreamType != VideoStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Video stream_type changed (%02x -> %02x)\n"), m_VideoStreamType, VideoStreamType);
		m_VideoStreamType = VideoStreamType;
		OnVideoStreamTypeChanged(VideoStreamType);
	}

	SetVideoPID(VideoPID, false);

	return true;
}


int TSEngine::GetVideoStream() const
{
	return m_CurVideoStream;
}


uint8_t TSEngine::GetVideoComponentTag() const
{
	return m_CurVideoComponentTag;
}


uint8_t TSEngine::GetAudioStreamType() const
{
	return m_AudioStreamType;
}


int TSEngine::GetAudioStreamCount(int ServiceIndex) const
{
	BlockLock Lock(m_EngineLock);

	if (m_pAnalyzer == nullptr)
		return 0;

	uint16_t ServiceID;

	if (ServiceIndex < 0) {
		ServiceID = m_CurServiceID;
	} else {
		ServiceID = GetSelectableServiceID(ServiceIndex);
	}

	if (ServiceID == SERVICE_ID_INVALID)
		return 0;

	return m_pAnalyzer->GetAudioESCount(m_pAnalyzer->GetServiceIndexByID(ServiceID));
}


bool TSEngine::SetAudioStream(int StreamIndex)
{
	BlockLock Lock(m_EngineLock);

	if (m_pAnalyzer == nullptr)
		return false;

	const int ServiceIndex = GetServiceIndex();
	if (ServiceIndex < 0)
		return false;

	const uint16_t AudioPID = m_pAnalyzer->GetAudioESPID(ServiceIndex, StreamIndex);
	if (AudioPID == PID_INVALID)
		return false;

	m_CurAudioStream = StreamIndex;
	m_CurAudioComponentTag = m_pAnalyzer->GetAudioComponentTag(ServiceIndex, StreamIndex);

	LIBISDB_TRACE(LIBISDB_STR("Select audio : [%d] (component_tag %02X)\n"), m_CurAudioStream, m_CurAudioComponentTag);

	SetAudioPID(AudioPID, false);

	uint8_t AudioStreamType = m_pAnalyzer->GetAudioStreamType(ServiceIndex, StreamIndex);

	if (m_AudioStreamType != AudioStreamType) {
		LIBISDB_TRACE(LIBISDB_STR("Audio stream_type changed (%02x -> %02x)\n"), m_AudioStreamType, AudioStreamType);
		m_AudioStreamType = AudioStreamType;
		OnAudioStreamTypeChanged(AudioStreamType);
	}

	return true;
}


int TSEngine::GetAudioStream() const
{
	return m_CurAudioStream;
}


uint8_t TSEngine::GetAudioComponentTag() const
{
	return m_CurAudioComponentTag;
}


void TSEngine::SetStartStreamingOnSourceOpen(bool Start)
{
	m_StartStreamingOnSourceOpen = Start;
}


void TSEngine::SetLogger(Logger *pLogger)
{
	ObjectBase::SetLogger(pLogger);

	m_FilterGraph.EnumFilters([pLogger](FilterBase *pFilter) { pFilter->SetLogger(pLogger); });
}


void TSEngine::OnGraphReset(SourceFilter *pSource)
{
	ResetStatus();
}


void TSEngine::OnSourceChanged(SourceFilter *pSource)
{
	BlockLock Lock(m_EngineLock);

	m_ServiceSel = m_SetChannelServiceSel;
	ResetStatus();
}


void TSEngine::OnPATUpdated(AnalyzerFilter *pAnalyzer)
{
	LIBISDB_TRACE(LIBISDB_STR("TSEngine::OnPATUpdated()\n"));

	BlockLock Lock(m_EngineLock);
	const uint16_t TransportStreamID = pAnalyzer->GetTransportStreamID();

	if (m_CurTransportStreamID != TransportStreamID) {
		// ストリームIDが変わっているなら初期化
		LIBISDB_TRACE(LIBISDB_STR("Stream changed (%04X <- %04X)\n"), TransportStreamID, m_CurTransportStreamID);

		m_CurTransportStreamID = TransportStreamID;
		m_CurServiceID = SERVICE_ID_INVALID;
		m_CurVideoStream = -1;
		m_CurVideoComponentTag = COMPONENT_TAG_INVALID;
		m_CurAudioStream = -1;
		m_CurAudioComponentTag = COMPONENT_TAG_INVALID;
		m_CurEventID = EVENT_ID_INVALID;

		bool SetService = true;
		uint16_t ServiceID = SERVICE_ID_INVALID;

		if (m_ServiceSel.OneSegSelect == OneSegSelectType::HighPriority) {
			// ワンセグ優先
			// この段階ではまだ保留
			SetService = false;
		} else if (m_ServiceSel.ServiceID != SERVICE_ID_INVALID) {
			// サービスが指定されている
			const int ServiceIndex = pAnalyzer->GetServiceIndexByID(m_ServiceSel.ServiceID);
			if (ServiceIndex < 0) {
				// サービスがPATにない
				LIBISDB_TRACE(LIBISDB_STR("Specified service_id %04X not found in PAT\n"), m_ServiceSel.ServiceID);
				SetService = false;
			} else {
				if (GetSelectableServiceIndexByID(m_ServiceSel.ServiceID) >= 0) {
					ServiceID = m_ServiceSel.ServiceID;
				} else {
					SetService = false;
				}
			}
		}

		if (!SetService || !SelectService(ServiceID, m_ServiceSel.OneSegSelect == OneSegSelectType::Refuse)) {
			SetVideoPID(PID_INVALID, true);
			SetAudioPID(PID_INVALID, true);
		}
	} else {
		bool SetService = true, OneSeg = false;
		uint16_t ServiceID = SERVICE_ID_INVALID;

		if (m_ServiceSel.OneSegSelect == OneSegSelectType::HighPriority) {
			// ワンセグ優先
			uint16_t SID;
			if (m_ServiceSel.PreferredServiceIndex >= 0) {
				SID = pAnalyzer->Get1SegServiceIDByIndex(m_ServiceSel.PreferredServiceIndex);
				if (SID != SERVICE_ID_INVALID) {
					OneSeg = true;
					if (pAnalyzer->IsServicePMTAcquired(pAnalyzer->GetServiceIndexByID(SID))) {
						ServiceID = SID;
					}
				}
			}
			if ((ServiceID == SERVICE_ID_INVALID)
					&& ((SID = pAnalyzer->GetFirst1SegServiceID()) != SERVICE_ID_INVALID)) {
				OneSeg = true;
				if (pAnalyzer->IsServicePMTAcquired(pAnalyzer->GetServiceIndexByID(SID))) {
					ServiceID = SID;
				} else {
					SetService = false;
				}
			}
		}

		if (!OneSeg && (m_ServiceSel.ServiceID != SERVICE_ID_INVALID)) {
			const int ServiceIndex = pAnalyzer->GetServiceIndexByID(m_ServiceSel.ServiceID);
			if (ServiceIndex < 0) {
				LIBISDB_TRACE(LIBISDB_STR("Specified service_id %04X not found in PAT\n"), m_ServiceSel.ServiceID);
				if (((m_CurServiceID == SERVICE_ID_INVALID) && !m_ServiceSel.FollowViewableService)
						|| (GetSelectableServiceCount() == 0)) {
					SetService = false;
				}
			} else {
				if (GetSelectableServiceIndexByID(m_ServiceSel.ServiceID) >= 0) {
					ServiceID = m_ServiceSel.ServiceID;
				} else if (((m_CurServiceID == SERVICE_ID_INVALID) && !m_ServiceSel.FollowViewableService)
						|| !pAnalyzer->IsServicePMTAcquired(ServiceIndex)) {
					// サービスはPATにあるが、まだPMTが来ていない
					SetService = false;
				}
			}
		}

		if (SetService && (ServiceID == SERVICE_ID_INVALID) && (m_CurServiceID != SERVICE_ID_INVALID)) {
			const int ServiceIndex = pAnalyzer->GetServiceIndexByID(m_CurServiceID);
			if (ServiceIndex < 0) {
				// サービスがPATにない
				LIBISDB_TRACE(LIBISDB_STR("Current service_id %04X not found in PAT\n"), m_CurServiceID);
				if (m_ServiceSel.FollowViewableService
						&& (GetSelectableServiceCount() > 0)) {
					m_CurServiceID = SERVICE_ID_INVALID;
				} else {
					// まだ視聴可能なサービスのPMTが一つも来ていない場合は保留
					SetService = false;
				}
			} else {
				if (GetSelectableServiceIndexByID(m_CurServiceID) >= 0) {
					ServiceID = m_CurServiceID;
				} else if (!m_ServiceSel.FollowViewableService
						|| !pAnalyzer->IsServicePMTAcquired(ServiceIndex)) {
					SetService = false;
				}
			}
		}

		if (SetService)
			SelectService(ServiceID, m_ServiceSel.OneSegSelect == OneSegSelectType::Refuse);
	}
}


void TSEngine::OnPMTUpdated(AnalyzerFilter *pAnalyzer, uint16_t ServiceID)
{
	OnPATUpdated(pAnalyzer);
}


void TSEngine::OnEITUpdated(AnalyzerFilter *pAnalyzer)
{
	const uint16_t EventID = pAnalyzer->GetEventID(pAnalyzer->GetServiceIndexByID(m_CurServiceID));

	if (m_CurEventID != EventID) {
		m_CurEventID = EventID;
		OnEventChanged(EventID);
	}
}


void TSEngine::ResetStatus()
{
	m_CurTransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_CurServiceID = SERVICE_ID_INVALID;
}


void TSEngine::SetVideoPID(uint16_t PID, bool ServiceChanged)
{
	m_FilterGraph.WalkGraph([=](FilterBase *pFilter) { pFilter->SetActiveVideoPID(PID, ServiceChanged); });
}


void TSEngine::SetAudioPID(uint16_t PID, bool ServiceChanged)
{
	m_FilterGraph.WalkGraph([=](FilterBase *pFilter) { pFilter->SetActiveAudioPID(PID, ServiceChanged); });
}


bool TSEngine::IsSelectableService(int Index) const
{
	return (m_pAnalyzer != nullptr)
		&& (Index >= 0)
		&& (Index < m_pAnalyzer->GetServiceCount());
}


int TSEngine::GetSelectableServiceCount() const
{
	if (m_pAnalyzer == nullptr)
		return 0;
	return m_pAnalyzer->GetServiceCount();
}


uint16_t TSEngine::GetSelectableServiceID(int Index) const
{
	if (m_pAnalyzer == nullptr)
		return SERVICE_ID_INVALID;
	return m_pAnalyzer->GetServiceID(Index);
}


uint16_t TSEngine::GetDefaultServiceID() const
{
	if (m_pAnalyzer == nullptr)
		return SERVICE_ID_INVALID;
	return m_pAnalyzer->GetServiceID(-1);
}


int TSEngine::GetSelectableServiceIndexByID(uint16_t ServiceID) const
{
	if (m_pAnalyzer == nullptr)
		return -1;
	return m_pAnalyzer->GetServiceIndexByID(ServiceID);
}


bool TSEngine::GetSelectableServiceList(AnalyzerFilter::ServiceList *pList) const
{
	if (m_pAnalyzer == nullptr)
		return false;
	return m_pAnalyzer->GetServiceList(pList);
}


void TSEngine::OnFilterRegistered(FilterBase *pFilter, FilterGraph::IDType ID)
{
	AnalyzerFilter *pAnalyzer = dynamic_cast<AnalyzerFilter *>(pFilter);
	if (pAnalyzer != nullptr) {
		pAnalyzer->AddEventListener(this);
		m_pAnalyzer = pAnalyzer;
		return;
	}

	SourceFilter *pSource = dynamic_cast<SourceFilter *>(pFilter);
	if (pSource != nullptr) {
		pSource->AddEventListener(this);
		m_pSource = pSource;
		return;
	}
}


}	// namespace LibISDB
