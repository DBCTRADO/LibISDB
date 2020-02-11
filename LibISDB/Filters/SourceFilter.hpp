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
 @file   SourceFilter.hpp
 @brief  ソースフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_SOURCE_FILTER_H
#define LIBISDB_SOURCE_FILTER_H


#include "FilterBase.hpp"
#include "../Base/EventListener.hpp"


namespace LibISDB
{

	/** ソースフィルタ基底クラス */
	class SourceFilter
		: public SingleOutputFilter
	{
	public:
		enum class SourceMode : unsigned int {
			Push = 0x0001U,
			Pull = 0x0002U,
		};

		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnGraphReset(SourceFilter *pSource) {}
			virtual void OnSourceOpened(SourceFilter *pSrouce) {}
			virtual void OnSourceClosed(SourceFilter *pSrouce) {}
			virtual void OnSourceChanged(SourceFilter *pSrouce) {}
			virtual void OnSourceChangeFailed(SourceFilter *pSrouce) {}
			virtual void OnSourceEnd(SourceFilter *pSrouce) {}
			virtual void OnStreamingStart(SourceFilter *pSrouce) {}
			virtual void OnStreamingStop(SourceFilter *pSrouce) {}
		};

	// FilterBase
		void Finalize() override;

	// SourceFilter
		virtual bool OpenSource(const CStringView &Name) = 0;
		virtual bool CloseSource() = 0;
		virtual bool IsSourceOpen() const = 0;

		virtual bool FetchSource(size_t RequestSize) { return false; }

		virtual SourceMode GetAvailableSourceModes() const noexcept = 0;
		virtual bool SetSourceMode(SourceMode Mode);
		SourceMode GetSourceMode() const noexcept { return m_SourceMode; }

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

	protected:
		SourceFilter(SourceMode Mode);

		SourceMode m_SourceMode;
		EventListenerList<EventListener> m_EventListenerList;
	};

	LIBISDB_ENUM_FLAGS(SourceFilter::SourceMode)

}	// namespace LibISDB


#endif	// ifndef LIBISDB_SOURCE_FILTER_H
