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
 @file   H265ParserFilter.cpp
 @brief  H.265 解析フィルタ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "H265ParserFilter.hpp"
#include "../DirectShowUtilities.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


H265ParserFilter::H265ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransInPlaceFilter(TEXT("H.265 Parser Filter"), pUnk, __uuidof(H265ParserFilter), phr, FALSE)
	, m_H265Parser(this)
{
	LIBISDB_TRACE(LIBISDB_STR("H265ParserFilter::H265ParserFilter() {}\n"), static_cast<void *>(this));

	*phr = S_OK;
}


H265ParserFilter::~H265ParserFilter()
{
	LIBISDB_TRACE(LIBISDB_STR("H265ParserFilter::~H265ParserFilter()\n"));
}


IBaseFilter* WINAPI H265ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	H265ParserFilter *pNewFilter = new H265ParserFilter(pUnk, phr);
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


HRESULT H265ParserFilter::CheckInputType(const CMediaType *mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video) {
		if (*mtIn->Subtype() == MEDIASUBTYPE_HEVC) {
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT H265ParserFilter::Transform(IMediaSample *pSample)
{
	BYTE *pData = nullptr;
	const HRESULT hr = pSample->GetPointer(&pData);
	if (FAILED(hr))
		return hr;
	const LONG DataSize = pSample->GetActualDataLength();

	CAutoLock Lock(&m_ParserLock);

	// シーケンスを取得
	m_H265Parser.StoreES(pData, DataSize);

	if (m_pStreamCallback)
		m_pStreamCallback->OnStream(MAKEFOURCC('H','2','6','5'), pData, DataSize);

	return S_OK;
}


HRESULT H265ParserFilter::Receive(IMediaSample *pSample)
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


HRESULT H265ParserFilter::StartStreaming()
{
	CAutoLock Lock(&m_ParserLock);

	m_H265Parser.Reset();
	m_VideoInfo.Reset();

	return S_OK;
}


HRESULT H265ParserFilter::StopStreaming()
{
	return S_OK;
}


HRESULT H265ParserFilter::BeginFlush()
{
	const HRESULT hr = CTransInPlaceFilter::BeginFlush();

	CAutoLock Lock(&m_ParserLock);

	m_H265Parser.Reset();
	m_VideoInfo.Reset();

	return hr;
}


void H265ParserFilter::OnAccessUnit(const H265Parser *pParser, const H265AccessUnit *pAccessUnit)
{
	int OrigWidth, OrigHeight;
	OrigWidth = pAccessUnit->GetHorizontalSize();
	OrigHeight = pAccessUnit->GetVerticalSize();

	uint16_t SARX = 0, SARY = 0;
	int AspectX = 0, AspectY = 0;
	if (pAccessUnit->GetSAR(&SARX, &SARY) && SARX != 0 && SARY != 0) {
		SARToDAR(SARX, SARY, OrigWidth, OrigHeight, &AspectX, &AspectY);
	} else {
		SARToDAR(1, 1, OrigWidth, OrigHeight, &AspectX, &AspectY);
	}

	VideoInfo Info(OrigWidth, OrigHeight, OrigWidth, OrigHeight, AspectX, AspectY);

	H265AccessUnit::TimingInfo TimingInfo;
	if (pAccessUnit->GetTimingInfo(&TimingInfo)) {
		Info.FrameRate.Num = TimingInfo.TimeScale;
		Info.FrameRate.Denom = TimingInfo.NumUnitsInTick;
	}

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		m_VideoInfo = Info;

		LIBISDB_TRACE(
			LIBISDB_STR("H.265 access unit {} x {} [SAR {}:{} (DAR {}:{})] {}/{}\n"),
			OrigWidth, OrigHeight, SARX, SARY, AspectX, AspectY,
			Info.FrameRate.Num, Info.FrameRate.Denom);

		// 通知
		NotifyVideoInfo();
	}
}


}	// namespace LibISDB::DirectShow
