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
 @file   TSEngine.hpp
 @brief  TSエンジン
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_ENGINE_H
#define LIBISDB_TS_ENGINE_H


#include "FilterGraph.hpp"
#include "../Filters/SourceFilter.hpp"
#include "../Filters/AnalyzerFilter.hpp"
#include <initializer_list>


namespace LibISDB
{

	/** TSエンジンクラス */
	class TSEngine
		: public ObjectBase
		, protected SourceFilter::EventListener
		, protected AnalyzerFilter::EventListener
	{
	public:
		enum class OneSegSelectType {
			LowPriority,
			HighPriority,
			Refuse,
		};

		struct ServiceSelectInfo {
			uint16_t ServiceID = SERVICE_ID_INVALID;
			bool FollowViewableService = false;
			OneSegSelectType OneSegSelect = OneSegSelectType::LowPriority;
			int PreferredServiceIndex = -1;

			void Reset() noexcept { *this = ServiceSelectInfo(); }
		};

		TSEngine() noexcept;
		virtual ~TSEngine();

		bool BuildEngine(const FilterGraph::ConnectionInfo *pConnectionList, size_t ConnectionCount);
		bool BuildEngine(std::initializer_list<FilterBase *> FilterList);
		bool IsEngineBuilt() const noexcept { return m_IsBuilt; };
		bool CloseEngine();
		bool ResetEngine();
		FilterGraph::IDType RegisterFilter(FilterBase *pFilter);
		template<typename T> T * GetFilter() const { return m_FilterGraph.GetFilter<T>(); }
		template<typename T> T * GetFilterExplicit() const { return m_FilterGraph.GetFilterExplicit<T>(); }

		bool OpenSource(const String &Name);
		bool CloseSource();
		bool IsSourceOpen() const;

		bool SetServiceSelectInfo(const ServiceSelectInfo *pServiceSelInfo);
		bool SetService(const ServiceSelectInfo &ServiceSelInfo, bool Reserve = true);
		uint16_t GetServiceID() const;
		int GetServiceIndex() const;
		bool SetOneSegSelectType(OneSegSelectType Type);

		uint16_t GetTransportStreamID() const;
		uint16_t GetNetworkID() const;
		uint16_t GetEventID() const;

		uint8_t GetVideoStreamType() const;
		int GetVideoStreamCount(int ServiceIndex = -1) const;
		bool SetVideoStream(int StreamIndex);
		int GetVideoStream() const;
		uint8_t GetVideoComponentTag() const;

		uint8_t GetAudioStreamType() const;
		int GetAudioStreamCount(int ServiceIndex = -1) const;
		bool SetAudioStream(int StreamIndex);
		int GetAudioStream() const;
		uint8_t GetAudioComponentTag() const;

		void SetStartStreamingOnSourceOpen(bool Start);

		virtual bool IsSelectableService(int Index) const;
		virtual int GetSelectableServiceCount() const;
		virtual uint16_t GetSelectableServiceID(int Index) const;
		virtual uint16_t GetDefaultServiceID() const;
		virtual int GetSelectableServiceIndexByID(uint16_t ServiceID) const;
		virtual bool GetSelectableServiceList(AnalyzerFilter::ServiceList *pList) const;

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TSEngine"); }
		void SetLogger(Logger *pLogger) override;

	protected:
	// SourceFilter::EventListener
		void OnGraphReset(SourceFilter *pSource) override;
		void OnSourceChanged(SourceFilter *pSource) override;

	// AnalyzerFilter::EventListener
		void OnPATUpdated(AnalyzerFilter *pAnalyzer) override;
		void OnPMTUpdated(AnalyzerFilter *pAnalyzer, uint16_t ServiceID) override;
		void OnEITUpdated(AnalyzerFilter *pAnalyzer) override;

	// TSEngine
		bool SelectService(uint16_t ServiceID, bool No1Seg = false);
		void ResetStatus();
		void SetVideoPID(uint16_t PID, bool ServiceChanged);
		void SetAudioPID(uint16_t PID, bool ServiceChanged);

		virtual void OnEngineClose() {}
		virtual void OnEngineClosed() {}
		virtual void OnFilterRegistered(FilterBase *pFilter, FilterGraph::IDType ID);
		virtual void OnServiceChanged(uint16_t ServiceID) {}
		virtual void OnEventChanged(uint16_t EventID) {}
		virtual void OnVideoStreamTypeChanged(uint8_t StreamType) {}
		virtual void OnAudioStreamTypeChanged(uint8_t StreamType) {}

		bool m_IsBuilt;
		FilterGraph m_FilterGraph;
		mutable MutexLock m_EngineLock;

		SourceFilter *m_pSource;
		AnalyzerFilter *m_pAnalyzer;

		uint16_t m_CurTransportStreamID;
		uint16_t m_CurServiceID;

		ServiceSelectInfo m_ServiceSel;
		ServiceSelectInfo m_SetChannelServiceSel;

		uint8_t m_VideoStreamType;
		uint8_t m_AudioStreamType;
		int m_CurVideoStream;
		uint8_t m_CurVideoComponentTag;
		int m_CurAudioStream;
		uint8_t m_CurAudioComponentTag;
		uint16_t m_CurEventID;

		bool m_StartStreamingOnSourceOpen;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TS_ENGINE_H
