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
 @file   ViewerEngine.hpp
 @brief  ビューアエンジン
 @author DBCTRADO
*/


#ifndef LIBISDB_VIEWER_ENGINE_H
#define LIBISDB_VIEWER_ENGINE_H


#include "../../Engine/TSEngine.hpp"
#include "ViewerFilter.hpp"
#include <bitset>


namespace LibISDB
{

	/** ビューアエンジンクラス */
	class ViewerEngine
		: public TSEngine
	{
	public:
		ViewerEngine();

		bool BuildViewer(const ViewerFilter::OpenSettings &Settings);
		bool RebuildViewer(const ViewerFilter::OpenSettings &Settings);
		bool CloseViewer();
		bool IsViewerOpen() const;
		bool ResetViewer();
		bool EnableViewer(bool Enable);

		bool SetStreamTypePlayable(uint8_t StreamType, bool Playable);
		bool IsStreamTypePlayable(uint8_t StreamType) const;

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("ViewerEngine"); }

	// TSEngine
		bool IsSelectableService(int Index) const override;
		int GetSelectableServiceCount() const override;
		uint16_t GetSelectableServiceID(int Index) const override;
		uint16_t GetDefaultServiceID() const override;
		int GetSelectableServiceIndexByID(uint16_t ServiceID) const override;
		bool GetSelectableServiceList(AnalyzerFilter::ServiceList *pList) const override;

	protected:
		void OnFilterRegistered(FilterBase *pFilter, FilterGraph::IDType ID) override;
		void OnServiceChanged(uint16_t ServiceID) override;
		void OnVideoStreamTypeChanged(uint8_t StreamType) override;
		void OnAudioStreamTypeChanged(uint8_t StreamType) override;

		void UpdateVideoAndAudioPID();

		ViewerFilter *m_pViewer;

		std::bitset<0x88> m_PlayableStreamType;
		bool m_PlayRadio;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_VIEWER_ENGINE_H
