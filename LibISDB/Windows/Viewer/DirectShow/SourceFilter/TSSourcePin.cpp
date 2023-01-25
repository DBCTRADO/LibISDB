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
 @file   TSSourcePin.cpp
 @brief  TS ソースピン
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "TSSourcePin.hpp"
#include "TSSourceFilter.hpp"
#include <process.h>
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


static constexpr size_t SAMPLE_PACKETS = 1024;
static constexpr size_t SAMPLE_BUFFER_SIZE = TS_PACKET_SIZE * SAMPLE_PACKETS;




TSSourcePin::TSSourcePin(HRESULT *phr, TSSourceFilter *pFilter)
	: CBaseOutputPin(TEXT("TSSourcePin"), pFilter, pFilter->m_pLock, phr, L"TS")
	, m_pFilter(pFilter)
	, m_InitialPoolPercentage(0)
	, m_Buffering(false)
	, m_OutputWhenPaused(false)
	, m_InputWait(0)
	, m_InputTimeout(false)
	, m_NewSegment(true)
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourcePin::TSSourcePin() {}\n"), static_cast<void *>(this));

	*phr = S_OK;
}


TSSourcePin::~TSSourcePin()
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourcePin::~TSSourcePin()\n"));

	StopStreamingThread();
}


HRESULT TSSourcePin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Stream);
	pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_TRANSPORT);
	pMediaType->SetTemporalCompression(FALSE);
	pMediaType->SetSampleSize(TS_PACKET_SIZE);

	return S_OK;
}


HRESULT TSSourcePin::CheckMediaType(const CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	CMediaType mt;
	GetMediaType(0, &mt);

	return (*pMediaType == mt) ? S_OK : E_FAIL;
}


HRESULT TSSourcePin::Active()
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourcePin::Active()\n"));

	HRESULT hr = CBaseOutputPin::Active();
	if (FAILED(hr))
		return hr;

	if (IsStarted())
		return E_UNEXPECTED;

	if (!m_SrcStream.Initialize())
		return E_OUTOFMEMORY;

	m_Buffering = m_InitialPoolPercentage > 0;
	m_InputTimeout = false;

	if (!StartStreamingThread())
		return E_FAIL;

	return S_OK;
}


HRESULT TSSourcePin::Inactive()
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourcePin::Inactive()\n"));

	HRESULT hr = CBaseOutputPin::Inactive();

	StopStreamingThread();

	return hr;
}


HRESULT TSSourcePin::Run(REFERENCE_TIME tStart)
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourcePin::Run()\n"));

	return CBaseOutputPin::Run(tStart);
}


HRESULT TSSourcePin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pRequest, E_POINTER);

	if (pRequest->cBuffers < 1)
		pRequest->cBuffers = 1;

	if (pRequest->cbBuffer < SAMPLE_BUFFER_SIZE)
		pRequest->cbBuffer = SAMPLE_BUFFER_SIZE;

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
	if (FAILED(hr))
		return hr;

	if ((Actual.cBuffers < pRequest->cBuffers)
			|| (Actual.cbBuffer < pRequest->cbBuffer))
		return E_FAIL;

	return S_OK;
}


bool TSSourcePin::InputData(DataBuffer *pData)
{
	const DWORD Wait = m_InputWait;

	if ((Wait != 0) && m_SrcStream.IsBufferFull()) {
		if (m_InputTimeout)
			return false;

		// サンプルが出力されるのを待つ
		const DWORD BeginTime = ::GetTickCount();
		for (;;) {
			::Sleep(10);
			if (!m_SrcStream.IsBufferFull())
				break;
			if (static_cast<DWORD>(::GetTickCount() - BeginTime) >= Wait) {
				LIBISDB_TRACE(LIBISDB_STR("TSSourcePin::InputMedia() : Timeout {} ms\n"), Wait);
				m_InputTimeout = true;
				return false;
			}
		}
	}

	m_SrcStream.InputData(pData);

	m_InputTimeout = false;

	return true;
}


void TSSourcePin::Reset()
{
	m_SrcStream.Reset();
	m_NewSegment.store(true, std::memory_order_release);
}


void TSSourcePin::Flush()
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourcePin::Flush()\n"));

	Reset();
	DeliverBeginFlush();
	DeliverEndFlush();
}


bool TSSourcePin::EnableSync(bool bEnable, bool b1Seg)
{
	return m_SrcStream.EnableSync(bEnable, b1Seg);
}


bool TSSourcePin::IsSyncEnabled() const
{
	return m_SrcStream.IsSyncEnabled();
}


void TSSourcePin::SetVideoPID(uint16_t PID)
{
	m_SrcStream.SetVideoPID(PID);
}


void TSSourcePin::SetAudioPID(uint16_t PID)
{
	m_SrcStream.SetAudioPID(PID);
}


bool TSSourcePin::SetBufferSize(size_t Size)
{
	return m_SrcStream.SetQueueSize((Size != 0) ? Size : TSSourceStream::DEFAULT_QUEUE_SIZE);
}


bool TSSourcePin::SetInitialPoolPercentage(int Percentage)
{
	if ((Percentage < 0) || (Percentage > 100))
		return false;

	m_InitialPoolPercentage = Percentage;

	return true;
}


int TSSourcePin::GetBufferFillPercentage()
{
	return m_SrcStream.GetFillPercentage();
}


bool TSSourcePin::SetInputWait(DWORD Wait)
{
	m_InputWait = Wait;
	return true;
}


bool TSSourcePin::MapAudioPID(uint16_t AudioPID, uint16_t MapPID)
{
	m_SrcStream.MapAudioPID(AudioPID, MapPID);
	return true;
}


void TSSourcePin::StreamingLoop()
{
	::CoInitialize(nullptr);

	m_NewSegment.store(true, std::memory_order_release);

	StreamingThread::StreamingLoop();

	DeliverEndOfStream();

	::CoUninitialize();
}


bool TSSourcePin::ProcessStream()
{
	if (m_Buffering) {
		const int PoolPercentage = m_InitialPoolPercentage;

		if ((m_SrcStream.GetFillPercentage() < PoolPercentage)
				/*
					バッファ使用割合のみでは、ワンセグ等ビットレートが低い場合になかなか再生が開始されないので、
					ビットレートを 2MB/s として経過時間でも判定する。
					これも設定できるようにするか、あるいは時間での判定に統一する方がいいかも知れない。
				*/
				&& (m_SrcStream.GetPTSDuration() <
						static_cast<LONGLONG>(m_SrcStream.GetQueueSize() * TS_PACKET_SIZE) * PoolPercentage / (2000000LL * 100LL / 90000LL))) {
			return false;
		}

		m_Buffering = false;
	}

	if (m_SrcStream.IsDataAvailable()) {
		bool Discontinuity = false;

		if (m_NewSegment.exchange(false, std::memory_order_acq_rel)) {
			DeliverNewSegment(0, LONGLONG_MAX, 1.0);
			Discontinuity = true;
		}

		IMediaSample *pSample = nullptr;
		HRESULT hr = GetDeliveryBuffer(&pSample, nullptr, nullptr, 0);
		if (SUCCEEDED(hr)) {
			BYTE *pSampleData = nullptr;
			hr = pSample->GetPointer(&pSampleData);
			if (SUCCEEDED(hr)) {
				size_t Size = m_SrcStream.GetData(pSampleData, SAMPLE_PACKETS);
				if (Size != 0) {
					pSample->SetActualDataLength(static_cast<long>(Size * TS_PACKET_SIZE));
					pSample->SetDiscontinuity(Discontinuity);
					Deliver(pSample);
				}
				pSample->Release();
			}
		}

		return true;
	}

	return false;
}


}	// namespace LibISDB::DirectShow
