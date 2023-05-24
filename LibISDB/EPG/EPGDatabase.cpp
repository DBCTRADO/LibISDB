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
 @file   EPGDatabase.cpp
 @brief  番組情報データベース
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "EPGDatabase.hpp"
#include <algorithm>
#include "../Base/ARIBTime.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{

namespace
{


bool IsEventValid(const EventInfo &Event)
{
	return !Event.EventName.empty() || Event.IsCommonEvent;
}


// EIT schedule の時刻を取得する
unsigned long long GetScheduleTime(unsigned long long CurTime, uint16_t TableID, uint8_t SectionNumber)
{
	constexpr unsigned long long HOUR = 60 * 60;

	return (CurTime / (24 * HOUR) * (24 * HOUR)) +
			((TableID & 0x07) * (4 * 24 * HOUR)) +
			((SectionNumber >> 3) * (3 * HOUR));
}


}




EPGDatabase::EPGDatabase() noexcept
	: m_IsUpdated(false)
	, m_ScheduleOnly(false)
	, m_NoPastEvents(true)
	, m_StringDecodeFlags(ARIBStringDecoder::DecodeFlag::UseCharSize)
	, m_CurTOTSeconds(0)
{
}


void EPGDatabase::Clear()
{
	BlockLock Lock(m_Lock);

	m_ServiceMap.clear();
	m_PendingServiceMap.clear();
}


int EPGDatabase::GetServiceCount() const
{
	BlockLock Lock(m_Lock);

	return static_cast<int>(m_ServiceMap.size());
}


bool EPGDatabase::GetServiceList(ServiceList *pList) const
{
	if (LIBISDB_TRACE_ERROR_IF(pList == nullptr))
		return false;

	BlockLock Lock(m_Lock);

	pList->resize(m_ServiceMap.size());

	auto it = pList->begin();
	for (auto &e : m_ServiceMap) {
		*it = e.first;
		++it;
	}

	return true;
}


bool EPGDatabase::IsServiceUpdated(uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID) const
{
	BlockLock Lock(m_Lock);

	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return false;

	return pService->IsUpdated;
}


bool EPGDatabase::ResetServiceUpdated(uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID)
{
	BlockLock Lock(m_Lock);

	auto it = m_ServiceMap.find(ServiceInfo(NetworkID, TransportStreamID, ServiceID));
	if (it == m_ServiceMap.end())
		return false;
	it->second.IsUpdated = false;

	return true;
}


bool EPGDatabase::GetEventList(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	ReturnArg<EventList> List, OptionalReturnArg<TimeEventMap> TimeMap) const
{
	if (!List)
		return false;

	List->clear();

	BlockLock Lock(m_Lock);

	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return false;

	List->reserve(pService->EventMap.size());

	if (TimeMap) {
		TimeMap->clear();
		for (auto &Time : pService->TimeMap) {
			auto itEvent = pService->EventMap.find(Time.EventID);
			if ((itEvent != pService->EventMap.end())
					&& IsEventValid(itEvent->second)) {
				List->push_back(itEvent->second);
				TimeMap->insert(Time);
			}
		}
	} else {
		for (auto &Event : pService->EventMap) {
			if (IsEventValid(Event.second))
				List->push_back(Event.second);
		}
	}

	return true;
}


bool EPGDatabase::GetEventListSortedByTime(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	ReturnArg<EventList> List) const
{
	if (!List)
		return false;

	List->clear();

	BlockLock Lock(m_Lock);

	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return false;

	List->reserve(pService->EventMap.size());

	for (auto &Time : pService->TimeMap) {
		auto itEvent = pService->EventMap.find(Time.EventID);
		if ((itEvent != pService->EventMap.end())
				&& IsEventValid(itEvent->second)) {
			List->push_back(itEvent->second);
		}
	}

	return true;
}


bool EPGDatabase::GetEventInfo(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	uint16_t EventID, ReturnArg<EventInfo> Info) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_Lock);

	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService != nullptr) {
		auto itEvent = pService->EventMap.find(EventID);
		if ((itEvent != pService->EventMap.end())
				&& IsEventValid(itEvent->second)) {
			*Info = itEvent->second;
			SetCommonEventInfo(&*Info);
			return true;
		}
	}

	return false;
}


bool EPGDatabase::GetEventInfo(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	const DateTime &Time, ReturnArg<EventInfo> Info) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_Lock);

	bool Found = false;
	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService != nullptr) {
		const TimeEventInfo Key(Time);
		auto itTime = pService->TimeMap.upper_bound(Key);
		if (itTime != pService->TimeMap.begin()) {
			--itTime;
			if (itTime->StartTime + itTime->Duration > Key.StartTime) {
				auto itEvent = pService->EventMap.find(itTime->EventID);
				if ((itEvent != pService->EventMap.end())
						&& IsEventValid(itEvent->second)) {
					*Info = itEvent->second;
					SetCommonEventInfo(&*Info);
					Found = true;
				}
			}
		}
	}

	return Found;
}


bool EPGDatabase::GetNextEventInfo(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	const DateTime &Time, ReturnArg<EventInfo> Info) const
{
	if (!Info)
		return false;

	BlockLock Lock(m_Lock);

	bool Found = false;
	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService != nullptr) {
		const TimeEventInfo Key(Time);
		auto itTime = pService->TimeMap.upper_bound(Key);
		if (itTime != pService->TimeMap.end()) {
			auto itEvent = pService->EventMap.find(itTime->EventID);
			if ((itEvent != pService->EventMap.end())
					&& IsEventValid(itEvent->second)) {
				*Info = itEvent->second;
				SetCommonEventInfo(&*Info);
				Found = true;
			}
		}
	}

	return Found;
}


bool EPGDatabase::EnumEventsUnsorted(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	const std::function<bool(const EventInfo &Event)> &Callback) const
{
	if (!Callback)
		return false;

	BlockLock Lock(m_Lock);

	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return false;

	for (auto &Event : pService->EventMap) {
		if (!Callback(Event.second))
			break;
	}

	return true;
}


bool EPGDatabase::EnumEventsSortedByTime(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	const std::function<bool(const EventInfo &Event)> &Callback) const
{
	if (!Callback)
		return false;

	BlockLock Lock(m_Lock);

	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return false;

	for (auto &Time : pService->TimeMap) {
		auto itEvent = pService->EventMap.find(Time.EventID);
		if (itEvent != pService->EventMap.end()) {
			if (!Callback(itEvent->second))
				break;
		}
	}

	return true;
}


bool EPGDatabase::EnumEventsSortedByTime(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	const DateTime *pEarliest, const DateTime *pLatest,
	const std::function<bool(const EventInfo &Event)> &Callback) const
{
	if (!Callback)
		return false;

	BlockLock Lock(m_Lock);

	const ServiceEventMap *pService = FindServiceEventMap(NetworkID, TransportStreamID, ServiceID);
	if (pService == nullptr)
		return false;

	TimeEventMap::const_iterator itTime, itEnd;

	if ((pEarliest != nullptr) && pEarliest->IsValid()) {
		const TimeEventInfo Key(*pEarliest);
		itTime = pService->TimeMap.upper_bound(Key);
		if (itTime != pService->TimeMap.begin()) {
			auto itPrev = itTime;
			--itPrev;
			if (itPrev->StartTime + itPrev->Duration > Key.StartTime)
				itTime = itPrev;
		}
	} else {
		itTime = pService->TimeMap.begin();
	}

	if ((pLatest != nullptr) && pLatest->IsValid()) {
		itEnd = pService->TimeMap.lower_bound(TimeEventInfo(*pLatest));
	} else {
		itEnd = pService->TimeMap.end();
	}

	for (;itTime != itEnd; ++itTime) {
		auto itEvent = pService->EventMap.find(itTime->EventID);
		if (itEvent != pService->EventMap.end()) {
			if (!Callback(itEvent->second))
				break;
		}
	}

	return true;
}


bool EPGDatabase::SetServiceEventList(const ServiceInfo &Info, EventList &&List)
{
	BlockLock Lock(m_Lock);

	auto it = m_ServiceMap.find(Info);
	if (it != m_ServiceMap.end())
		m_ServiceMap.erase(it);

	ServiceEventMap &Service = m_ServiceMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(Info),
		std::forward_as_tuple()).first->second;

	Service.EventMap.rehash(300);

	for (EventInfo &Event : List) {
		Service.EventMap.emplace(Event.EventID, std::move(Event));
		Service.TimeMap.emplace(Event);
	}

	return true;
}


bool EPGDatabase::Merge(
	EPGDatabase *pSrcDatabase,
	MergeFlag Flags, std::optional<EventInfo::SourceIDType> SourceID)
{
	if (LIBISDB_TRACE_ERROR_IF(pSrcDatabase == nullptr))
		return false;

	BlockLock Lock(m_Lock);

	for (auto &SrcService : pSrcDatabase->m_ServiceMap) {
		MergeEventMap(SrcService.first, SrcService.second, Flags, SourceID);
	}

	return true;
}


bool EPGDatabase::MergeService(
	EPGDatabase *pSrcDatabase,
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
	MergeFlag Flags, std::optional<EventInfo::SourceIDType> SourceID)
{
	if (LIBISDB_TRACE_ERROR_IF(pSrcDatabase == nullptr))
		return false;

	const ServiceInfo Key(NetworkID, TransportStreamID, ServiceID);
	auto itSrcService = pSrcDatabase->m_ServiceMap.find(Key);
	if (itSrcService == pSrcDatabase->m_ServiceMap.end())
		return false;

	BlockLock Lock(m_Lock);

	MergeEventMap(itSrcService->first, itSrcService->second, Flags, SourceID);

	return true;
}


bool EPGDatabase::IsScheduleComplete(uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID, bool Extended) const
{
	BlockLock Lock(m_Lock);

	auto itService = m_ServiceMap.find(ServiceInfo(NetworkID, TransportStreamID, ServiceID));
	if (itService == m_ServiceMap.end())
		return false;

	return itService->second.Schedule.IsComplete(m_CurTOTTime.Hour, Extended);
}


bool EPGDatabase::HasSchedule(uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID, bool Extended) const
{
	BlockLock Lock(m_Lock);

	auto itService = m_ServiceMap.find(ServiceInfo(NetworkID, TransportStreamID, ServiceID));
	if (itService == m_ServiceMap.end())
		return false;

	return itService->second.Schedule.HasSchedule(Extended);
}


void EPGDatabase::ResetScheduleStatus()
{
	LIBISDB_TRACE(LIBISDB_STR("EPGDatabase::ResetScheduleStatus()\n"));

	BlockLock Lock(m_Lock);

	for (auto &e : m_ServiceMap)
		e.second.Schedule.Reset();
}


void EPGDatabase::SetScheduleOnly(bool ScheduleOnly)
{
	BlockLock Lock(m_Lock);

	m_ScheduleOnly = ScheduleOnly;
}


void EPGDatabase::SetNoPastEvents(bool NoPastEvents)
{
	BlockLock Lock(m_Lock);

	m_NoPastEvents = NoPastEvents;
}


void EPGDatabase::SetStringDecodeFlags(ARIBStringDecoder::DecodeFlag Flags)
{
	BlockLock Lock(m_Lock);

	m_StringDecodeFlags = Flags;
}


bool EPGDatabase::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool EPGDatabase::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


bool EPGDatabase::UpdateSection(
	const EITPfScheduleTable *pScheduleTable, const EITTable *pEITTable,
	EventInfo::SourceIDType SourceID)
{
	if (LIBISDB_TRACE_ERROR_IF(pEITTable == nullptr))
		return false;

	const uint16_t TableID = pEITTable->GetTableID();
	if ((TableID < 0x4E) || (TableID > 0x6F))
		return false;

	BlockLock Lock(m_Lock);

	const bool IsSchedule = (TableID >= 0x50);
	const bool IsExtended = (IsSchedule && ((TableID & 0x08) != 0));
	if (m_ScheduleOnly && !IsSchedule)
		return false;

	const ServiceInfo Key(
		pEITTable->GetOriginalNetworkID(),
		pEITTable->GetTransportStreamID(),
		pEITTable->GetServiceID());
	const auto [itService, ServiceInserted] = m_ServiceMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(Key),
		std::forward_as_tuple());
	ServiceEventMap &Service = itService->second;
	if (ServiceInserted) {
		Service.EventMap.rehash(300);
		Service.ScheduleUpdatedTime = m_CurTOTTime;
	}

	DateTime CurSysTime;
	if (m_NoPastEvents)
		GetCurrentEPGTime(&CurSysTime);

	bool IsUpdated = false;

	const int EventCount = pEITTable->GetEventCount();

	if (EventCount > 0) {
		const uint16_t NetworkID = pEITTable->GetOriginalNetworkID();
		const uint16_t TransportStreamID = pEITTable->GetTransportStreamID();
		const uint16_t ServiceID = pEITTable->GetServiceID();
		ARIBString StrBuf;

		for (int i = 0; i < EventCount; i++) {
			const EITTable::EventInfo *pEventInfo = pEITTable->GetEventInfo(i);

			// 開始/終了時刻が未定義のものは除外する
			if (!pEventInfo->StartTime.IsValid() || (pEventInfo->Duration == 0))
				continue;

			if (m_NoPastEvents) {
				// 既に終了しているものは除外する
				// (時計のずれを考えて5分マージンをとっている)
				DateTime EndTime(pEventInfo->StartTime);
				if (!EndTime.OffsetSeconds(pEventInfo->Duration))
					continue;
				if (EndTime.DiffSeconds(CurSysTime) <= -5 * 60)
					continue;
			}

			bool IsPending = false, IsExtendedOnly = false;

			if (auto itEvent = Service.EventMap.find(pEventInfo->EventID);
					itEvent != Service.EventMap.end()) {
				// 既存のデータの方が新しい場合は除外する
				if (itEvent->second.UpdatedTime > m_CurTOTSeconds) {
					if (m_CurTOTSeconds != 0)
						continue;

					// TOT がまだ来ていない場合は保留
					IsPending = true;
				}

				if (IsExtended
						&& (!(itEvent->second.Type & EventInfo::TypeFlag::Basic)
							|| (itEvent->second.SourceID != SourceID)))
					IsExtendedOnly = true;
			} else {
				if (IsExtended)
					IsExtendedOnly = true;
			}

			ServiceEventMap *pService = &Service;
			ServiceEventMap *pPendingService = nullptr;
			if (m_CurTOTSeconds == 0) {
				pPendingService = &m_PendingServiceMap.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(Key),
					std::forward_as_tuple()).first->second;
				if (IsPending) {
					pService = pPendingService;

					IsExtendedOnly = false;
					if (auto itEvent = pPendingService->EventMap.find(pEventInfo->EventID);
							itEvent != pPendingService->EventMap.end()) {
						if (IsExtended
								&& (!(itEvent->second.Type & EventInfo::TypeFlag::Basic)
									|| (itEvent->second.SourceID != SourceID)))
							IsExtendedOnly = true;
					} else {
						if (IsExtended)
							IsExtendedOnly = true;
					}
				}
			}

			EventMapType &EventMap = IsExtendedOnly ? pService->EventExtendedMap : pService->EventMap;

			if (!IsExtendedOnly) {
				TimeEventInfo TimeEvent(pEventInfo->StartTime);
				TimeEvent.Duration = pEventInfo->Duration;
				TimeEvent.EventID = pEventInfo->EventID;
				TimeEvent.UpdatedTime = m_CurTOTSeconds;

				bool TimeUpdated = false;
				if (!UpdateTimeMap(*pService, TimeEvent, &TimeUpdated))
					continue;
				if (TimeUpdated && !IsPending)
					IsUpdated = true;
			}

			// イベントを追加 or 既存のイベントを取得
			auto EventResult = EventMap.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(pEventInfo->EventID),
				std::forward_as_tuple());
			EventInfo *pEvent = &EventResult.first->second;
			if (!EventResult.second) {
				// 既に番組情報がある場合

				bool IsReset = false;

				if (pEvent->StartTime != pEventInfo->StartTime) {
					// 開始時刻が変わった
					if (!IsExtendedOnly) {
						auto it = pService->TimeMap.find(TimeEventInfo(pEvent->StartTime));
						if ((it != pService->TimeMap.end()) && (it->EventID == pEventInfo->EventID))
							pService->TimeMap.erase(it);
					}

					IsReset = true;
				}

				// 別ソースの情報はマージしない
				if (pEvent->SourceID != SourceID)
					IsReset = true;

				if (IsReset)
					*pEvent = EventInfo();
			}

			pEvent->UpdatedTime       = m_CurTOTSeconds;
			pEvent->SourceID          = SourceID;
			pEvent->NetworkID         = NetworkID;
			pEvent->TransportStreamID = TransportStreamID;
			pEvent->ServiceID         = ServiceID;
			pEvent->EventID           = pEventInfo->EventID;
			pEvent->StartTime         = pEventInfo->StartTime;
			pEvent->Duration          = pEventInfo->Duration;
			pEvent->RunningStatus     = pEventInfo->RunningStatus;
			pEvent->FreeCAMode        = pEventInfo->FreeCAMode;

			if (IsSchedule) {
				if (IsExtended) {
					pEvent->Type |= EventInfo::TypeFlag::Extended;
				} else {
					pEvent->Type |= EventInfo::TypeFlag::Basic;
				}
				pEvent->Type &= ~(EventInfo::TypeFlag::Present | EventInfo::TypeFlag::Following);
			} else {
				// p/f
				pEvent->Type =
					EventInfo::TypeFlag::Basic |
					EventInfo::TypeFlag::Extended |
					((TableID == 0x4E) ? EventInfo::TypeFlag::Present : EventInfo::TypeFlag::Following);
			}

			// extended のみが ServiceEventMap::EventMap に追加されることは無い
			LIBISDB_ASSERT(!!(pEvent->Type & EventInfo::TypeFlag::Basic) || &EventMap == &pService->EventExtendedMap);

			const DescriptorBlock *pDescBlock = &pEventInfo->Descriptors;

			// 短形式イベント記述子
			const ShortEventDescriptor *pShortEvent =
				pDescBlock->GetDescriptor<ShortEventDescriptor>();
			if (pShortEvent != nullptr) {
				if (pShortEvent->GetEventName(&StrBuf))
					m_StringDecoder.Decode(StrBuf, &pEvent->EventName, m_StringDecodeFlags);
				if (pShortEvent->GetEventDescription(&StrBuf))
					m_StringDecoder.Decode(StrBuf, &pEvent->EventText, m_StringDecodeFlags);
			}

			// 拡張形式イベント記述子
			if (!GetEventExtendedTextList(pDescBlock, m_StringDecoder, m_StringDecodeFlags, &pEvent->ExtendedText)) {
				if (!IsExtended)
					MergeEventExtendedInfo(*pService, pEvent);
			}

			// コンポーネント記述子
			if (pDescBlock->GetDescriptorByTag(ComponentDescriptor::TAG) != nullptr) {
				pEvent->VideoList.clear();
				pDescBlock->EnumDescriptors<ComponentDescriptor>(
					[&](const ComponentDescriptor *pComponentDesc) {
						EventInfo::VideoInfo &Info = pEvent->VideoList.emplace_back();

						Info.StreamContent = pComponentDesc->GetStreamContent();
						Info.ComponentType = pComponentDesc->GetComponentType();
						Info.ComponentTag = pComponentDesc->GetComponentTag();
						Info.LanguageCode = pComponentDesc->GetLanguageCode();
						if (pComponentDesc->GetText(&StrBuf))
							m_StringDecoder.Decode(StrBuf, &Info.Text, m_StringDecodeFlags);
					});
			}

			// 音声コンポーネント記述子
			if (pDescBlock->GetDescriptorByTag(AudioComponentDescriptor::TAG) != nullptr) {
				pEvent->AudioList.clear();
				pDescBlock->EnumDescriptors<AudioComponentDescriptor>(
					[&](const AudioComponentDescriptor *pAudioDesc) {
						EventInfo::AudioInfo &Info = pEvent->AudioList.emplace_back();

						Info.StreamContent = pAudioDesc->GetStreamContent();
						Info.ComponentType = pAudioDesc->GetComponentType();
						Info.ComponentTag = pAudioDesc->GetComponentTag();
						Info.SimulcastGroupTag = pAudioDesc->GetSimulcastGroupTag();
						Info.ESMultiLingualFlag = pAudioDesc->GetESMultiLingualFlag();
						Info.MainComponentFlag = pAudioDesc->GetMainComponentFlag();
						Info.QualityIndicator = pAudioDesc->GetQualityIndicator();
						Info.SamplingRate = pAudioDesc->GetSamplingRate();
						Info.LanguageCode = pAudioDesc->GetLanguageCode();
						Info.LanguageCode2 = pAudioDesc->GetLanguageCode2();
						if (pAudioDesc->GetText(&StrBuf))
							m_StringDecoder.Decode(StrBuf, &Info.Text);
					});
			}

			// コンテント記述子
			const ContentDescriptor *pContentDesc = pDescBlock->GetDescriptor<ContentDescriptor>();
			if (pContentDesc != nullptr) {
				int NibbleCount = pContentDesc->GetNibbleCount();
				if (NibbleCount > 7)
					NibbleCount = 7;
				pEvent->ContentNibble.NibbleCount = NibbleCount;
				for (int j = 0; j < NibbleCount; j++)
					pContentDesc->GetNibble(j, &pEvent->ContentNibble.NibbleList[j]);
			}

			// イベントグループ記述子
			if (pDescBlock->GetDescriptorByTag(EventGroupDescriptor::TAG) != nullptr) {
				pEvent->EventGroupList.clear();

				pDescBlock->EnumDescriptors<EventGroupDescriptor>(
					[&](const EventGroupDescriptor *pGroupDesc) {
						EventInfo::EventGroupInfo GroupInfo;
						GroupInfo.GroupType = pGroupDesc->GetGroupType();
						const int EventCount = pGroupDesc->GetEventCount();
						GroupInfo.EventList.resize(EventCount);
						for (int j = 0; j < EventCount; j++)
							pGroupDesc->GetEventInfo(j, &GroupInfo.EventList[j]);

						auto it = std::ranges::find(pEvent->EventGroupList, GroupInfo);
						if (it == pEvent->EventGroupList.end()) {
							pEvent->EventGroupList.push_back(GroupInfo);

							if ((GroupInfo.GroupType == EventGroupDescriptor::GROUP_TYPE_COMMON)
									&& (EventCount == 1)) {
								const EventGroupDescriptor::EventInfo &Info = GroupInfo.EventList.front();
								if (Info.ServiceID != pEITTable->GetServiceID()) {
									pEvent->IsCommonEvent = true;
									pEvent->CommonEvent.ServiceID = Info.ServiceID;
									pEvent->CommonEvent.EventID = Info.EventID;
								}
							}
						}
					});
			}

			if (!IsPending && !IsExtendedOnly) {
				IsUpdated = true;

				if (pPendingService != nullptr)
					MergeEventMapEvent(*pPendingService, EventInfo(*pEvent), MergeFlag::MergeBasicExtended);
			}
		}
	} else {
		// このセグメントで開始するイベントが無い場合

		// イベントが消滅した場合は削除する
		if ((m_CurTOTTime.Hour > 0) || (m_CurTOTTime.Minute > 0) || (m_CurTOTTime.Second >= 30)) {
			if ((TableID >= 0x50 && TableID <= 0x57) || (TableID >= 0x60 && TableID <= 0x67)) {
				// Schedule basic
				const unsigned long long Time = GetScheduleTime(m_CurTOTSeconds, TableID, pEITTable->GetSectionNumber());
				auto it = Service.TimeMap.lower_bound(TimeEventInfo(Time));
				while (it != Service.TimeMap.end()) {
					if ((it->StartTime < Time) || (it->StartTime >= Time + (3 * 60 * 60))
							|| (it->UpdatedTime >= m_CurTOTSeconds))
						break;
					LIBISDB_TRACE(LIBISDB_STR("Segment removed\n"));
					RemoveEvent(Service.EventMap, it->EventID);
					Service.TimeMap.erase(it++);
					IsUpdated = true;
				}
			}
		}
	}

	if (IsUpdated) {
		Service.IsUpdated = true;
		m_IsUpdated = true;
	}

	if (IsSchedule) {
		if ((m_CurTOTTime.Hour > 0) || (m_CurTOTTime.Minute > 0) || (m_CurTOTTime.Second >= 30)) {
			if (Service.ScheduleUpdatedTime.IsValid()
					&& ((Service.ScheduleUpdatedTime.Year != m_CurTOTTime.Year)
						|| (Service.ScheduleUpdatedTime.Month != m_CurTOTTime.Month)
						|| (Service.ScheduleUpdatedTime.Day != m_CurTOTTime.Day))) {
				LIBISDB_TRACE(
					LIBISDB_STR("Reset EPG schedule : NID {:x} / TSID {:x} / SID {:x}\n"),
					itService->first.NetworkID,
					itService->first.TransportStreamID,
					itService->first.ServiceID);
				Service.Schedule.Reset();

				m_EventListenerList.CallEventListener(
					&EventListener::OnScheduleStatusReset,
					this,
					itService->first.NetworkID,
					itService->first.TransportStreamID,
					itService->first.ServiceID);
			}
		}

		const bool IsComplete = Service.Schedule.IsComplete(m_CurTOTTime.Hour, IsExtended);

		if (Service.Schedule.OnSection(pEITTable, m_CurTOTTime.Hour)) {
			if (m_CurTOTTime.IsValid())
				Service.ScheduleUpdatedTime = m_CurTOTTime;

			// 1サービス分の番組情報が揃ったら通知する
			if (!IsComplete && Service.Schedule.IsComplete(m_CurTOTTime.Hour, IsExtended)) {
				LIBISDB_TRACE(
					LIBISDB_STR("EPG schedule {} completed : NID {:x} / TSID {:x} / SID {:x}\n"),
					IsExtended ? LIBISDB_STR("extended") : LIBISDB_STR("basic"),
					itService->first.NetworkID,
					itService->first.TransportStreamID,
					itService->first.ServiceID);

				m_EventListenerList.CallEventListener(
					&EventListener::OnServiceCompleted,
					this,
					itService->first.NetworkID,
					itService->first.TransportStreamID,
					itService->first.ServiceID,
					IsExtended);
			}
		}
	}

	return true;
}


bool EPGDatabase::UpdateTOT(const TOTTable *pTOTTable)
{
	if (LIBISDB_TRACE_ERROR_IF(pTOTTable == nullptr))
		return false;

	DateTime Time;

	if (!pTOTTable->GetDateTime(&Time))
		return false;

	BlockLock Lock(m_Lock);

	m_CurTOTTime = Time;
	m_CurTOTSeconds = Time.GetLinearSeconds();

	// TOT が来るまで保留にしていた情報をマージする
	if (m_CurTOTSeconds != 0 && !m_PendingServiceMap.empty()) {
		for (auto &Service : m_PendingServiceMap) {
			LIBISDB_TRACE(LIBISDB_STR("Merge pending events...\n"));

			for (auto &Event : Service.second.EventMap)
				Event.second.UpdatedTime = m_CurTOTSeconds;
			for (auto &Event : Service.second.EventExtendedMap)
				Event.second.UpdatedTime = m_CurTOTSeconds;
			Service.second.ScheduleUpdatedTime = m_CurTOTTime;

			MergeEventMap(Service.first, Service.second, MergeFlag::MergeBasicExtended | MergeFlag::SetServiceUpdated);
		}

		m_PendingServiceMap.clear();
	}

	return true;
}


void EPGDatabase::ResetTOTTime()
{
	m_CurTOTTime.Reset();
	m_CurTOTSeconds = 0;
}


const EPGDatabase::ServiceEventMap * EPGDatabase::FindServiceEventMap(const ServiceInfo &Info) const
{
	if (Info.TransportStreamID != TRANSPORT_STREAM_ID_INVALID) {
		auto it = m_ServiceMap.find(Info);
		if (it != m_ServiceMap.end())
			return &it->second;
	} else {
		auto it = std::ranges::find_if(
			m_ServiceMap,
			[&Info](const auto &Item) {
				return Item.first.NetworkID == Info.NetworkID
					&& Item.first.ServiceID == Info.ServiceID;
			});
		if (it != m_ServiceMap.end())
			return &it->second;
	}

	return nullptr;
}


bool EPGDatabase::MergeEventMap(
	const ServiceInfo &Info, ServiceEventMap &Map,
	MergeFlag Flags, std::optional<EventInfo::SourceIDType> SourceID)
{
	if (Map.EventMap.empty())
		return false;

	if (SourceID) {
		for (auto &Event : Map.EventMap)
			Event.second.SourceID = *SourceID;
	}

	auto itService = m_ServiceMap.find(Info);
	if (itService == m_ServiceMap.end()) {
		// 新規サービスの追加
		m_ServiceMap.emplace(Info, std::move(Map));
		m_IsUpdated = true;
		return true;
	}

	ServiceEventMap &Service = itService->second;

	if (!!(Flags & MergeFlag::DiscardOldEvents)) {
		// 古い番組情報を破棄する場合
		Service = std::move(Map);
		m_IsUpdated = true;
		return true;
	}

#ifdef LIBISDB_DEBUG
	// 更新される範囲をトレース出力
	{
		DateTime OldestTime, NewestTime;

		Map.EventMap.find(Map.TimeMap.begin()->EventID)->second.GetStartTime(&OldestTime);
		Map.EventMap.find(Map.TimeMap.rbegin()->EventID)->second.GetEndTime(&NewestTime);

		LIBISDB_TRACE(
			LIBISDB_STR("EPGDatabase::MergeEventMap() : [{:x} {:x} {:x}] {}/{} {}:{:02} - {}/{} {}:{:02} {} Events\n"),
			Info.NetworkID, Info.TransportStreamID, Info.ServiceID,
			OldestTime.Month, OldestTime.Day, OldestTime.Hour, OldestTime.Minute,
			NewestTime.Month, NewestTime.Day, NewestTime.Hour, NewestTime.Minute,
			Map.EventMap.size());
	}
#endif

	// 終了している番組を除外する場合、基準となる現在日時を取得
	const bool DiscardEndedEvents = !!(Flags & MergeFlag::DiscardEndedEvents);
	unsigned long long CurTime;
	if (DiscardEndedEvents) {
		DateTime Time;
		GetCurrentEPGTime(&Time);
		CurTime = Time.GetLinearSeconds();
	}

	bool IsUpdated = false;

	// 番組情報の更新
	// XXX: 本来 schedule はセグメント単位で更新した方がよい

	for (auto &Event : Map.EventMap) {
		const TimeEventInfo Time(Event.second);

		// 既に終了している番組を除外
		if (DiscardEndedEvents
				&& (Time.StartTime + Time.Duration <= CurTime))
			continue;

		if (auto itEvent = Service.EventMap.find(Event.first);
				itEvent != Service.EventMap.end()) {
			// 既存のデータの方が新しい場合は除外する
			if (itEvent->second.UpdatedTime > Time.UpdatedTime)
				continue;
		}

		if (MergeEventMapEvent(Service, std::move(Event.second), Flags))
			IsUpdated = true;
	}

	if (IsUpdated) {
		m_IsUpdated = true;

		if (!!(Flags & MergeFlag::SetServiceUpdated))
			Service.IsUpdated = true;
	}

	return true;
}


bool EPGDatabase::MergeEventMapEvent(ServiceEventMap &Service, EventInfo &&NewEvent, MergeFlag Flags)
{
	bool IsUpdated = false;

	if (!UpdateTimeMap(Service, NewEvent, &IsUpdated))
		return false;

	// イベントを追加 or 既存のイベントを取得
	auto EventResult = Service.EventMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(NewEvent.EventID),
		std::forward_as_tuple());
	EventInfo &CurEvent = EventResult.first->second;
	bool Overwrite = true;
	bool DatabaseFlag = !!(Flags & MergeFlag::Database);

	if (!EventResult.second) {
		// 既に番組情報がある場合

		// 開始時刻が変わった場合、古い開始時刻のマップを削除する
		if (CurEvent.StartTime != NewEvent.StartTime) {
			auto it = Service.TimeMap.find(TimeEventInfo(CurEvent.StartTime));
			if ((it != Service.TimeMap.end()) && (it->EventID == CurEvent.EventID))
				Service.TimeMap.erase(it);
		}

		if (!!(Flags & MergeFlag::MergeBasicExtended)) {
			// schedule basic と extended のマージ
			if ((NewEvent.SourceID == CurEvent.SourceID)
					&& (NewEvent.StartTime == CurEvent.StartTime)) {
				if (!(CurEvent.Type & EventInfo::TypeFlag::Extended) && !!(NewEvent.Type & EventInfo::TypeFlag::Extended)) {
					if (!NewEvent.ExtendedText.empty()) {
						CurEvent.ExtendedText = std::move(NewEvent.ExtendedText);
						CurEvent.Type |= EventInfo::TypeFlag::Extended;
					}
					CurEvent.UpdatedTime = NewEvent.UpdatedTime;
					Overwrite = false;
				} else if (!!(CurEvent.Type & EventInfo::TypeFlag::Extended) && !(NewEvent.Type & EventInfo::TypeFlag::Extended)) {
					if (!CurEvent.ExtendedText.empty()) {
						NewEvent.ExtendedText = std::move(CurEvent.ExtendedText);
						NewEvent.Type |= EventInfo::TypeFlag::Extended;
					}
				}
			}
		} else {
			// 新しい番組情報に拡張テキストが無い場合、古い番組情報からコピーする
			if (!NewEvent.HasExtended()
					&& CurEvent.HasExtended()
					&& (NewEvent.SourceID == CurEvent.SourceID)
					&& (NewEvent.StartTime == CurEvent.StartTime)) {
				if (CopyEventExtendedText(&NewEvent, CurEvent)) {
					DatabaseFlag = true;
				}
			}
		}
	}

	if (Overwrite)
		CurEvent = std::move(NewEvent);

	MergeEventExtendedInfo(Service, &CurEvent);

	if (DatabaseFlag)
		CurEvent.Type |= EventInfo::TypeFlag::Database;
	else
		CurEvent.Type &= ~EventInfo::TypeFlag::Database;

	return true;
}


bool EPGDatabase::UpdateTimeMap(ServiceEventMap &Service, const TimeEventInfo &Time, bool *pIsUpdated)
{
	bool IsUpdated = false;
	auto TimeResult = Service.TimeMap.insert(Time);
	auto itCur = TimeResult.first;

	if (TimeResult.second
			|| (itCur->Duration != Time.Duration)
			|| (itCur->EventID != Time.EventID)) {
		if (!TimeResult.second) {
			// 既存のデータの方が新しい場合は除外する
			if (itCur->UpdatedTime > Time.UpdatedTime)
				return false;
		}

		// 時間が被っていないか調べる
		bool Skip = false;
		auto it = itCur;

		for (++it; it != Service.TimeMap.end();) {
			if (it->StartTime >= Time.StartTime + Time.Duration)
				break;
			if (it->UpdatedTime > Time.UpdatedTime) {
				Skip = true;
				break;
			}
			LIBISDB_TRACE(LIBISDB_STR("Event overlapped\n"));
			RemoveEvent(Service.EventMap, it->EventID);
			Service.TimeMap.erase(it++);
			IsUpdated = true;
		}

		if (!Skip && (itCur != Service.TimeMap.begin())) {
			it = itCur;
			--it;
			for (;;) {
				if (it->StartTime + it->Duration <= Time.StartTime)
					break;
				if (it->UpdatedTime > Time.UpdatedTime) {
					Skip = true;
					break;
				}
				LIBISDB_TRACE(LIBISDB_STR("Event overlapped\n"));
				RemoveEvent(Service.EventMap, it->EventID);
				IsUpdated = true;
				if (it == Service.TimeMap.begin()) {
					Service.TimeMap.erase(it);
					break;
				}
				Service.TimeMap.erase(it--);
			}
		}

		if (Skip) {
			if (TimeResult.second)
				Service.TimeMap.erase(itCur);
			if (IsUpdated)
				*pIsUpdated = true;
			return false;
		}

		if (!TimeResult.second) {
			if (itCur->EventID != Time.EventID) {
				LIBISDB_TRACE(LIBISDB_STR("event_id changed ({:04x} -> {:04})\n"), itCur->EventID, Time.EventID);
				RemoveEvent(Service.EventMap, itCur->EventID);
			}
		}
	}

	if (!TimeResult.second) {
		Service.TimeMap.erase(itCur);
		Service.TimeMap.insert(Time);
		IsUpdated = true;
	}

	if (IsUpdated)
		*pIsUpdated = true;

	return true;
}


const EventInfo * EPGDatabase::GetEventInfoByIDs(
	uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID, uint16_t EventID) const
{
	auto itService = m_ServiceMap.find(ServiceInfo(NetworkID, TransportStreamID, ServiceID));
	if (itService == m_ServiceMap.end())
		return nullptr;

	auto itEvent = itService->second.EventMap.find(EventID);
	if (itEvent == itService->second.EventMap.end())
		return nullptr;

	return &itEvent->second;
}


bool EPGDatabase::SetCommonEventInfo(EventInfo *pInfo) const
{
	// イベント共有の参照先から情報を取得する
	if (pInfo->IsCommonEvent) {
		const EventInfo *pCommonEvent = GetEventInfoByIDs(
			pInfo->NetworkID,
			pInfo->TransportStreamID,
			pInfo->CommonEvent.ServiceID,
			pInfo->CommonEvent.EventID);

		if (pCommonEvent != nullptr) {
			pInfo->EventName     = pCommonEvent->EventName;
			pInfo->EventText     = pCommonEvent->EventText;
			pInfo->ExtendedText  = pCommonEvent->ExtendedText;
			pInfo->FreeCAMode    = pCommonEvent->FreeCAMode;
			pInfo->VideoList     = pCommonEvent->VideoList;
			pInfo->AudioList     = pCommonEvent->AudioList;
			pInfo->ContentNibble = pCommonEvent->ContentNibble;

			return true;
		}
	}

	return false;
}


bool EPGDatabase::CopyEventExtendedText(EventInfo *pDstInfo, const EventInfo &SrcInfo) const
{
	if (pDstInfo->ExtendedText.empty()
			&& !SrcInfo.ExtendedText.empty()
			&& (pDstInfo->EventName == SrcInfo.EventName)) {
		pDstInfo->ExtendedText = SrcInfo.ExtendedText;
		return true;
	}

	return false;
}


bool EPGDatabase::MergeEventExtendedInfo(ServiceEventMap &Service, EventInfo *pEvent)
{
	auto itEvent = Service.EventExtendedMap.find(pEvent->EventID);
	if (itEvent == Service.EventExtendedMap.end())
		return false;

	EventInfo &ExtendedInfo = itEvent->second;

	if ((pEvent->SourceID != ExtendedInfo.SourceID)
			|| (pEvent->StartTime != ExtendedInfo.StartTime))
		return false;

	if (!pEvent->ExtendedText.empty() && (pEvent->UpdatedTime > ExtendedInfo.UpdatedTime)) {
		Service.EventExtendedMap.erase(itEvent);
		return false;
	}

	LIBISDB_TRACE(
		LIBISDB_STR("Merge extended info : [{:04x}] {}/{}/{} {}:{:02}:{:02}\n"),
		pEvent->EventID,
		pEvent->StartTime.Year, pEvent->StartTime.Month, pEvent->StartTime.Day,
		pEvent->StartTime.Hour, pEvent->StartTime.Minute, pEvent->StartTime.Second);

	pEvent->ExtendedText = std::move(ExtendedInfo.ExtendedText);
	pEvent->Type |= EventInfo::TypeFlag::Extended;
	if (pEvent->UpdatedTime < ExtendedInfo.UpdatedTime)
		pEvent->UpdatedTime = ExtendedInfo.UpdatedTime;

	Service.EventExtendedMap.erase(itEvent);

	return true;
}


bool EPGDatabase::RemoveEvent(EventMapType &Map, uint16_t EventID)
{
	auto it = Map.find(EventID);
	if (it == Map.end())
		return false;

	LIBISDB_TRACE(
		LIBISDB_STR("EPGDatabase::RemoveEvent() : [{:04x}] {}/{}/{} {}:{:02}:{:02} {}\n"),
		EventID,
		it->second.StartTime.Year, it->second.StartTime.Month, it->second.StartTime.Day,
		it->second.StartTime.Hour, it->second.StartTime.Minute, it->second.StartTime.Second,
		it->second.EventName);

	Map.erase(it);

	return true;
}




EPGDatabase::TimeEventInfo::TimeEventInfo(unsigned long long Time)
	: StartTime(Time)
{
}


EPGDatabase::TimeEventInfo::TimeEventInfo(const DateTime &StartTime)
	: StartTime(StartTime.GetLinearSeconds())
{
}


EPGDatabase::TimeEventInfo::TimeEventInfo(const EventInfo &Info)
	: StartTime(Info.StartTime.GetLinearSeconds())
	, Duration(Info.Duration)
	, EventID(Info.EventID)
	, UpdatedTime(Info.UpdatedTime)
{
}




void EPGDatabase::ScheduleInfo::Reset()
{
	m_Basic.TableCount = 0;
	m_Extended.TableCount = 0;
}


bool EPGDatabase::ScheduleInfo::IsComplete(int Hour, bool Extended) const
{
	const TableList &List = (Extended ? m_Extended : m_Basic);

	if (List.TableCount == 0)
		return false;

#if 0
	for (int i = 0; i < List.TableCount; i++) {
		if (!IsTableComplete(i, Hour, Extended))
			return false;
	}
#else
	if (!List.Table[0].IsComplete) {
		if (!IsTableComplete(0, Hour, Extended))
			return false;
	}
	for (int i = 1; i < List.TableCount; i++) {
		if (!List.Table[i].IsComplete)
			return false;
	}
#endif

	return true;
}


bool EPGDatabase::ScheduleInfo::IsTableComplete(int TableIndex, int Hour, bool Extended) const
{
	const TableList &TableList = (Extended ? m_Extended : m_Basic);

	if ((TableIndex < 0) || (TableIndex >= TableList.TableCount)
			|| ((TableIndex == 0) && ((Hour < 0) || (Hour > 23))))
		return false;

	const TableInfo &Table = TableList.Table[TableIndex];

	for (int i = ((TableIndex == 0) ? (Hour / 3) : 0); i < 32; i++) {
		const SegmentInfo &Segment = Table.SegmentList[i];

		if (Segment.SectionCount == 0)
			return false;

		if (Segment.SectionFlags != (1 << Segment.SectionCount) - 1)
			return false;
	}

	return true;
}


bool EPGDatabase::ScheduleInfo::HasSchedule(bool Extended) const
{
	return (Extended ? m_Extended : m_Basic).TableCount > 0;
}


bool EPGDatabase::ScheduleInfo::OnSection(const EITTable *pTable, int Hour)
{
	const uint8_t TableID = pTable->GetTableID();
	const uint8_t LastTableID = pTable->GetLastTableID();
	const uint8_t FirstTableID = LastTableID & 0xF8;
	const uint8_t SectionNumber = pTable->GetSectionNumber();
	const uint8_t LastSectionNumber = pTable->GetSegmentLastSectionNumber();
	const uint8_t FirstSectionNumber = LastSectionNumber & 0xF8;

	if ((TableID < 0x50) || (TableID > 0x6F)
			|| (TableID < FirstTableID) || (TableID > LastTableID)
			|| (SectionNumber < FirstSectionNumber) || (SectionNumber > LastSectionNumber)) {
		LIBISDB_TRACE_WARNING(
			LIBISDB_STR("EPGDatabase::ScheduleInfo::OnSection() : table_id or section_number out of range : table_id {:x}[{:x} - {:x}] / section_number {:x}[{:x} - {:x}]\n"),
			TableID, FirstTableID, LastTableID,
			SectionNumber, FirstSectionNumber, LastSectionNumber);
		return false;
	}

	const bool IsExtended = ((TableID & 0x08) != 0);
	TableList *pTableList = (IsExtended ? &m_Extended : &m_Basic);
	const uint8_t TableCount = (LastTableID - FirstTableID) + 1;
	const int TableIndex = TableID & 0x07;
	TableInfo &Table = pTableList->Table[TableIndex];

	if (pTableList->TableCount != TableCount) {
		pTableList->TableCount = TableCount;
		std::memset(pTableList->Table, 0, sizeof(pTableList->Table));
		Table.Version = pTable->GetVersionNumber();
	} else if (pTable->GetVersionNumber() != Table.Version) {
		Table.Version = pTable->GetVersionNumber();
		Table.IsComplete = false;
		std::memset(Table.SegmentList, 0, sizeof(Table.SegmentList));
	}

	SegmentInfo &Segment = Table.SegmentList[SectionNumber >> 3];
	const uint8_t SectionCount = (LastSectionNumber - FirstSectionNumber) + 1;
	const uint8_t SectionFlag = 1 << (SectionNumber & 0x07);

	if (Segment.SectionCount != SectionCount) {
		Segment.SectionCount = SectionCount;
		Segment.SectionFlags = 0;
	}

	if ((Segment.SectionFlags & SectionFlag) == 0) {
		Segment.SectionFlags |= SectionFlag;

		LIBISDB_ASSERT(!Table.IsComplete);
		if (Segment.SectionFlags == (1 << Segment.SectionCount) - 1) {
			Table.IsComplete = IsTableComplete(TableIndex, Hour, IsExtended);
		}
	}

	return true;
}


}	// namespace LibISDB
