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
 @file   EPGDatabase.hpp
 @brief  番組情報データベース
 @author DBCTRADO
*/


#ifndef LIBISDB_EPG_DATABASE_H
#define LIBISDB_EPG_DATABASE_H


#include "EventInfo.hpp"
#include "../Base/EventListener.hpp"
#include "../Utilities/Lock.hpp"
#include "../TS/Tables.hpp"
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <functional>


namespace LibISDB
{

	/** 番組情報データベースクラス */
	class EPGDatabase
	{
	public:
		/** イベントリスナ */
		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnServiceCompleted(
				EPGDatabase *pEPGDatabase,
				uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
				bool IsExtended) {}
			virtual void OnScheduleStatusReset(
				EPGDatabase *pEPGDatabase,
				uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID) {}
		};

		typedef std::vector<EventInfo> EventList;

		struct ServiceInfo {
			uint16_t NetworkID;
			uint16_t TransportStreamID;
			uint16_t ServiceID;

			ServiceInfo() noexcept
				: NetworkID(NETWORK_ID_INVALID)
				, TransportStreamID(TRANSPORT_STREAM_ID_INVALID)
				, ServiceID(SERVICE_ID_INVALID)
			{
			}

			ServiceInfo(uint16_t NID, uint16_t TSID, uint16_t SID) noexcept
				: NetworkID(NID)
				, TransportStreamID(TSID)
				, ServiceID(SID)
			{
			}

			bool operator == (const ServiceInfo &rhs) const noexcept = default;

			bool operator < (const ServiceInfo &rhs) const noexcept
			{
				return GetKey() < rhs.GetKey();
			}

			unsigned long long GetKey() const noexcept
			{
				return (static_cast<unsigned long long>(NetworkID) << 32)
				     | (static_cast<unsigned long long>(TransportStreamID) << 16)
				     | (static_cast<unsigned long long>(ServiceID));
			}
		};

		typedef std::vector<ServiceInfo> ServiceList;

		struct TimeEventInfo {
			unsigned long long StartTime;
			uint32_t Duration;
			uint16_t EventID;
			unsigned long long UpdatedTime;

			TimeEventInfo(unsigned long long Time);
			TimeEventInfo(const DateTime &StartTime);
			TimeEventInfo(const EventInfo &Info);

			bool operator < (const TimeEventInfo &Obj) const noexcept
			{
				return StartTime < Obj.StartTime;
			}
		};

		typedef std::set<TimeEventInfo> TimeEventMap;

		enum class MergeFlag : unsigned int {
			None               = 0x0000U,
			DiscardOldEvents   = 0x0001U,
			DiscardEndedEvents = 0x0002U,
			Database           = 0x0004U,
			MergeBasicExtended = 0x0008U,
			SetServiceUpdated  = 0x0010U,
		};

		EPGDatabase() noexcept;

		void Clear();
		int GetServiceCount() const;
		bool GetServiceList(ServiceList *pList) const;
		bool IsServiceUpdated(uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID) const;
		bool ResetServiceUpdated(uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID);

		bool GetEventList(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			ReturnArg<EventList> List, OptionalReturnArg<TimeEventMap> TimeMap = std::nullopt) const;
		bool GetEventListSortedByTime(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			ReturnArg<EventList> List) const;

		bool GetEventInfo(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			uint16_t EventID, ReturnArg<EventInfo> Info) const;
		bool GetEventInfo(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			const DateTime &Time, ReturnArg<EventInfo> Info) const;
		bool GetNextEventInfo(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			const DateTime &Time, ReturnArg<EventInfo> Info) const;

		bool EnumEventsUnsorted(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			const std::function<bool(const EventInfo &Event)> &Callback) const;
		bool EnumEventsSortedByTime(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			const std::function<bool(const EventInfo &Event)> &Callback) const;
		bool EnumEventsSortedByTime(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			const DateTime *pEarliest, const DateTime *pLatest,
			const std::function<bool(const EventInfo &Event)> &Callback) const;

		bool SetServiceEventList(const ServiceInfo &Info, EventList &&List);

		bool Merge(
			EPGDatabase *pSrcDatabase,
			MergeFlag Flags = MergeFlag::None,
			std::optional<EventInfo::SourceIDType> SourceID = std::nullopt);
		bool MergeService(
			EPGDatabase *pSrcDatabase,
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			MergeFlag Flags = MergeFlag::None,
			std::optional<EventInfo::SourceIDType> SourceID = std::nullopt);

		bool IsScheduleComplete(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			bool Extended = false) const;
		bool HasSchedule(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID,
			bool Extended = false) const;
		void ResetScheduleStatus();

		bool IsUpdated() const noexcept { return m_IsUpdated; }
		void SetUpdated(bool Updated) { m_IsUpdated = Updated; }

		void SetScheduleOnly(bool ScheduleOnly);
		bool GetScheduleOnly() const noexcept { return m_ScheduleOnly; }
		void SetNoPastEvents(bool NoPastEvents);
		bool GetNoPastEvents() const noexcept { return m_NoPastEvents; }
		void SetStringDecodeFlags(ARIBStringDecoder::DecodeFlag Flags);
		ARIBStringDecoder::DecodeFlag GetStringDecodeFlags() const noexcept { return m_StringDecodeFlags; }

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

		bool UpdateSection(
			const EITPfScheduleTable *pScheduleTable, const EITTable *pEITTable,
			EventInfo::SourceIDType SourceID = 0);
		bool UpdateTOT(const TOTTable *pTOTTable);
		void ResetTOTTime();

		MutexLock & GetLock() noexcept { return m_Lock; }

	protected:
		class ScheduleInfo
		{
		public:
			void Reset();
			bool IsComplete(int Hour, bool Extended) const;
			bool IsTableComplete(int TableIndex, int Hour, bool Extended) const;
			bool HasSchedule(bool Extended) const;
			bool OnSection(const EITTable *pTable, int Hour);

		private:
			struct SegmentInfo {
				uint8_t SectionCount;
				uint8_t SectionFlags;
			};

			struct TableInfo {
				uint8_t Version;
				bool IsComplete;
				SegmentInfo SegmentList[32];
			};

			struct TableList {
				uint8_t TableCount = 0;
				TableInfo Table[8];
			};

			TableList m_Basic;
			TableList m_Extended;
		};

		typedef std::unordered_map<uint16_t, EventInfo> EventMapType;

		struct ServiceEventMap {
			EventMapType EventMap;
			EventMapType EventExtendedMap;
			TimeEventMap TimeMap;
			bool IsUpdated = false;
			ScheduleInfo Schedule;
			DateTime ScheduleUpdatedTime;
		};

		typedef std::map<ServiceInfo, ServiceEventMap> ServiceMap;

		ServiceMap m_ServiceMap;
		ServiceMap m_PendingServiceMap;
		mutable MutexLock m_Lock;
		bool m_IsUpdated;
		bool m_ScheduleOnly;
		bool m_NoPastEvents;
		ARIBStringDecoder m_StringDecoder;
		ARIBStringDecoder::DecodeFlag m_StringDecodeFlags;
		DateTime m_CurTOTTime;
		unsigned long long m_CurTOTSeconds;
		EventListenerList<EventListener> m_EventListenerList;

		bool MergeEventMap(
			const ServiceInfo &Info, ServiceEventMap &Map,
			MergeFlag Flags = MergeFlag::None,
			std::optional<EventInfo::SourceIDType> SourceID = std::nullopt);
		bool MergeEventMapEvent(
			ServiceEventMap &Service, EventInfo &&NewEvent,
			MergeFlag Flags = MergeFlag::None);
		bool UpdateTimeMap(ServiceEventMap &Service, const TimeEventInfo &Time, bool *pIsUpdated);
		const EventInfo * GetEventInfoByIDs(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID, uint16_t EventID) const;
		bool SetCommonEventInfo(EventInfo *pInfo) const;
		bool CopyEventExtendedText(EventInfo *pDstInfo, const EventInfo &SrcInfo) const;
		bool MergeEventExtendedInfo(ServiceEventMap &Service, EventInfo *pEvent);

		static bool RemoveEvent(EventMapType &Map, uint16_t EventID);
	};

	LIBISDB_ENUM_FLAGS(EPGDatabase::MergeFlag)

}	// namespace LibISDB


#endif	// ifndef LIBISDB_EPG_DATABASE_H
