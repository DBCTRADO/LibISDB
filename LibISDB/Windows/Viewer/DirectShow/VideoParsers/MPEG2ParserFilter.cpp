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
 @file   MPEG2ParserFilter.cpp
 @brief  MPEG-2 解析フィルタ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "MPEG2ParserFilter.hpp"
#include <dvdmedia.h>
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


constexpr long SAMPLE_BUFFER_SIZE = 0x800000L;	// 8MiB


MPEG2ParserFilter::MPEG2ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
#ifndef MPEG2PARSERFILTER_INPLACE
	: CTransformFilter(TEXT("MPEG2 Parser Filter"), pUnk, __uuidof(MPEG2ParserFilter))
#else
	: CTransInPlaceFilter(TEXT("MPEG2 Parser Filter"), pUnk, __uuidof(MPEG2ParserFilter), phr, FALSE)
#endif
	, m_MPEG2Parser(this)
	, m_pOutSample(nullptr)
{
	LIBISDB_TRACE(LIBISDB_STR("MPEG2ParserFilter::MPEG2ParserFilter() %p\n"), this);

	*phr = S_OK;
}


MPEG2ParserFilter::~MPEG2ParserFilter()
{
	LIBISDB_TRACE(LIBISDB_STR("MPEG2ParserFilter::~MPEG2ParserFilter()\n"));
}


IBaseFilter * WINAPI MPEG2ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	MPEG2ParserFilter *pNewFilter = new MPEG2ParserFilter(pUnk, phr);
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


HRESULT MPEG2ParserFilter::CheckInputType(const CMediaType *mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video) {
		if (*mtIn->Subtype() == MEDIASUBTYPE_MPEG2_VIDEO) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


#ifndef MPEG2PARSERFILTER_INPLACE


HRESULT MPEG2ParserFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Video) {
		if (*mtOut->Subtype() == MEDIASUBTYPE_MPEG2_VIDEO) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT MPEG2ParserFilter::DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop)
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


HRESULT MPEG2ParserFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(m_pLock);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_pInput->CurrentMediaType();

	return S_OK;
}


HRESULT MPEG2ParserFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	BYTE *pInData = nullptr;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;
	LONG InDataSize = pIn->GetActualDataLength();

	pOut->SetActualDataLength(0);
	m_pOutSample = pOut;

	CAutoLock Lock(&m_ParserLock);

	// シーケンスを取得
	m_MPEG2Parser.StoreEs(pInData, InDataSize);

	if (m_pStreamCallback)
		m_pStreamCallback->OnStream(MAKEFOURCC('m','p','2','v'), pInData, InDataSize);

	return pOut->GetActualDataLength() > 0 ? S_OK : S_FALSE;
}


#else	// ndef MPEG2PARSERFILTER_INPLACE


HRESULT MPEG2ParserFilter::Transform(IMediaSample *pSample)
{
	BYTE *pData = nullptr;
	HRESULT hr = pSample->GetPointer(&pData);
	if (FAILED(hr))
		return hr;
	LONG DataSize = pSample->GetActualDataLength();
	m_pOutSample = pSample;

	CAutoLock Lock(&m_ParserLock);

	// シーケンスを取得
	m_MPEG2Parser.StoreES(pData, DataSize);

	if (m_pStreamCallback)
		m_pStreamCallback->OnStream(MAKEFOURCC('m','p','2','v'), pData, DataSize);

	return S_OK;
}


HRESULT MPEG2ParserFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	if (UsingDifferentAllocators()) {
		pSample = Copy(pSample);
		if (pSample == nullptr)
			return E_UNEXPECTED;
	}

	HRESULT hr = Transform(pSample);
	if (SUCCEEDED(hr)) {
		if (hr == S_OK)
			hr = m_pOutput->Deliver(pSample);
		else if (hr == S_FALSE)
			hr = S_OK;
	}

	if (UsingDifferentAllocators())
		pSample->Release();

	return hr;
}


#endif	// MPEG2PARSERFILTER_INPLACE


HRESULT MPEG2ParserFilter::StartStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	m_MPEG2Parser.Reset();
	m_VideoInfo.Reset();

	return S_OK;
}


HRESULT MPEG2ParserFilter::StopStreaming()
{
	return S_OK;
}


HRESULT MPEG2ParserFilter::BeginFlush()
{
	HRESULT hr = __super::BeginFlush();

	CAutoLock Lock(&m_ParserLock);

	m_MPEG2Parser.Reset();
	m_VideoInfo.Reset();

	return hr;
}


void MPEG2ParserFilter::OnMPEG2Sequence(const MPEG2VideoParser *pParser, const MPEG2Sequence *pSequence)
{
#ifndef MPEG2PARSERFILTER_INPLACE
	LONG Offset = m_pOutSample->GetActualDataLength();
	BYTE *pOutData = nullptr;
	if (SUCCEEDED(m_pOutSample->GetPointer(&pOutData))
			&& SUCCEEDED(m_pOutSample->SetActualDataLength(Offset + pSequence->GetSize()))) {
		::CopyMemory(&pOutData[Offset], pSequence->GetData(), pSequence->GetSize());
	}
#endif

	int OrigWidth, OrigHeight;
	int DisplayWidth, DisplayHeight;
	uint8_t AspectX, AspectY;

	OrigWidth = pSequence->GetHorizontalSize();
	OrigHeight = pSequence->GetVerticalSize();

	if (pSequence->HasExtendDisplayInfo()) {
		DisplayWidth = pSequence->GetExtendDisplayHorizontalSize();
		DisplayHeight = pSequence->GetExtendDisplayVerticalSize();
	} else {
		DisplayWidth = OrigWidth;
		DisplayHeight = OrigHeight;
	}

	if (!pSequence->GetAspectRatio(&AspectX, &AspectY))
		AspectX = AspectY = 0;

	VideoParser::VideoInfo Info(OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, AspectX, AspectY);

	uint32_t FrameRateNum, FrameRateDenom;
	if (pSequence->GetFrameRate(&FrameRateNum, &FrameRateDenom)) {
		Info.FrameRate.Num = FrameRateNum;
		Info.FrameRate.Denom = FrameRateDenom;
	}

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		if (m_AttachMediaType
				&& (m_VideoInfo.OriginalWidth != OrigWidth ||
					m_VideoInfo.OriginalHeight != OrigHeight)) {
			CMediaType MediaType(m_pOutput->CurrentMediaType());
			MPEG2VIDEOINFO *pVideoInfo = reinterpret_cast<MPEG2VIDEOINFO *>(MediaType.Format());

			if (pVideoInfo
					&& (pVideoInfo->hdr.bmiHeader.biWidth != OrigWidth ||
						pVideoInfo->hdr.bmiHeader.biHeight != OrigHeight)) {
				pVideoInfo->hdr.bmiHeader.biWidth = OrigWidth;
				pVideoInfo->hdr.bmiHeader.biHeight = OrigHeight;
				m_pOutSample->SetMediaType(&MediaType);
			}
		}

		m_VideoInfo = Info;

		LIBISDB_TRACE(
			LIBISDB_STR("MPEG2 sequence %d x %d [%d x %d (%d=%d:%d)]\n"),
			OrigWidth, OrigHeight, DisplayWidth, DisplayHeight,
			pSequence->GetAspectRatioInfo(), AspectX, AspectY);

		// 通知
		NotifyVideoInfo();
	}
}


}	// namespace LibISDB::DirectShow
