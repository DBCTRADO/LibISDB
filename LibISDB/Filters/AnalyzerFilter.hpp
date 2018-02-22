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
 @file   AnalyzerFilter.hpp
 @brief  解析フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_ANALYZER_FILTER_H
#define LIBISDB_ANALYZER_FILTER_H


#include "FilterBase.hpp"
#include "../TS/PIDMap.hpp"
#include "../TS/Descriptors.hpp"
#include "../TS/Tables.hpp"
#include "../EPG/EventInfo.hpp"
#include "../Base/EventListener.hpp"
#include <vector>
#include <map>


#ifndef LIBISDB_ANALYZER_FILTER_NO_EIT
// EIT の解析を行う
#define LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
// L-EIT[p/f] の解析を行う
#define LIBISDB_ANALYZER_FILTER_L_EIT_SUPPORT
#endif


namespace LibISDB
{

	/** 解析フィルタクラス */
	class AnalyzerFilter
		: public SingleIOFilter
	{
	public:
		static constexpr uint16_t LOGO_ID_INVALID = LogoTransmissionDescriptor::LOGO_ID_INVALID;

		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnPATUpdated(AnalyzerFilter *pAnalyzer) {}
			virtual void OnPMTUpdated(AnalyzerFilter *pAnalyzer, uint16_t ServiceID) {}
			virtual void OnSDTUpdated(AnalyzerFilter *pAnalyzer) {}
			virtual void OnNITUpdated(AnalyzerFilter *pAnalyzer) {}
			virtual void OnEITUpdated(AnalyzerFilter *pAnalyzer) {}
			virtual void OnCATUpdated(AnalyzerFilter *pAnalyzer) {}
			virtual void OnTOTUpdated(AnalyzerFilter *pAnalyzer) {}
		};

		struct ESInfo {
			uint16_t PID = PID_INVALID;
			uint8_t StreamType = STREAM_TYPE_INVALID;
			uint8_t ComponentTag = COMPONENT_TAG_INVALID;
			uint8_t QualityLevel = 0xFF_u8;
			uint16_t HierarchicalReferencePID = PID_INVALID;
		};

		typedef std::vector<ESInfo> ESInfoList;

		struct ECMInfo {
			uint16_t CASystemID;
			uint16_t PID;
		};

		struct ServiceInfo {
			bool IsPMTAcquired;
			uint16_t ServiceID;
			uint16_t PMTPID;
			std::vector<ESInfo> ESList;
			std::vector<ESInfo> VideoESList;
			std::vector<ESInfo> AudioESList;
			std::vector<ESInfo> CaptionESList;
			std::vector<ESInfo> DataCarrouselESList;
			std::vector<ESInfo> OtherESList;
			uint16_t PCRPID;
			std::vector<ECMInfo> ECMList;
			uint8_t RunningStatus;
			bool FreeCAMode;
			String ProviderName;
			String ServiceName;
			uint8_t ServiceType;
			uint16_t LogoID;
		};

		typedef std::vector<ServiceInfo> ServiceList;

		struct SDTServiceInfo {
			uint16_t ServiceID;
			uint8_t RunningStatus;
			bool FreeCAMode;
			String ProviderName;
			String ServiceName;
			uint8_t ServiceType;
			uint16_t LogoID;
		};

		typedef std::vector<SDTServiceInfo> SDTServiceList;

		struct SDTStreamInfo {
			uint16_t TransportStreamID;
			uint16_t OriginalNetworkID;
			SDTServiceList ServiceList;
		};

		typedef std::vector<SDTStreamInfo> SDTStreamList;
		typedef std::map<uint32_t, SDTStreamInfo> SDTStreamMap;
		static constexpr uint32_t SDTStreamMapKey(uint16_t NetworkID, uint16_t TransportStreamID) {
			return (static_cast<uint32_t>(NetworkID) << 16) | static_cast<uint32_t>(TransportStreamID);
		}

		typedef ServiceListDescriptor::ServiceInfo NetworkServiceInfo;

		struct NetworkStreamInfo {
			uint16_t TransportStreamID;
			uint16_t OriginalNetworkID;
			std::vector<NetworkServiceInfo> ServiceList;
		};

		typedef std::vector<NetworkStreamInfo> NetworkStreamList;

		struct SatelliteDeliverySystemInfo {
			uint16_t TransportStreamID;
			uint32_t Frequency;
			uint16_t OrbitalPosition;
			bool WestEastFlag;
			uint8_t Polarization;
			uint8_t Modulation;
			uint32_t SymbolRate;
			uint8_t FECInner;
		};

		struct TerrestrialDeliverySystemInfo {
			uint16_t TransportStreamID;
			uint16_t AreaCode;
			uint8_t GuardInterval;
			uint8_t TransmissionMode;
			std::vector<uint16_t> Frequency;
		};

		typedef std::vector<SatelliteDeliverySystemInfo> SatelliteDeliverySystemList;
		typedef std::vector<TerrestrialDeliverySystemInfo> TerrestrialDeliverySystemList;

		typedef std::vector<uint16_t> EMMPIDList;

		AnalyzerFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("AnalyzerFilter"); }

	// FilterBase
		void Reset() override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	// AnalyzerFilter
		int GetServiceCount() const;
		uint16_t GetServiceID(int Index) const;
		int GetServiceIndexByID(uint16_t ServiceID) const;
		bool GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const;
		bool GetServiceInfoByID(uint16_t ServiceID, ReturnArg<ServiceInfo> Info) const;
		bool IsServicePMTAcquired(int Index) const;
		bool Is1SegService(int Index) const;
		uint16_t GetPMTPID(int Index) const;

		int GetVideoESCount(int Index) const;
		bool GetVideoESList(int Index, ReturnArg<ESInfoList> ESList) const;
		bool GetVideoESInfo(int Index, int VideoIndex, ReturnArg<ESInfo> ESInfo) const;
		uint16_t GetVideoESPID(int Index, int VideoIndex) const;
		uint8_t GetVideoStreamType(int Index, int VideoIndex) const;
		uint8_t GetVideoComponentTag(int Index, int VideoIndex) const;
		int GetVideoIndexByComponentTag(int Index, uint8_t ComponentTag) const;

		int GetAudioESCount(int Index) const;
		bool GetAudioESList(int Index, ReturnArg<ESInfoList> ESList) const;
		bool GetAudioESInfo(int Index, int AudioIndex, ReturnArg<ESInfo> ESInfo) const;
		uint16_t GetAudioESPID(int Index, int AudioIndex) const;
		uint8_t GetAudioStreamType(int Index, int AudioIndex) const;
		uint8_t GetAudioComponentTag(int Index, int AudioIndex) const;
		int GetAudioIndexByComponentTag(int Index, uint8_t ComponentTag) const;

#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
		uint8_t GetVideoComponentType(int Index) const;
		uint8_t GetAudioComponentType(int Index, int AudioIndex) const;
		bool GetAudioComponentText(int Index, int AudioIndex, ReturnArg<String> Text) const;
#endif

		int GetCaptionESCount(int Index) const;
		uint16_t GetCaptionESPID(int Index, int CaptionIndex) const;

		int GetDataCarrouselESCount(int Index) const;
		uint16_t GetDataCarrouselESPID(int Index, int DataCarrouselIndex) const;

		uint16_t GetPCRPID(int Index) const;
		uint64_t GetPCRTimeStamp(int Index) const;

		bool GetServiceName(int Index, ReturnArg<String> Name) const;
		uint8_t GetServiceType(int Index) const;
		uint16_t GetLogoID(int Index) const;

		uint16_t GetTransportStreamID() const;
		uint16_t GetNetworkID() const;
		uint8_t GetBroadcastingID() const;
		bool GetNetworkName(ReturnArg<String> Name) const;
		uint8_t GetRemoteControlKeyID() const;
		bool GetTSName(ReturnArg<String> Name) const;
		bool GetServiceList(ReturnArg<ServiceList> List) const;
		bool GetSDTServiceList(ReturnArg<SDTServiceList> List) const;
		bool GetSDTStreamList(ReturnArg<SDTStreamList> List) const;
		bool GetNetworkStreamList(ReturnArg<NetworkStreamList> List) const;

		bool IsPATUpdated() const;
		bool IsSDTUpdated() const;
		bool IsNITUpdated() const;
#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
		bool IsEITUpdated() const;
#endif
		bool IsSDTComplete() const;

		bool Is1SegStream() const;
		bool Has1SegService() const;
		uint16_t GetFirst1SegServiceID() const;
		uint16_t Get1SegServiceIDByIndex(int Index) const;

#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
		typedef EventInfo::VideoInfo EventVideoInfo;
		typedef EventInfo::VideoInfoList EventVideoList;
		typedef EventInfo::AudioInfo EventAudioInfo;
		typedef EventInfo::AudioInfoList EventAudioList;
		typedef EventInfo::ContentNibbleInfo EventContentNibble;
		typedef EventInfo::SeriesInfo EventSeriesInfo;

		struct EventComponentGroupInfo {
			uint8_t ComponentGroupID;
			uint8_t NumOfCAUnit;
			ComponentGroupDescriptor::CAUnitInfo CAUnitList[16];
			uint8_t TotalBitRate;
			String Text;
		};

		typedef std::vector<EventComponentGroupInfo> EventComponentGroupList;

		uint16_t GetEventID(int ServiceIndex, bool Next = false) const;
		bool GetEventStartTime(int ServiceIndex, ReturnArg<DateTime> StartTime, bool Next = false) const;
		uint32_t GetEventDuration(int ServiceIndex, bool Next = false) const;
		bool GetEventTime(int ServiceIndex, OptionalReturnArg<DateTime> Time, OptionalReturnArg<uint32_t> Duration = std::nullopt, bool Next = false) const;
		bool GetEventName(int ServiceIndex, ReturnArg<String> Name, bool Next = false) const;
		bool GetEventText(int ServiceIndex, ReturnArg<String> Text, bool Next = false) const;
		bool GetEventExtendedText(int ServiceIndex, ReturnArg<String> Text, bool UseEventGroup = true, bool Next = false) const;
		bool GetEventExtendedText(int ServiceIndex, ReturnArg<EventInfo::ExtendedTextInfoList> List, bool UseEventGroup = true, bool Next = false) const;
		bool GetEventVideoInfo(int ServiceIndex, int VideoIndex, ReturnArg<EventVideoInfo> Info, bool Next = false) const;
		bool GetEventVideoList(int ServiceIndex, ReturnArg<EventVideoList> List, bool Next = false) const;
		bool GetEventAudioInfo(int ServiceIndex, int AudioIndex, ReturnArg<EventAudioInfo> Info, bool Next = false) const;
		bool GetEventAudioList(int ServiceIndex, ReturnArg<EventAudioList> List, bool Next = false) const;
		bool GetEventContentNibble(int ServiceIndex, ReturnArg<EventContentNibble> Info, bool Next = false) const;
		bool GetEventSeriesInfo(int ServiceIndex, ReturnArg<EventSeriesInfo> Info, bool Next = false) const;
		bool GetEventInfo(int ServiceIndex, ReturnArg<EventInfo> Info, bool UseEventGroup = true, bool Next = false) const;
		int GetEventComponentGroupCount(int ServiceIndex, bool Next = false) const;
		bool GetEventComponentGroupInfo(int ServiceIndex, int GroupIndex, ReturnArg<EventComponentGroupInfo> Info, bool Next = false) const;
		bool GetEventComponentGroupList(int ServiceIndex, ReturnArg<EventComponentGroupList> List, bool Next = false) const;
		int GetEventComponentGroupIndexByComponentTag(int ServiceIndex, uint8_t ComponentTag, bool Next = false) const;
#endif	// LIBISDB_ANALYZER_FILTER_EIT_SUPPORT

		bool GetTOTTime(ReturnArg<DateTime> Time) const;
		bool GetInterpolatedTOTTime(ReturnArg<DateTime> Time, OptionalReturnArg<bool> Interpolated = std::nullopt) const;

		bool GetSatelliteDeliverySystemList(ReturnArg<SatelliteDeliverySystemList> List) const;
		bool GetTerrestrialDeliverySystemList(ReturnArg<TerrestrialDeliverySystemList> List) const;

		bool GetEMMPIDList(ReturnArg<EMMPIDList> List) const;

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

	protected:
#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
		const class EITTable * GetEITPfTableByServiceID(uint16_t ServiceID, bool Next = false) const;
		const DescriptorBlock * GetHEITItemDesc(int ServiceIndex, bool Next = false) const;
#ifdef LIBISDB_ANALYZER_FILTER_L_EIT_SUPPORT
		const DescriptorBlock * GetLEITItemDesc(int ServiceIndex, bool Next = false) const;
#endif
		const DescriptorBlock * GetExtendedEventDescriptor(int ServiceIndex, bool UseEventGroup, bool Next) const;
		const ComponentDescriptor * GetComponentDescByComponentTag(const DescriptorBlock *pDescBlock, uint8_t ComponentTag) const;
		const AudioComponentDescriptor * GetAudioComponentDescByComponentTag(const DescriptorBlock *pDescBlock, uint8_t ComponentTag) const;
		void ComponentDescToVideoInfo(const ComponentDescriptor *pComponentDesc, ReturnArg<EventVideoInfo> Info) const;
		void AudioComponentDescToAudioInfo(const AudioComponentDescriptor *pAudioDesc, ReturnArg<EventAudioInfo> Info) const;
#endif

		void GetSDTServiceInfo(ServiceInfo *pServiceInfo, const SDTTable *pSDTTable, int SDTIndex);
		void UpdateSDTServiceList(const SDTTable *pSDTTable, ReturnArg<SDTServiceList> List);
		void UpdateSDTStreamMap(const SDTTable *pSDTTable, SDTStreamMap *pStreamMap);

		struct NITInfo {
			uint8_t BroadcastingFlag = 0;
			uint8_t BroadcastingID = 0;
			uint8_t RemoteControlKeyID = 0;
			String NetworkName;
			String TSName;

			void Reset() { *this = NITInfo(); }
		};

		struct TOTInterpolationInfo {
			uint16_t PCRPID;
			uint64_t PCRTime;
		};

		PIDMapManager m_PIDMapManager;

		uint16_t m_TransportStreamID;
		uint16_t m_NetworkID;

		bool m_PATUpdated;
		bool m_SDTUpdated;
		bool m_NITUpdated;
#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
		bool m_EITUpdated;
		bool m_SendEITUpdatedEvent;
#endif

		ServiceList m_ServiceList;
		SDTServiceList m_SDTServiceList;
		SDTStreamMap m_SDTStreamMap;
		NetworkStreamList m_NetworkStreamList;
		NITInfo m_NITInfo;
		EMMPIDList m_EMMPIDList;

		EventListenerList<EventListener> m_EventListenerList;

		mutable ARIBStringDecoder m_StringDecoder;

		TOTInterpolationInfo m_TOTInterpolation;

	private:
		void OnPATSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnPMTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnSDTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnNITSection(const PSITableBase *pTable, const PSISection *pSection);
#ifdef LIBISDB_ANALYZER_FILTER_EIT_SUPPORT
		void OnEITSection(const PSITableBase *pTable, const PSISection *pSection);
#endif
		void OnCATSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnTOTSection(const PSITableBase *pTable, const PSISection *pSection);
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ANALYZER_FILTER_H
