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
 @file   VideoRenderer_VMR9.cpp
 @brief  VMR-9 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_VMR9.hpp"
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>
#include "../DirectShowUtilities.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


bool VideoRenderer_VMR9::Initialize(
	IGraphBuilder *pGraphBuilder, IPin *pInputPin, HWND hwndRender, HWND hwndMessageDrain)
{
	if (pGraphBuilder == nullptr) {
		SetHRESULTError(E_POINTER);
		return false;
	}

	HRESULT hr;

	hr = ::CoCreateInstance(
		CLSID_VideoMixingRenderer9, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(m_Renderer.GetPP()));
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("VMR-9 のインスタンスを作成できません。"));
		return false;
	}

	IVMRFilterConfig9 *pFilterConfig;
	hr = m_Renderer.QueryInterface(&pFilterConfig);
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("IVMRFilterConfig9 を取得できません。"));
		return false;
	}
	pFilterConfig->SetRenderingMode(VMR9Mode_Windowless);
	pFilterConfig->Release();

	IVMRMixerControl9 *pMixerControl;
	if (SUCCEEDED(m_Renderer.QueryInterface(&pMixerControl))) {
		DWORD MixingPref;

		pMixerControl->GetMixingPrefs(&MixingPref);
		MixingPref = (MixingPref & ~MixerPref9_DecimateMask) | MixerPref9_NonSquareMixing;
		pMixerControl->SetMixingPrefs(MixingPref);
		pMixerControl->Release();
	}

	IVMRWindowlessControl9 *pWindowlessControl;
	hr = m_Renderer.QueryInterface(&pWindowlessControl);
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("IVMRWindowlessControl9 を取得できません。"));
		return false;
	}
	pWindowlessControl->SetVideoClippingWindow(hwndRender);
	pWindowlessControl->SetBorderColor(RGB(0, 0, 0));
	pWindowlessControl->SetAspectRatioMode(VMR9ARMode_None);
	RECT rc;
	::GetClientRect(hwndRender, &rc);
	pWindowlessControl->SetVideoPosition(nullptr, &rc);
	pWindowlessControl->Release();

	hr = pGraphBuilder->AddFilter(m_Renderer.Get(), L"VMR9");
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("VMR-9 をフィルタグラフに追加できません。"));
		return false;
	}

	IFilterGraph2 *pFilterGraph2;
	hr = pGraphBuilder->QueryInterface(IID_PPV_ARGS(&pFilterGraph2));
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("IFilterGraph2 を取得できません。"));
		return false;
	}
	hr = pFilterGraph2->RenderEx(pInputPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, nullptr);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("映像レンダラを構築できません。"));
		return false;
	}

	m_GraphBuilder = pGraphBuilder;
	m_hwndRender = hwndRender;

	ResetError();

	return true;
}


bool VideoRenderer_VMR9::SetVideoPosition(
	int SourceWidth, int SourceHeight, const RECT &SourceRect,
	const RECT &DestRect, const RECT &WindowRect)
{
	if (!m_Renderer)
		return false;

	IVMRWindowlessControl9 *pWindowlessControl;
	HRESULT hr;

	hr = m_Renderer.QueryInterface(&pWindowlessControl);
	if (FAILED(hr))
		return false;

	RECT rcSrc, rcDest;
	LONG Width, Height;

	if (SUCCEEDED(pWindowlessControl->GetNativeVideoSize(&Width, &Height, nullptr, nullptr))) {
		if (SourceWidth > 0 && SourceHeight > 0) {
			rcSrc = MapRect(SourceRect, Width, SourceWidth, Height, SourceHeight);
		} else {
			rcSrc = SourceRect;
		}
		if (m_Crop1088To1080 && Height == 1088) {
			rcSrc.top    = ::MulDiv(rcSrc.top,    1080, 1088);
			rcSrc.bottom = ::MulDiv(rcSrc.bottom, 1080, 1088);
		}
	} else {
		rcSrc = SourceRect;
	}

	rcDest = DestRect;
	::OffsetRect(&rcDest, WindowRect.left, WindowRect.top);
	pWindowlessControl->SetVideoPosition(&rcSrc, &rcDest);
	pWindowlessControl->Release();
	::InvalidateRect(m_hwndRender, nullptr, TRUE);

	return true;
}


bool VideoRenderer_VMR9::GetDestPosition(ReturnArg<RECT> Rect)
{
	bool Result = false;

	if (m_Renderer && Rect) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_Renderer.QueryInterface(&pWindowlessControl))) {
			Result = SUCCEEDED(pWindowlessControl->GetVideoPosition(nullptr, &*Rect));
			pWindowlessControl->Release();
		}
	}

	return Result;
}


COMMemoryPointer<> VideoRenderer_VMR9::GetCurrentImage()
{
	BYTE *pDib = nullptr;

	if (m_Renderer) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_Renderer.QueryInterface(&pWindowlessControl))) {
			if (FAILED(pWindowlessControl->GetCurrentImage(&pDib)))
				pDib = nullptr;
			pWindowlessControl->Release();
		}
	}

	return COMMemoryPointer<>(pDib);
}


bool VideoRenderer_VMR9::RepaintVideo(HWND hwnd, HDC hdc)
{
	bool Result = false;

	if (m_Renderer) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_Renderer.QueryInterface(&pWindowlessControl))) {
			if (SUCCEEDED(pWindowlessControl->RepaintVideo(hwnd, hdc)))
				Result = true;
			pWindowlessControl->Release();
		}
	}

	return Result;
}


bool VideoRenderer_VMR9::DisplayModeChanged()
{
	bool Result = false;

	if (m_Renderer) {
		IVMRWindowlessControl9 *pWindowlessControl;

		if (SUCCEEDED(m_Renderer.QueryInterface(&pWindowlessControl))) {
			if (SUCCEEDED(pWindowlessControl->DisplayModeChanged()))
				Result = true;
			pWindowlessControl->Release();
		}
	}

	return Result;
}


bool VideoRenderer_VMR9::SetVisible(bool Visible)
{
	// ウィンドウを再描画させる
	if (m_hwndRender != nullptr)
		return ::InvalidateRect(m_hwndRender, nullptr, TRUE) != FALSE;
	return false;
}


}	// namespace LibISDB::DirectShow
