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
 @file   TSSourceFilter.cpp
 @brief  TS ソースフィルタ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "TSSourceFilter.hpp"
#include "TSSourcePin.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


TSSourceFilter::TSSourceFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(TEXT("TS Source Filter"), pUnk, &m_cStateLock, __uuidof(TSSourceFilter))
	, m_pSrcPin(nullptr)
	, m_OutputWhenPaused(false)
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourceFilter::TSSourceFilter %p\n"), this);

	// ピンのインスタンス生成
	m_pSrcPin = new TSSourcePin(phr, this);

	*phr = S_OK;
}


TSSourceFilter::~TSSourceFilter()
{
	delete m_pSrcPin;
}


IBaseFilter * WINAPI TSSourceFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	TSSourceFilter *pNewFilter = new TSSourceFilter(pUnk, phr);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return nullptr;
	}

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_PPV_ARGS(&pFilter));
	if (FAILED(*phr)) {
		delete pNewFilter;
		return nullptr;
	}

	return pFilter;
}


int TSSourceFilter::GetPinCount()
{
	// ピン数を返す
	return 1;
}


CBasePin * TSSourceFilter::GetPin(int n)
{
	// ピンのインスタンスを返す
	return (n == 0) ? m_pSrcPin : nullptr;
}


#ifdef LIBISDB_DEBUG

STDMETHODIMP TSSourceFilter::Run(REFERENCE_TIME tStart)
{
	LIBISDB_TRACE(LIBISDB_STR("■TSSourceFilter::Run()\n"));

	return CBaseFilter::Run(tStart);
}


STDMETHODIMP TSSourceFilter::Pause()
{
	LIBISDB_TRACE(LIBISDB_STR("■TSSourceFilter::Pause()\n"));

	return CBaseFilter::Pause();
}


STDMETHODIMP TSSourceFilter::Stop()
{
	LIBISDB_TRACE(LIBISDB_STR("■TSSourceFilter::Stop()\n"));

	return CBaseFilter::Stop();
}

#endif


STDMETHODIMP TSSourceFilter::GetState(DWORD dw, FILTER_STATE *pState)
{
	*pState = m_State;
	if (m_State == State_Paused && !m_OutputWhenPaused)
		return VFW_S_CANT_CUE;
	return S_OK;
}


bool TSSourceFilter::InputMedia(DataBuffer *pData)
{
	{
		CAutoLock Lock(&m_cStateLock);

		if (!m_pSrcPin
				|| (m_State == State_Stopped)
				|| (m_State == State_Paused && !m_OutputWhenPaused)) {
			return false;
		}
	}

	return m_pSrcPin->InputData(pData);
}


void TSSourceFilter::Reset()
{
	if (m_pSrcPin)
		m_pSrcPin->Reset();
}


void TSSourceFilter::Flush()
{
	CAutoLock Lock(m_pLock);

	if (m_pSrcPin)
		m_pSrcPin->Flush();
}


bool TSSourceFilter::EnableSync(bool bEnable,bool b1Seg)
{
	if (m_pSrcPin)
		return m_pSrcPin->EnableSync(bEnable, b1Seg);
	return false;
}


bool TSSourceFilter::IsSyncEnabled() const
{
	if (m_pSrcPin)
		return m_pSrcPin->IsSyncEnabled();
	return false;
}


void TSSourceFilter::SetVideoPID(uint16_t PID)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetVideoPID(PID);
}


void TSSourceFilter::SetAudioPID(uint16_t PID)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetAudioPID(PID);
}


void TSSourceFilter::SetOutputWhenPaused(bool bOutput)
{
	m_OutputWhenPaused=bOutput;
	if (m_pSrcPin)
		m_pSrcPin->SetOutputWhenPaused(bOutput);
}


bool TSSourceFilter::SetBufferSize(size_t Size)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetBufferSize(Size);
	return false;
}


bool TSSourceFilter::SetInitialPoolPercentage(int Percentage)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetInitialPoolPercentage(Percentage);
	return false;
}


int TSSourceFilter::GetBufferFillPercentage() const
{
	if (m_pSrcPin)
		return m_pSrcPin->GetBufferFillPercentage();
	return 0;
}


bool TSSourceFilter::SetInputWait(DWORD Wait)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetInputWait(Wait);
	return false;
}


bool TSSourceFilter::MapAudioPID(uint16_t AudioPID, uint16_t MapPID)
{
	if (m_pSrcPin)
		return m_pSrcPin->MapAudioPID(AudioPID, MapPID);
	return false;
}


}	// namespace LibISDB::DirectShow
