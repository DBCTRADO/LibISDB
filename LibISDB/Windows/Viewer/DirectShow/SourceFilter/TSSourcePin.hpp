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
 @file   TSSourcePin.hpp
 @brief  TS ソースピン
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_SOURCE_PIN_H
#define LIBISDB_TS_SOURCE_PIN_H


#include "TSSourceStream.hpp"
#include "../DirectShowBase.hpp"
#include "../../../../Base/StreamingThread.hpp"
#include <atomic>


namespace LibISDB::DirectShow
{

	class TSSourceFilter;

	/** TS ソースピンクラス */
	class TSSourcePin
		: public CBaseOutputPin
		, protected StreamingThread
	{
	public:
		TSSourcePin(HRESULT *phr, TSSourceFilter *pFilter);
		~TSSourcePin();

	// CBasePin
		HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
		HRESULT CheckMediaType(const CMediaType *pMediaType) override;

		HRESULT Active() override;
		HRESULT Inactive() override;
		HRESULT Run(REFERENCE_TIME tStart) override;

	// CBaseOutputPin
		HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest) override;

	// TSSourcePin
		bool InputData(DataBuffer *pData);
		void Reset();
		void Flush();
		bool EnableSync(bool bEnable, bool b1Seg = false);
		bool IsSyncEnabled() const;
		void SetVideoPID(uint16_t PID);
		void SetAudioPID(uint16_t PID);
		void SetOutputWhenPaused(bool bOutput) { m_OutputWhenPaused = bOutput; }
		bool SetBufferSize(size_t Size);
		bool SetInitialPoolPercentage(int Percentage);
		int GetBufferFillPercentage();
		bool SetInputWait(DWORD Wait);
		bool MapAudioPID(uint16_t AudioPID, uint16_t MapPID);

	protected:
	// Thread
		const CharType * GetThreadName() const noexcept override { return LIBISDB_STR("TSSourcePin"); }

	// StreamingThread
		void StreamingLoop() override;
		bool ProcessStream() override;

		TSSourceFilter *m_pFilter;

		TSSourceStream m_SrcStream;

		int m_InitialPoolPercentage;
		bool m_Buffering;
		bool m_OutputWhenPaused;
		DWORD m_InputWait;
		bool m_InputTimeout;
		std::atomic<bool> m_NewSegment;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_TS_SOURCE_PIN_H
