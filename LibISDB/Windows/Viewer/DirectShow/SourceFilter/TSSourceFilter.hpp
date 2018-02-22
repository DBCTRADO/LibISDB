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
 @file   TSSourceFilter.hpp
 @brief  TS ソースフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_SOURCE_FILTER_H
#define LIBISDB_TS_SOURCE_FILTER_H


#include "TSSourceStream.hpp"
#include "../DirectShowBase.hpp"


namespace LibISDB::DirectShow
{

	class TSSourcePin;

	/** TS ソースフィルタクラス */
	class __declspec(uuid("DCA86296-964A-4e64-857D-8D140E630707")) TSSourceFilter
		: public CBaseFilter
	{
		friend TSSourcePin;

	public:
		static IBaseFilter * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

		DECLARE_IUNKNOWN

	// CBaseFilter
#ifdef LIBISDB_DEBUG
		STDMETHODIMP Run(REFERENCE_TIME tStart) override;
		STDMETHODIMP Pause() override;
		STDMETHODIMP Stop() override;
#endif
		STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State) override;
		int GetPinCount() override;
		CBasePin *GetPin(int n) override;

	// TSSourceFilter
		bool InputMedia(DataBuffer *pData);
		void Reset();
		void Flush();
		bool EnableSync(bool bEnable, bool b1Seg=false);
		bool IsSyncEnabled() const;
		void SetVideoPID(uint16_t PID);
		void SetAudioPID(uint16_t PID);
		void SetOutputWhenPaused(bool bOutput);
		bool SetBufferSize(size_t Size);
		bool SetInitialPoolPercentage(int Percentage);
		int GetBufferFillPercentage() const;
		bool SetInputWait(DWORD Wait);
		bool MapAudioPID(uint16_t AudioPID, uint16_t MapPID);

	protected:
		TSSourceFilter(LPUNKNOWN pUnk, HRESULT *phr);
		virtual ~TSSourceFilter();

		TSSourcePin *m_pSrcPin;
		CCritSec m_cStateLock;
		bool m_OutputWhenPaused;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_TS_SOURCE_FILTER_H
