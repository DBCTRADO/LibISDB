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
 @file   VideoRenderer_VMR7Renderless.cpp
 @brief  VMR-7 Renderless 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_VMR7Renderless.hpp"
#include "../DirectShowUtilities.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


VideoRenderer_VMR7Renderless::VideoRenderer_VMR7Renderless()
	: m_RefCount(1)
	, m_Duration(-1)
{
}


HRESULT VideoRenderer_VMR7Renderless::CreateDefaultAllocatorPresenter(HWND hwndRender)
{
	HRESULT hr;

	hr = ::CoCreateInstance(
		CLSID_AllocPresenter, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(m_SurfaceAllocator.GetPP()));
	if (SUCCEEDED(hr)) {
		m_SurfaceAllocator->AdviseNotify(this);
		hr = m_SurfaceAllocator.QueryInterface(&m_ImagePresenter);
		if (SUCCEEDED(hr)) {
			IVMRWindowlessControl *pWindowlessControl;

			hr = m_SurfaceAllocator.QueryInterface(&pWindowlessControl);
			if (SUCCEEDED(hr)) {
				RECT rc;

				pWindowlessControl->SetVideoClippingWindow(hwndRender);
				pWindowlessControl->SetBorderColor(RGB(0, 0, 0));
				pWindowlessControl->SetAspectRatioMode(VMR_ARMODE_NONE);
				::GetClientRect(hwndRender, &rc);
				pWindowlessControl->SetVideoPosition(nullptr, &rc);
				pWindowlessControl->Release();
			}
		}
	}

	if (FAILED(hr)) {
		m_ImagePresenter.Release();
		m_SurfaceAllocator.Release();
	}

	return hr;
}


STDMETHODIMP VideoRenderer_VMR7Renderless::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IVMRSurfaceAllocator) {
		*ppvObject = static_cast<IVMRSurfaceAllocator *>(this);
	} else if (riid == IID_IVMRSurfaceAllocatorNotify) {
		*ppvObject = static_cast<IVMRSurfaceAllocatorNotify *>(this);
	} else if (riid == IID_IVMRImagePresenter) {
		*ppvObject = static_cast<IVMRImagePresenter *>(this);
	} else if (riid == IID_IUnknown) {
		*ppvObject = static_cast<IUnknown *>(static_cast<IVMRSurfaceAllocator *>(this));
	} else {
		*ppvObject = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


STDMETHODIMP_(ULONG) VideoRenderer_VMR7Renderless::AddRef()
{
	return ::InterlockedIncrement(&m_RefCount);
}


STDMETHODIMP_(ULONG) VideoRenderer_VMR7Renderless::Release()
{
	LONG Result = ::InterlockedDecrement(&m_RefCount);
	if (Result == 0)
		delete this;
	return Result;
}


STDMETHODIMP VideoRenderer_VMR7Renderless::AllocateSurface(
	DWORD_PTR dwUserID, VMRALLOCATIONINFO *lpAllocInfo, DWORD *lpdwBuffer, LPDIRECTDRAWSURFACE7 *lplpSurface)
{
	return m_SurfaceAllocator->AllocateSurface(dwUserID, lpAllocInfo, lpdwBuffer, lplpSurface);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::FreeSurface(DWORD_PTR dwUserID)
{
	return m_SurfaceAllocator->FreeSurface(dwUserID);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::PrepareSurface(
	DWORD_PTR dwUserID, LPDIRECTDRAWSURFACE7 lplpSurface, DWORD dwSurfaceFlags)
{
	return m_SurfaceAllocator->PrepareSurface(dwUserID, lplpSurface, dwSurfaceFlags);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::AdviseNotify(
	IVMRSurfaceAllocatorNotify *lpIVMRSurfAllocNotify)
{
	return m_SurfaceAllocator->AdviseNotify(lpIVMRSurfAllocNotify);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::AdviseSurfaceAllocator(
	DWORD_PTR dwUserID, IVMRSurfaceAllocator *lpIVRMSurfaceAllocator)
{
	return m_SurfaceAllocatorNotify->AdviseSurfaceAllocator(dwUserID, lpIVRMSurfaceAllocator);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor)
{
    return m_SurfaceAllocatorNotify->SetDDrawDevice(lpDDrawDevice, hMonitor);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor)
{
	return m_SurfaceAllocatorNotify->ChangeDDrawDevice(lpDDrawDevice, hMonitor);
}


static HRESULT WINAPI DDSurfEnumFunc(LPDIRECTDRAWSURFACE7 pdds, DDSURFACEDESC2 *pddsd, void *lpContext)
{
	LPDIRECTDRAWSURFACE7 *ppdds = static_cast<LPDIRECTDRAWSURFACE7*>(lpContext);
	DDSURFACEDESC2 ddsd = {};

	ddsd.dwSize = sizeof(ddsd);
	HRESULT hr = pdds->GetSurfaceDesc(&ddsd);
	if (SUCCEEDED(hr)) {
		if (ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
			*ppdds = pdds;
			return DDENUMRET_CANCEL;
		}
	}

	pdds->Release();

	return DDENUMRET_OK;
}


HRESULT VideoRenderer_VMR7Renderless::OnSetDDrawDevice(LPDIRECTDRAW7 pDD, HMONITOR hMon)
{
	HRESULT hr;

	ReleaseSurfaces();

	DDSURFACEDESC2 ddsd = {};

	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	hr = pDD->EnumSurfaces(
		DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL,
		&ddsd, m_PrimarySurface.GetPP(), DDSurfEnumFunc);
	if (FAILED(hr))
		return hr;
	if (!m_PrimarySurface)
		return E_FAIL;

	MONITORINFOEX mix;
	mix.cbSize = sizeof(mix);
	::GetMonitorInfo(hMon, &mix);

	::ZeroMemory(&ddsd, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth  = mix.rcMonitor.right  - mix.rcMonitor.left;
	ddsd.dwHeight = mix.rcMonitor.bottom - mix.rcMonitor.top;

	hr=pDD->CreateSurface(&ddsd, m_PriTextSurface.GetPP(), nullptr);
	if (SUCCEEDED(hr))
		hr = pDD->CreateSurface(&ddsd, m_RenderTSurface.GetPP(), nullptr);

	if (FAILED(hr))
		ReleaseSurfaces();

	return hr;
}


STDMETHODIMP VideoRenderer_VMR7Renderless::RestoreDDrawSurfaces()
{
    return m_SurfaceAllocatorNotify->RestoreDDrawSurfaces();
}


STDMETHODIMP VideoRenderer_VMR7Renderless::NotifyEvent(LONG EventCode, LONG_PTR lp1, LONG_PTR lp2)
{
	return m_SurfaceAllocatorNotify->NotifyEvent(EventCode, lp1, lp2);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::SetBorderColor(COLORREF clr)
{
	return m_SurfaceAllocatorNotify->SetBorderColor(clr);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::StartPresenting(DWORD_PTR dwUserID)
{
	return m_ImagePresenter->StartPresenting(dwUserID);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::StopPresenting(DWORD_PTR dwUserID)
{
	return m_ImagePresenter->StopPresenting(dwUserID);
}


STDMETHODIMP VideoRenderer_VMR7Renderless::PresentImage(
	DWORD_PTR dwUserID, VMRPRESENTATIONINFO* lpPresInfo)
{
#if 0
	LPDIRECTDRAWSURFACE7 lpSurface = lpPresInfo->lpSurf;
	const REFERENCE_TIME rtNow = lpPresInfo->rtStart;
	const DWORD dwSurfaceFlags = lpPresInfo->dwFlags;

	if (m_iDuration > 0) {
		HRESULT hr;
		RECT rs,rd;
		DDSURFACEDESC2 ddsdV = {};

		ddsdV.dwSize = sizeof(ddsd);
		hr = lpSurface->GetSurfaceDesc(&ddsdV);

		DDSURFACEDESC2 ddsdP = {};
		ddsdP.dwSize = sizeof(ddsd);
		hr = m_PriTextSurface->GetSurfaceDesc(&ddsdP);

		FLOAT fPos = (FLOAT)m_Duration / 30.0F;
		FLOAT fPosInv = 1.0F - fPos;

		::SetRect(&rs, 0, 0,
			::MulDiv(ddsdV.dwWidth, 30 - m_Duration, 30),
			ddsdV.dwHeight);
		::SetRect(&rd, 0, 0,
			::MulDiv(ddsdP.dwWidth, 30 - m_Duration, 30),
			ddsdP.dwHeight);
		hr = m_RenderTSurface->Blt(&rd, lpSurface, &rs, DDBLT_WAIT, nullptr);

		::SetRect(&rs, 0, 0,
			::MulDiv(ddsdP.dwWidth, m_Duration, 30),
			ddsdP.dwHeight);
		::SetRect(&rd,
			(LONG)ddsdP.dwWidth - ::MulDiv(ddsdP.dwWidth, m_Duration, 30),
			0,
			ddsdP.dwWidth,
			ddsdP.dwHeight);
		hr = m_RenderTSurface->Blt(&rd, m_PriTextSurface.Get(), &rs, DDBLT_WAIT, nullptr);

		LPDIRECTDRAW lpdd;
		hr = m_PrimarySurface->GetDDInterface((LPVOID*)&lpdd);
		if (SUCCEEDED(hr)) {
			DWORD dwScanLine;
			for (;;) {
				hr = lpdd->GetScanLine(&dwScanLine);
				if (hr == DDERR_VERTICALBLANKINPROGRESS)
					break;
				if (FAILED(hr))
					break;
				if ((LONG)dwScanLine >= rd.top) {
					if ((LONG)dwScanLine <= rd.bottom)
						continue;
				}
				break;
			}
			lpdd->Release();
		}
		hr = m_PrimarySurface->Blt(nullptr, m_RenderTSurface.Get(), nullptr, DDBLT_WAIT, nullptr);
		m_Duration--;
		if (m_Duration == 0 && (ddsdV.ddsCaps.dwCaps & DDSCAPS_OVERLAY) != 0) {
			::InvalidateRect(m_hwndRender, nullptr, FALSE);
		}
		return hr;
	}
#endif

	return m_ImagePresenter->PresentImage(dwUserID, lpPresInfo);
}


bool VideoRenderer_VMR7Renderless::Initialize(
	IGraphBuilder *pGraphBuilder, IPin *pInputPin, HWND hwndRender, HWND hwndMessageDrain)
{
	if (pGraphBuilder == nullptr) {
		SetHRESULTError(E_POINTER);
		return false;
	}

	HRESULT hr;

	hr = ::CoCreateInstance(
		CLSID_VideoMixingRenderer, nullptr, CLSCTX_INPROC,
		IID_PPV_ARGS(m_Renderer.GetPP()));
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("VMRのインスタンスを作成できません。"));
		return false;
	}

	hr = pGraphBuilder->AddFilter(m_Renderer.Get(), L"VMR");
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("VMRをフィルタグラフに追加できません。"));
		return false;
	}

	IVMRFilterConfig *pFilterConfig;
	hr = m_Renderer.QueryInterface(&pFilterConfig);
	pFilterConfig->SetRenderingMode(VMRMode_Renderless);
	pFilterConfig->Release();
	m_Renderer.QueryInterface(&m_SurfaceAllocatorNotify);
	CreateDefaultAllocatorPresenter(hwndRender);
	m_SurfaceAllocatorNotify->AdviseSurfaceAllocator(1234, this);

	IFilterGraph2 *pFilterGraph2;
	hr=pGraphBuilder->QueryInterface(IID_PPV_ARGS(&pFilterGraph2));
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("IFilterGraph2を取得できません。"));
		return false;
	}
	hr = pFilterGraph2->RenderEx(pInputPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, nullptr);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("映像レンダラを構築できません。"));
		return false;
	}

	m_GraphBuilder = pGraphBuilder;
	m_hwndRender = hwndRender;

	ResetError();

	return true;
}


bool VideoRenderer_VMR7Renderless::Finalize()
{
	ReleaseSurfaces();

	m_SurfaceAllocatorNotify.Release();
	m_SurfaceAllocator.Release();
	m_ImagePresenter.Release();

	VideoRenderer::Finalize();

	return true;
}


bool VideoRenderer_VMR7Renderless::SetVideoPosition(
	int SourceWidth, int SourceHeight, const RECT &SourceRect,
	const RECT &DestRect, const RECT &WindowRect)
{
	if (!m_SurfaceAllocator)
		return false;

	IVMRWindowlessControl *pWindowlessControl;
	HRESULT hr;

	hr = m_SurfaceAllocator.QueryInterface(&pWindowlessControl);
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


bool VideoRenderer_VMR7Renderless::GetDestPosition(ReturnArg<RECT> Rect)
{
	bool OK = false;

	if (m_Renderer && Rect) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_SurfaceAllocator.QueryInterface(&pWindowlessControl))) {
			OK = SUCCEEDED(pWindowlessControl->GetVideoPosition(nullptr, &*Rect));
			pWindowlessControl->Release();
		}
	}

	return OK;
}


COMMemoryPointer<> VideoRenderer_VMR7Renderless::GetCurrentImage()
{
	BYTE *pDib = nullptr;

	if (m_Renderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_SurfaceAllocator.QueryInterface(&pWindowlessControl))) {
			if (FAILED(pWindowlessControl->GetCurrentImage(&pDib)))
				pDib = nullptr;
			pWindowlessControl->Release();
		}
	}

	return COMMemoryPointer<>(pDib);
}


bool VideoRenderer_VMR7Renderless::RepaintVideo(HWND hwnd, HDC hdc)
{
	bool fOK = false;

	if (m_Renderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_SurfaceAllocator.QueryInterface(&pWindowlessControl))) {
			if (SUCCEEDED(pWindowlessControl->RepaintVideo(hwnd, hdc)))
				fOK = true;
			pWindowlessControl->Release();
		}
	}

	return fOK;
}


bool VideoRenderer_VMR7Renderless::DisplayModeChanged()
{
	bool fOK = false;

	if (m_Renderer) {
		IVMRWindowlessControl *pWindowlessControl;

		if (SUCCEEDED(m_SurfaceAllocator.QueryInterface(&pWindowlessControl))) {
			if (SUCCEEDED(pWindowlessControl->DisplayModeChanged()))
				fOK = true;
			pWindowlessControl->Release();
		}
	}

	return fOK;
}


bool VideoRenderer_VMR7Renderless::SetVisible(bool Visible)
{
	// ウィンドウを再描画させる
	if (m_hwndRender != nullptr)
		return ::InvalidateRect(m_hwndRender, nullptr, TRUE) != FALSE;
	return false;
}


void VideoRenderer_VMR7Renderless::ReleaseSurfaces()
{
	m_RenderTSurface.Release();
	m_PriTextSurface.Release();
	m_PrimarySurface.Release();
}


}	// namespace LibISDB::DirectShow
