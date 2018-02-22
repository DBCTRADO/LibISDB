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
 @file   H264ParserFilter.cpp
 @brief  H.264 解析フィルタ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "H264ParserFilter.hpp"
#include "../DirectShowUtilities.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


namespace
{

/*
	タイムスタンプ調整の処理内容はワンセグ仕様前提なので、
	それ以外の場合は問題が出ると思います。
*/

// REFERENCE_TIMEの一秒
constexpr REFERENCE_TIME REFERENCE_TIME_SECOND = 10000000LL;

// フレームレート
constexpr int FRAME_RATE_NUM      = 30000;
constexpr int FRAME_RATE_FACTOR   = 1001;
constexpr int FRAME_RATE_1SEG_NUM = 15000;

// バッファサイズ
constexpr long SAMPLE_BUFFER_SIZE = 0x800000L;	// 8MiB

constexpr DWORD INITIAL_BITRATE = 32000000;
constexpr int INITIAL_WIDTH  = 1920;
constexpr int INITIAL_HEIGHT = 1080;

constexpr REFERENCE_TIME MAX_SAMPLE_TIME_DIFF   = REFERENCE_TIME_SECOND * 3LL;
constexpr REFERENCE_TIME MAX_SAMPLE_TIME_JITTER = REFERENCE_TIME_SECOND / 4LL;

// フレームの表示時間を算出
constexpr REFERENCE_TIME CalcFrameTime(LONGLONG Frames, bool OneSeg = false) {
	return Frames * REFERENCE_TIME_SECOND * FRAME_RATE_FACTOR / (OneSeg ? FRAME_RATE_1SEG_NUM : FRAME_RATE_NUM);
}

}	// namespace


H264ParserFilter::H264ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(TEXT("H264 Parser Filter"), pUnk, __uuidof(H264ParserFilter))
	, m_H264Parser(this)
	, m_AdjustTime(false)
	, m_AdjustFrameRate(false)
	, m_Adjust1Seg(false)
{
	LIBISDB_TRACE(LIBISDB_STR("H264ParserFilter::H264ParserFilter() %p\n"),this);

	m_MediaType.InitMediaType();
	m_MediaType.SetType(&MEDIATYPE_Video);
	m_MediaType.SetSubtype(&MEDIASUBTYPE_H264);
	m_MediaType.SetTemporalCompression(TRUE);
	m_MediaType.SetSampleSize(0);
	m_MediaType.SetFormatType(&FORMAT_VideoInfo);
	VIDEOINFOHEADER *pvih = reinterpret_cast<VIDEOINFOHEADER *>(
		m_MediaType.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	if (!pvih) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	::ZeroMemory(pvih, sizeof(VIDEOINFOHEADER));
	pvih->dwBitRate = INITIAL_BITRATE;
	pvih->AvgTimePerFrame = CalcFrameTime(1);
	pvih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvih->bmiHeader.biWidth = INITIAL_WIDTH;
	pvih->bmiHeader.biHeight = INITIAL_HEIGHT;
	pvih->bmiHeader.biCompression = MAKEFOURCC('h','2','6','4');

	*phr = S_OK;
}


H264ParserFilter::~H264ParserFilter(void)
{
	LIBISDB_TRACE(LIBISDB_STR("H264ParserFilter::~H264ParserFilter()\n"));

	ClearSampleDataQueue(&m_SampleQueue);
}


IBaseFilter* WINAPI H264ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	H264ParserFilter *pNewFilter = new H264ParserFilter(pUnk, phr);
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


HRESULT H264ParserFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video)
		return S_OK;

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT H264ParserFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Video
			&& (*mtOut->Subtype() == MEDIASUBTYPE_H264
				|| *mtOut->Subtype() == MEDIASUBTYPE_h264
				|| *mtOut->Subtype() == MEDIASUBTYPE_H264_bis
				|| *mtOut->Subtype() == MEDIASUBTYPE_AVC1
				|| *mtOut->Subtype() == MEDIASUBTYPE_avc1)) {
		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT H264ParserFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	// バッファは1個あればよい
	if (pprop->cBuffers < 1)
		pprop->cBuffers = 1;

	if (pprop->cbBuffer < SAMPLE_BUFFER_SIZE)
		pprop->cbBuffer = SAMPLE_BUFFER_SIZE;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if (FAILED(hr))
		return hr;

	// 要求を受け入れられたか判定
	if (Actual.cBuffers < pprop->cBuffers
			|| Actual.cbBuffer < pprop->cbBuffer)
		return E_FAIL;

	return S_OK;
}


HRESULT H264ParserFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(m_pLock);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}


HRESULT H264ParserFilter::StartStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	Reset();
	m_VideoInfo.Reset();

	return S_OK;
}


HRESULT H264ParserFilter::StopStreaming()
{
	return S_OK;
}


HRESULT H264ParserFilter::BeginFlush()
{
	HRESULT hr = CTransformFilter::BeginFlush();

	CAutoLock Lock(&m_ParserLock);

	Reset();
	m_VideoInfo.Reset();

	return hr;
}


HRESULT H264ParserFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	// 入力データポインタを取得する
	BYTE *pInData = nullptr;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;
	LONG InDataSize = pIn->GetActualDataLength();

	// 出力データポインタを取得する
	BYTE *pOutData = nullptr;
	hr = pOut->GetPointer(&pOutData);
	if (FAILED(hr))
		return hr;
	pOut->SetActualDataLength(0);

	m_ChangeSize = false;

	{
		CAutoLock Lock(&m_ParserLock);

		if (m_AdjustTime) {
			// タイムスタンプ取得
			REFERENCE_TIME StartTime, EndTime;
			hr = pIn->GetTime(&StartTime, &EndTime);
			if (hr == S_OK || hr == VFW_S_NO_STOP_TIME) {
				if (m_AdjustFrameRate) {
					if (m_PrevTime >= 0
							&& (m_PrevTime >= StartTime
								|| m_PrevTime + MAX_SAMPLE_TIME_DIFF < StartTime)) {
						LIBISDB_TRACE(LIBISDB_STR("Reset H.264 media queue\n"));
						while (!m_SampleQueue.empty()) {
							m_OutSampleQueue.push_back(m_SampleQueue.front());
							m_SampleQueue.pop_front();
						}
					} else if (!m_SampleQueue.empty()) {
						const REFERENCE_TIME Duration = StartTime - m_PrevTime;
						const size_t Frames = m_SampleQueue.size();
						REFERENCE_TIME Start, End;

						Start = m_PrevTime;
						for (size_t i = 1; i <= Frames; i++) {
							SampleData *pSampleData = m_SampleQueue.front();
							m_SampleQueue.pop_front();
							End = m_PrevTime + Duration * (REFERENCE_TIME)i / (REFERENCE_TIME)Frames;
							pSampleData->SetTime(Start, End);
							Start = End;
							m_OutSampleQueue.push_back(pSampleData);
						}
					}
					m_PrevTime = StartTime;
				} else {
					bool bReset = false;
					if (m_PrevTime < 0) {
						bReset = true;
					} else {
						LONGLONG Diff = (m_PrevTime + CalcFrameTime(m_SampleCount, m_Adjust1Seg)) - StartTime;
						if (_abs64(Diff) > MAX_SAMPLE_TIME_JITTER) {
							bReset = true;
							LIBISDB_TRACE(LIBISDB_STR("Reset H.264 sample time (Diff = %.5f)\n"),
								  (double)Diff / (double)REFERENCE_TIME_SECOND);
						}
					}
					if (bReset) {
						m_PrevTime = StartTime;
						m_SampleCount = 0;
					}
				}
			}

			hr = S_OK;
		} else {
			hr = pOut->SetActualDataLength(InDataSize);
			if (SUCCEEDED(hr))
				::CopyMemory(pOutData, pInData, InDataSize);
		}

		m_H264Parser.StoreES(pInData, InDataSize);

		if (m_pStreamCallback)
			m_pStreamCallback->OnStream(MAKEFOURCC('H','2','6','4'), pInData, InDataSize);
	}

	if (!m_OutSampleQueue.empty()) {
		do {
			SampleData *pSampleData = m_OutSampleQueue.front();

			hr = pOut->SetActualDataLength((long)pSampleData->GetSize());
			if (SUCCEEDED(hr)) {
				::CopyMemory(pOutData, pSampleData->GetData(), pSampleData->GetSize());

				if (pSampleData->HasTimestamp())
					pOut->SetTime(&pSampleData->m_StartTime, &pSampleData->m_EndTime);
				else
					pOut->SetTime(nullptr, nullptr);

				if (pSampleData->m_ChangeSize)
					AttachMediaType(pOut, pSampleData->m_Width, pSampleData->m_Height);
				else
					pOut->SetMediaType(nullptr);

				hr = m_pOutput->Deliver(pOut);
			}
			m_OutSampleQueue.pop_front();
			m_SampleDataPool.Restore(pSampleData);
			if (FAILED(hr))
				break;
		} while (!m_OutSampleQueue.empty());

		pOut->SetActualDataLength(0);

		ClearSampleDataQueue(&m_OutSampleQueue);
	}

	if (SUCCEEDED(hr)) {
		if (pOut->GetActualDataLength() > 0) {
			if (m_ChangeSize)
				AttachMediaType(pOut, m_VideoInfo.OriginalWidth, m_VideoInfo.OriginalHeight);
			hr = S_OK;
		} else {
			hr = S_FALSE;
		}
	}

	return hr;
}


HRESULT H264ParserFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	IMediaSample *pOutSample;
	HRESULT hr;

	hr = InitializeOutputSample(pSample, &pOutSample);
	if (FAILED(hr))
		return hr;
	hr = Transform(pSample, pOutSample);
	if (SUCCEEDED(hr)) {
		if (hr == S_OK)
			hr = m_pOutput->Deliver(pOutSample);
		else if (hr == S_FALSE)
			hr = S_OK;
		m_bSampleSkipped = FALSE;
	}

	pOutSample->Release();

	return hr;
}


bool H264ParserFilter::SetAdjustSampleOptions(AdjustSampleFlag Flags)
{
	CAutoLock Lock(&m_ParserLock);

	const bool bAdjustTime = !!(Flags & AdjustSampleFlag::Time);
	const bool bAdjustFrameRate = !!(Flags & AdjustSampleFlag::FrameRate);
	const bool bAdjust1Seg = !!(Flags & AdjustSampleFlag::OneSeg);
	const bool bReset = (m_AdjustTime != bAdjustTime)
		|| (bAdjustTime && (m_AdjustFrameRate != bAdjustFrameRate || m_Adjust1Seg != bAdjust1Seg));

	m_AdjustTime = bAdjustTime;
	m_AdjustFrameRate = bAdjustFrameRate;
	m_Adjust1Seg = bAdjust1Seg;

	if (bReset)
		Reset();

	return true;
}


void H264ParserFilter::Reset()
{
	m_H264Parser.Reset();
	m_PrevTime = -1;
	m_SampleCount = 0;
	ClearSampleDataQueue(&m_SampleQueue);
}


void H264ParserFilter::ClearSampleDataQueue(SampleDataQueue *pQueue)
{
	while (!pQueue->empty()) {
		m_SampleDataPool.Restore(pQueue->front());
		pQueue->pop_front();
	}
}


HRESULT H264ParserFilter::AttachMediaType(IMediaSample *pSample, int Width, int Height)
{
	CMediaType MediaType(m_pOutput->CurrentMediaType());
	VIDEOINFOHEADER *pVideoInfo = reinterpret_cast<VIDEOINFOHEADER *>(MediaType.Format());
	HRESULT hr;

	if (pVideoInfo
			&& (pVideoInfo->bmiHeader.biWidth != Width ||
				pVideoInfo->bmiHeader.biHeight != Height)) {
		pVideoInfo->bmiHeader.biWidth = Width;
		pVideoInfo->bmiHeader.biHeight = Height;
		hr = pSample->SetMediaType(&MediaType);
	} else {
		hr = S_FALSE;
	}

	return hr;
}


void H264ParserFilter::OnAccessUnit(const H264Parser *pParser, const H264AccessUnit *pAccessUnit)
{
	int OrigWidth, OrigHeight;
	OrigWidth = pAccessUnit->GetHorizontalSize();
	OrigHeight = pAccessUnit->GetVerticalSize();
	bool bChangeSize = false;

	if (m_AttachMediaType
			&& (m_VideoInfo.OriginalWidth != OrigWidth ||
				m_VideoInfo.OriginalHeight != OrigHeight)) {
		bChangeSize = true;
		m_ChangeSize = true;
	}

	if (m_AdjustTime) {
		// ワンセグは1フレーム単位でタイムスタンプを設定しないとかくつく
		SampleData *pSampleData = m_SampleDataPool.Get(*pAccessUnit);

		if (pSampleData != nullptr) {
			if (bChangeSize)
				pSampleData->ChangeSize(OrigWidth, OrigHeight);

			if (m_AdjustFrameRate && m_PrevTime >= 0) {
				m_SampleQueue.push_back(pSampleData);
			} else {
				if (m_PrevTime >= 0) {
					REFERENCE_TIME StartTime = m_PrevTime + CalcFrameTime(m_SampleCount, m_Adjust1Seg);
					REFERENCE_TIME EndTime = m_PrevTime + CalcFrameTime(m_SampleCount + 1, m_Adjust1Seg);
					pSampleData->SetTime(StartTime, EndTime);
				}
				m_OutSampleQueue.push_back(pSampleData);
			}
		}
		m_SampleCount++;
	}

	int DisplayWidth, DisplayHeight;
	int AspectX = 0, AspectY = 0;

	DisplayWidth = OrigWidth;
	DisplayHeight = OrigHeight;

	uint16_t SARX = 0, SARY = 0;
	if (pAccessUnit->GetSAR(&SARX, &SARY) && SARX != 0 && SARY != 0) {
		SARToDAR(SARX, SARY, OrigWidth, OrigHeight, &AspectX, &AspectY);
	} else {
		SARToDAR(1, 1, OrigWidth, OrigHeight, &AspectX, &AspectY);
	}

	VideoInfo Info(OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, AspectX, AspectY);

	H264AccessUnit::TimingInfo TimingInfo;
	if (pAccessUnit->GetTimingInfo(&TimingInfo)) {
		// 実際のフレームレートではない
		Info.FrameRate.Num = TimingInfo.TimeScale;
		Info.FrameRate.Denom = TimingInfo.NumUnitsInTick;
	}

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		m_VideoInfo = Info;

		LIBISDB_TRACE(
			LIBISDB_STR("H.264 access unit %d x %d [SAR %d:%d (DAR %d:%d)] %d/%d\n"),
			OrigWidth, OrigHeight, SARX, SARY, AspectX, AspectY,
			Info.FrameRate.Num, Info.FrameRate.Denom);

		// 通知
		NotifyVideoInfo();
	}
}




H264ParserFilter::SampleData::SampleData(const DataBuffer &Data)
	: DataBuffer(Data)
{
	ResetProperties();
}


void H264ParserFilter::SampleData::ResetProperties()
{
	m_StartTime = -1;
	m_EndTime = -1;
	m_ChangeSize = false;
}


void H264ParserFilter::SampleData::SetTime(REFERENCE_TIME StartTime, REFERENCE_TIME EndTime)
{
	m_StartTime = StartTime;
	m_EndTime = EndTime;
}


void H264ParserFilter::SampleData::ChangeSize(int Width, int Height)
{
	m_ChangeSize = true;
	m_Width = Width;
	m_Height = Height;
}




H264ParserFilter::SampleDataPool::SampleDataPool()
	: m_MaxData(256)
	, m_DataCount(0)
{
}


H264ParserFilter::SampleDataPool::~SampleDataPool()
{
	LIBISDB_TRACE(
		LIBISDB_STR("H264ParserFilter::SampleDataPool::~SampleDataPool() Data count %zu / %zu\n"),
		m_DataCount, m_MaxData);

	Clear();
}


void H264ParserFilter::SampleDataPool::Clear()
{
	CAutoLock Lock(&m_Lock);

	while (!m_Queue.empty()) {
		delete m_Queue.back();
		m_Queue.pop_back();
	}

	m_DataCount = 0;
}


H264ParserFilter::SampleData * H264ParserFilter::SampleDataPool::Get(const DataBuffer &Data)
{
	CAutoLock Lock(&m_Lock);

	if (m_Queue.empty()) {
		if (m_DataCount >= m_MaxData)
			return nullptr;
		m_DataCount++;
		return new SampleData(Data);
	}

	SampleData *pData = m_Queue.back();
	pData->ResetProperties();
	const size_t Size = Data.GetSize();
	if (pData->SetData(Data.GetData(), Size) < Size)
		return nullptr;
	m_Queue.pop_back();
	return pData;
}


void H264ParserFilter::SampleDataPool::Restore(SampleData *pData)
{
	CAutoLock Lock(&m_Lock);

	m_Queue.push_back(pData);
}


}	// namespace LibISDB::DirectShow
