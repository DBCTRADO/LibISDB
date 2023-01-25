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
 @file   VideoRenderer_EVR.cpp
 @brief  EVR 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_EVR.hpp"
#include <d3d9.h>
#include <evr.h>
#include <evr9.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <shlwapi.h>
#include "../DirectShowUtilities.hpp"
#include "../../../../Base/DebugDef.hpp"


#pragma comment(lib, "mfuuid.lib")


namespace LibISDB::DirectShow
{


#define EVR_VIDEO_WINDOW_CLASS TEXT("LibISDB EVR Video Window")




bool VideoRenderer_EVR::Initialize(
	IGraphBuilder *pGraphBuilder, IPin *pInputPin, HWND hwndRender, HWND hwndMessageDrain)
{
	if (pGraphBuilder == nullptr) {
		SetHRESULTError(E_POINTER);
		return false;
	}

#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	static bool Registered = false;
	HINSTANCE hinst = GetWindowInstance(hwndRender);

	if (!Registered) {
		WNDCLASS wc;

		wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = VideoWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = nullptr;
		wc.hbrBackground = ::CreateSolidBrush(RGB(0, 0, 0));
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = EVR_VIDEO_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0) {
			SetWin32Error(::GetLastError(), LIBISDB_STR("EVRウィンドウクラスを登録できません。"));
			return false;
		}
		Registered = true;
	}

	m_hwndVideo = ::CreateWindowEx(
		0, EVR_VIDEO_WINDOW_CLASS, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 0, 0,
		hwndRender, nullptr, hinst, this);
	if (m_hwndVideo == nullptr) {
		SetWin32Error(::GetLastError(), LIBISDB_STR("EVRウィンドウを作成できません。"));
		return false;
	}
#endif

	HRESULT hr;

	hr = ::CoCreateInstance(
		CLSID_EnhancedVideoRenderer, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(m_Renderer.GetPP()));
	if (FAILED(hr)) {
		SetError(
			HRESULTErrorCode(hr),
			LIBISDB_STR("EVRのインスタンスを作成できません。"),
			LIBISDB_STR("システムがEVRに対応していない可能性があります。"));
		goto OnError;
	}

	hr = pGraphBuilder->AddFilter(m_Renderer.Get(), L"EVR");
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("EVRをフィルタグラフに追加できません。"));
		goto OnError;
	}

	hr = InitializePresenter(m_Renderer.Get());
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("カスタムプレゼンタを初期化できません。"));
		goto OnError;
	}

	IEVRFilterConfig *pFilterConfig;
	hr = m_Renderer.QueryInterface(&pFilterConfig);
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("IEVRFilterConfig を取得できません。"));
		goto OnError;
	}
	pFilterConfig->SetNumberOfStreams(1);
	pFilterConfig->Release();

	IMFGetService *pGetService;
	hr = m_Renderer.QueryInterface(&pGetService);
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("IMFGetService を取得できません。"));
		goto OnError;
	}

	IMFVideoDisplayControl *pDisplayControl;
	hr = pGetService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplayControl));
	if (FAILED(hr)) {
		pGetService->Release();
		SetHRESULTError(hr, LIBISDB_STR("IMFVideoDisplayControl を取得できません。"));
		goto OnError;
	}
#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	pDisplayControl->SetVideoWindow(m_hwndVideo);
#else
	pDisplayControl->SetVideoWindow(hwndRender);
#endif
	pDisplayControl->SetAspectRatioMode(MFVideoARMode_None);
	/*
	RECT rc;
	::GetClientRect(hwndRender, &rc);
	pDisplayControl->SetVideoPosition(nullptr, &rc);
	*/
	pDisplayControl->SetBorderColor(RGB(0, 0, 0));

	UpdateRenderingPrefs(pDisplayControl);

	pDisplayControl->Release();

	IMFVideoProcessor *pVideoProcessor;
	hr = pGetService->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pVideoProcessor));
	if (FAILED(hr)) {
		pGetService->Release();
		SetHRESULTError(hr, LIBISDB_STR("IMFVideoProcessor を取得できません。"));
		goto OnError;
	}
	pVideoProcessor->SetBackgroundColor(RGB(0, 0, 0));

/*
	UINT NumModes;
	GUID *pProcessingModes;
	if (SUCCEEDED(pVideoProcessor->GetAvailableVideoProcessorModes(&NumModes, &pProcessingModes))) {
#ifdef LIBISDB_DEBUG
		for (UINT i = 0; i < NumModes; i++) {
			DXVA2_VideoProcessorCaps Caps;

			if (SUCCEEDED(pVideoProcessor->GetVideoProcessorCaps(&pProcessingModes[i], &Caps))) {
				LIBISDB_TRACE(LIBISDB_STR("EVR Video Processor {}\n"), i);
				LIBISDB_TRACE(
					LIBISDB_STR("DeviceCaps : {}\n"),
					Caps.DeviceCaps == DXVA2_VPDev_EmulatedDXVA1 ?
						LIBISDB_STR("DXVA2_VPDev_EmulatedDXVA1"):
					Caps.DeviceCaps == DXVA2_VPDev_HardwareDevice ?
						LIBISDB_STR("DXVA2_VPDev_HardwareDevice"):
					Caps.DeviceCaps == DXVA2_VPDev_SoftwareDevice ?
						LIBISDB_STR("DXVA2_VPDev_SoftwareDevice") :
						LIBISDB_STR("Unknown"));
			}
		}
#endif
		for (UINT i = 0; i < NumModes; i++) {
			DXVA2_VideoProcessorCaps Caps;

			if (SUCCEEDED(pVideoProcessor->GetVideoProcessorCaps(&pProcessingModes[i], &Caps))) {
				if (Caps.DeviceCaps == DXVA2_VPDev_HardwareDevice) {
					pVideoProcessor->SetVideoProcessorMode(&pProcessingModes[i]);
					break;
				}
			}
		}
		::CoTaskMemFree(pProcessingModes);
	}
*/

	pVideoProcessor->Release();

	pGetService->Release();

	IFilterGraph2 *pFilterGraph2;
	hr = pGraphBuilder->QueryInterface(IID_PPV_ARGS(&pFilterGraph2));
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("IFilterGraph2 を取得できません。"));
		goto OnError;
	}
	hr = pFilterGraph2->RenderEx(pInputPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, nullptr);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("映像レンダラを構築できません。"));
		goto OnError;
	}

	m_GraphBuilder = pGraphBuilder;
	m_hwndRender = hwndRender;
#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	m_hwndMessageDrain = hwndMessageDrain;
#endif

	ResetError();

	return true;

OnError:
	m_Renderer.Release();

#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	::DestroyWindow(m_hwndVideo);
	m_hwndVideo = nullptr;
#endif

	return false;
}


bool VideoRenderer_EVR::Finalize()
{
	VideoRenderer::Finalize();

#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	if (m_hwndVideo != nullptr) {
		::DestroyWindow(m_hwndVideo);
		m_hwndVideo = nullptr;
	}
#endif

	return true;
}


bool VideoRenderer_EVR::SetVideoPosition(
	int SourceWidth, int SourceHeight, const RECT &SourceRect,
	const RECT &DestRect, const RECT &WindowRect)
{
	bool Result = false;

	if (m_Renderer) {
		IMFVideoDisplayControl *pDisplayControl = GetVideoDisplayControl();

		if (pDisplayControl != nullptr) {
			MFVideoNormalizedRect rcSrc;
			RECT rcDest;

			rcSrc.left   = (float)SourceRect.left   / (float)SourceWidth;
			rcSrc.top    = (float)SourceRect.top    / (float)SourceHeight;
			rcSrc.right  = (float)SourceRect.right  / (float)SourceWidth;
			rcSrc.bottom = (float)SourceRect.bottom / (float)SourceHeight;
			rcDest = DestRect;
			::OffsetRect(&rcDest, WindowRect.left, WindowRect.top);
#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
			::SetWindowPos(
				m_hwndVideo, HWND_BOTTOM,
				rcDest.left, rcDest.top,
				rcDest.right - rcDest.left, rcDest.bottom - rcDest.top,
				SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
			/*
			::OffsetRect(&rcDest, -rcDest.left, -rcDest.top);
			Result = SUCCEEDED(pDisplayControl->SetVideoPosition(&rcSrc, &rcDest));
			*/
			Result = SUCCEEDED(pDisplayControl->SetVideoPosition(&rcSrc, nullptr));
#else
			Result = SUCCEEDED(pDisplayControl->SetVideoPosition(&rcSrc, &rcDest));

			// EVRのバグでバックバッファがクリアされない時があるので、強制的にクリアする
			COLORREF crBorder;
			pDisplayControl->GetBorderColor(&crBorder);
			pDisplayControl->SetBorderColor(crBorder == 0 ? RGB(1, 1, 1) : RGB(0, 0, 0));
			::InvalidateRect(m_hwndRender, nullptr, TRUE);
#endif

			pDisplayControl->Release();
		}
	}

	return Result;
}


bool VideoRenderer_EVR::GetDestPosition(ReturnArg<RECT> Rect)
{
	if (!Rect)
		return false;

	bool Result = false;

#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	if (m_hwndVideo != nullptr) {
		if (::GetWindowRect(m_hwndVideo, &*Rect)) {
			::MapWindowRect(nullptr, m_hwndRender, &*Rect);
			Result = true;
		}
	}
#else
	if (m_Renderer) {
		IMFVideoDisplayControl *pDisplayControl = GetVideoDisplayControl();

		if (pDisplayControl != nullptr) {
			MFVideoNormalizedRect rcSrc;

			Result = SUCCEEDED(pDisplayControl->GetVideoPosition(&rcSrc, &*Rect));
			pDisplayControl->Release();
		}
	}
#endif

	if (!Result)
		::SetRectEmpty(&*Rect);

	return Result;
}


COMMemoryPointer<> VideoRenderer_EVR::GetCurrentImage()
{
	BYTE *pDib = nullptr;

	if (m_Renderer) {
		IMFVideoDisplayControl *pDisplayControl = GetVideoDisplayControl();

		if (pDisplayControl) {
			BITMAPINFOHEADER bmih;
			BYTE *pBits;
			DWORD BitsSize;
			LONGLONG TimeStamp = 0;

			bmih.biSize = sizeof(BITMAPINFOHEADER);
			if (SUCCEEDED(pDisplayControl->GetCurrentImage(&bmih, &pBits, &BitsSize, &TimeStamp))) {
				pDib = static_cast<BYTE *>(::CoTaskMemAlloc(sizeof(BITMAPINFOHEADER) + BitsSize));
				if (pDib != nullptr) {
					::CopyMemory(pDib, &bmih, sizeof(BITMAPINFOHEADER));
					::CopyMemory(pDib + sizeof(BITMAPINFOHEADER), pBits, BitsSize);
				}
				::CoTaskMemFree(pBits);
			}

			pDisplayControl->Release();
		}
	}

	return COMMemoryPointer<>(pDib);
}


bool VideoRenderer_EVR::ShowCursor(bool Show)
{
#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	if (m_ShowCursor != Show) {
		if (m_hwndVideo != nullptr) {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::GetWindowRect(m_hwndVideo, &rc);
			if (::PtInRect(&rc, pt))
				::SetCursor(Show ? ::LoadCursor(nullptr, IDC_ARROW) : nullptr);
		}

		m_ShowCursor = Show;
	}
#endif

	return true;
}


bool VideoRenderer_EVR::RepaintVideo(HWND hwnd, HDC hdc)
{
	bool Result = false;

	if (m_Renderer) {
		IMFVideoDisplayControl *pDisplayControl = GetVideoDisplayControl();

		if (pDisplayControl != nullptr) {
			Result = SUCCEEDED(pDisplayControl->RepaintVideo());
			pDisplayControl->Release();
		}
	}

	return Result;
}


bool VideoRenderer_EVR::DisplayModeChanged()
{
	return true;
}


bool VideoRenderer_EVR::SetVisible(bool Visible)
{
#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
	if (m_hwndVideo == nullptr)
		return false;
	return ::ShowWindow(m_hwndVideo, Visible ? SW_SHOW : SW_HIDE) != FALSE;
#else
	return true;
#endif
}


bool VideoRenderer_EVR::SetClipToDevice(bool Clip)
{
	if (m_ClipToDevice != Clip) {
		m_ClipToDevice = Clip;

		if (m_Renderer) {
			IMFVideoDisplayControl *pDisplayControl = GetVideoDisplayControl();

			if (pDisplayControl != nullptr) {
				UpdateRenderingPrefs(pDisplayControl);
				pDisplayControl->Release();
			}
		}
	}

	return true;
}


IMFVideoDisplayControl * VideoRenderer_EVR::GetVideoDisplayControl() const
{
	HRESULT hr;
	IMFGetService *pGetService;
	IMFVideoDisplayControl *pDisplayControl;

	hr = m_Renderer.QueryInterface(&pGetService);
	if (FAILED(hr))
		return nullptr;
	hr = pGetService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplayControl));
	pGetService->Release();
	if (FAILED(hr))
		return nullptr;
	return pDisplayControl;
}


HRESULT VideoRenderer_EVR::UpdateRenderingPrefs(IMFVideoDisplayControl *pDisplayControl)
{
	DWORD RenderingPrefs;
	HRESULT hr = pDisplayControl->GetRenderingPrefs(&RenderingPrefs);
	if (SUCCEEDED(hr)) {
		LIBISDB_TRACE(LIBISDB_STR("ClipToDevice = {}\n"), m_ClipToDevice);
		if (!m_ClipToDevice)
			RenderingPrefs |= MFVideoRenderPrefs_DoNotClipToDevice;
		else
			RenderingPrefs &= ~MFVideoRenderPrefs_DoNotClipToDevice;
		hr = pDisplayControl->SetRenderingPrefs(RenderingPrefs);
	}
	return hr;
}


#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW


VideoRenderer_EVR * VideoRenderer_EVR::GetThis(HWND hwnd)
{
	return reinterpret_cast<VideoRenderer_EVR *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CALLBACK VideoRenderer_EVR::VideoWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			VideoRenderer_EVR *pThis = static_cast<VideoRenderer_EVR *>(
				reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_SIZE:
		{
			VideoRenderer_EVR *pThis = GetThis(hwnd);

			if (pThis->m_Renderer) {
				IMFVideoDisplayControl *pDisplayControl = pThis->GetVideoDisplayControl();

				if (pDisplayControl) {
					RECT rc = {0, 0, LOWORD(lParam), HIWORD(lParam)};

					pDisplayControl->SetVideoPosition(nullptr, &rc);
					pDisplayControl->Release();
				}
			}
		}
		return 0;

	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_MOUSEACTIVATE:
	case WM_NCLBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCMOUSEMOVE:
		{
			VideoRenderer_EVR *pThis = GetThis(hwnd);

			if (pThis->m_hwndMessageDrain) {
				::PostMessage(pThis->m_hwndMessageDrain, uMsg, wParam, lParam);
				return 0;
			}
		}
		break;

	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		{
			VideoRenderer_EVR *pThis = GetThis(hwnd);

			if (pThis->m_hwndMessageDrain) {
				POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

				::MapWindowPoints(hwnd, pThis->m_hwndMessageDrain, &pt, 1);
				::PostMessage(pThis->m_hwndMessageDrain, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
				return 0;
			}
		}
		break;

	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			VideoRenderer_EVR *pThis = GetThis(hwnd);

			::SetCursor(pThis->m_ShowCursor ? ::LoadCursor(nullptr, IDC_ARROW) : nullptr);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			VideoRenderer_EVR *pThis = GetThis(hwnd);

			pThis->m_hwndVideo = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


#endif	// LIBISDB_EVR_USE_VIDEO_WINDOW


}	// namespace LibISDB::DirectShow
