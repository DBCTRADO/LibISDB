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
 @file   VideoRenderer.cpp
 @brief  映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer.hpp"
#include "VideoRenderer_VMR7.hpp"
#include "VideoRenderer_VMR7Renderless.hpp"
#include "VideoRenderer_VMR9.hpp"
#include "VideoRenderer_VMR9Renderless.hpp"
#include "VideoRenderer_EVR.hpp"
#include "VideoRenderer_OverlayMixer.hpp"
#include "VideoRenderer_madVR.hpp"
#include "VideoRenderer_MPCVideoRenderer.hpp"
#include "EVRCustomPresenter/VideoRenderer_EVRCustomPresenter.hpp"
#include "../DirectShowUtilities.hpp"
#include "../../../../Utilities/StringFormat.hpp"
#include "../../../../Utilities/StringUtilities.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


bool VideoRenderer_Default::InitializeBasicVideo(
	IGraphBuilder *pGraphBuilder, HWND hwndRender, HWND hwndMessageDrain)
{
	if (pGraphBuilder == nullptr) {
		SetHRESULTError(E_POINTER);
		return false;
	}

	HRESULT hr;

	hr = pGraphBuilder->QueryInterface(IID_PPV_ARGS(m_VideoWindow.GetPP()));
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("IVideoWindow を取得できません。"));
		return false;
	}

	m_VideoWindow->put_Owner(reinterpret_cast<OAHWND>(hwndRender));
	m_VideoWindow->put_MessageDrain(reinterpret_cast<OAHWND>(hwndMessageDrain));
	m_VideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
	m_VideoWindow->put_BackgroundPalette(OATRUE);
	m_VideoWindow->put_BorderColor(RGB(0, 0, 0));
	WCHAR szCaption[] = L"";
	m_VideoWindow->put_Caption(szCaption);
	RECT rc;
	::GetClientRect(hwndRender, &rc);
	m_VideoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
	m_VideoWindow->SetWindowForeground(OATRUE);
	m_VideoWindow->put_Visible(OATRUE);

	hr = pGraphBuilder->QueryInterface(IID_PPV_ARGS(m_BasicVideo.GetPP()));
	if (FAILED(hr)) {
		m_VideoWindow.Release();
		SetHRESULTError(hr, LIBISDB_STR("IBasicVideo を取得できません。"));
		return false;
	}

	m_GraphBuilder = pGraphBuilder;
	m_hwndRender = hwndRender;

	if (!m_Renderer) {
		IBaseFilter *pFilter = nullptr;
		if (SUCCEEDED(pGraphBuilder->FindFilterByName(L"Video Renderer", &pFilter)))
			m_Renderer.Attach(pFilter);
	}

	if (m_hwndVideo == nullptr && m_Renderer)
		m_hwndVideo = FindVideoWindow();

	return true;
}


HWND VideoRenderer_Default::FindVideoWindow()
{
	if (!m_Renderer)
		return nullptr;

	IEnumPins *pEnumPins;
	if (FAILED(m_Renderer->EnumPins(&pEnumPins)))
		return nullptr;

	HWND hwndVideo = nullptr;
	IPin *pPin;
	ULONG cFetched;

	while ((hwndVideo == nullptr) && (pEnumPins->Next(1, &pPin, &cFetched) == S_OK)) {
		PIN_INFO Pin;

		if (SUCCEEDED(pPin->QueryPinInfo(&Pin))){
			if (Pin.dir == PINDIR_INPUT) {
				IOverlay *pOverlay;
				if (SUCCEEDED(pPin->QueryInterface(IID_PPV_ARGS(&pOverlay)))) {
					HWND hwnd = nullptr;
					if (SUCCEEDED(pOverlay->GetWindowHandle(&hwnd)))
						hwndVideo = hwnd;
					pOverlay->Release();
				}
			}

			if (Pin.pFilter != nullptr)
				Pin.pFilter->Release();
		}

		pPin->Release();
	}

	pEnumPins->Release();

	return hwndVideo;
}


bool VideoRenderer_Default::Initialize(
	IGraphBuilder *pGraphBuilder, IPin *pInputPin, HWND hwndRender, HWND hwndMessageDrain)
{
	if (pGraphBuilder == nullptr) {
		SetHRESULTError(E_POINTER);
		return false;
	}

	const HRESULT hr = pGraphBuilder->Render(pInputPin);
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("映像レンダラを構築できません。"));
		return false;
	}

	if (!InitializeBasicVideo(pGraphBuilder, hwndRender, hwndMessageDrain))
		return false;

	ResetError();

	return true;
}


bool VideoRenderer_Default::Finalize()
{
	m_BasicVideo.Release();

	if (m_VideoWindow) {
		m_VideoWindow->put_Visible(OAFALSE);
		m_VideoWindow->put_Owner(reinterpret_cast<OAHWND>(nullptr));
		m_VideoWindow.Release();
	}

	VideoRenderer::Finalize();

	return true;
}


bool VideoRenderer_Default::SetVideoPosition(
	int SourceWidth, int SourceHeight, const RECT &SourceRect,
	const RECT &DestRect, const RECT &WindowRect)
{
	if (!m_VideoWindow || !m_BasicVideo)
		return false;

	m_BasicVideo->SetSourcePosition(
		SourceRect.left, SourceRect.top,
		SourceRect.right - SourceRect.left, SourceRect.bottom - SourceRect.top);
	m_BasicVideo->SetDestinationPosition(
		DestRect.left, DestRect.top,
		DestRect.right - DestRect.left, DestRect.bottom - DestRect.top);
	m_VideoWindow->SetWindowPosition(
		WindowRect.left, WindowRect.top,
		WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top);

	return true;
}


bool VideoRenderer_Default::GetDestPosition(ReturnArg<RECT> Rect)
{
	if (!Rect)
		return false;

	if (!m_BasicVideo
			|| FAILED(m_BasicVideo->GetDestinationPosition(
					&Rect->left, &Rect->top, &Rect->right, &Rect->bottom))) {
		::SetRectEmpty(&*Rect);
		return false;
	}

	Rect->right += Rect->left;
	Rect->bottom += Rect->top;

	return true;
}


COMMemoryPointer<> VideoRenderer_Default::GetCurrentImage()
{
	if (m_BasicVideo) {
		long Size = 0;

		if (SUCCEEDED(m_BasicVideo->GetCurrentImage(&Size, nullptr)) && (Size > 0)) {
			long *pDib = static_cast<long *>(::CoTaskMemAlloc(Size));

			if (pDib != nullptr) {
				if (SUCCEEDED(m_BasicVideo->GetCurrentImage(&Size, pDib)))
					return COMMemoryPointer<>(reinterpret_cast<BYTE *>(pDib));
				::CoTaskMemFree(pDib);
			}
		}
	}

	return COMMemoryPointer<>();
}


bool VideoRenderer_Default::ShowCursor(bool Show)
{
	if (m_VideoWindow) {
		if (SUCCEEDED(m_VideoWindow->HideCursor(Show ? OAFALSE : OATRUE)))
			return true;
	}
	return false;
}


bool VideoRenderer_Default::SetVisible(bool Visible)
{
	if (m_VideoWindow)
		return SUCCEEDED(m_VideoWindow->put_Visible(Visible ? OATRUE : OAFALSE));
	return false;
}




VideoRenderer_Basic::VideoRenderer_Basic(
	const CLSID &clsid, const CharType *pszName, bool NoSourcePosition) noexcept
	: m_clsidRenderer(clsid)
	, m_RendererName(pszName)
	, m_NoSourcePosition(NoSourcePosition)
{
}


bool VideoRenderer_Basic::Initialize(
	IGraphBuilder *pGraphBuilder, IPin *pInputPin, HWND hwndRender, HWND hwndMessageDrain)
{
	if (pGraphBuilder == nullptr) {
		SetHRESULTError(E_POINTER);
		return false;
	}

	HRESULT hr;
	TCHAR Message[256];

	hr = ::CoCreateInstance(
		m_clsidRenderer, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(m_Renderer.GetPP()));
	if (FAILED(hr)) {
		StringFormat(
			Message,
			LIBISDB_STR("{} のインスタンスを作成できません。"),
			m_RendererName);
		SetError(HRESULTErrorCode(hr), Message, LIBISDB_STR("指定したレンダラがインストールされているか確認してください。"));
		return false;
	}

	hr = pGraphBuilder->AddFilter(m_Renderer.Get(), m_RendererName.c_str());
	if (FAILED(hr)) {
		m_Renderer.Release();
		StringFormat(
			Message,
			LIBISDB_STR("{} をフィルタグラフに追加できません。"),
			m_RendererName);
		SetHRESULTError(hr, Message);
		return false;
	}

	IFilterGraph2 *pFilterGraph2;
	hr = pGraphBuilder->QueryInterface(IID_PPV_ARGS(&pFilterGraph2));
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("IFilterGraph2を取得できません。"));
		return false;
	}
	hr = pFilterGraph2->RenderEx(pInputPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, nullptr);
	pFilterGraph2->Release();
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("映像レンダラを構築できません。"));
		return false;
	}

	if (!InitializeBasicVideo(pGraphBuilder, hwndRender, hwndMessageDrain)) {
		m_Renderer.Release();
		return false;
	}

	ResetError();

	return true;
}


bool VideoRenderer_Basic::SetVideoPosition(
	int SourceWidth, int SourceHeight, const RECT &SourceRect,
	const RECT &DestRect, const RECT &WindowRect)
{
	if (!m_VideoWindow || !m_BasicVideo)
		return false;

	if (m_NoSourcePosition) {
		/*
			IBasicVideo::SetSourcePosition() に対応していないレンダラ用
		*/
		const long CutWidth = SourceRect.right - SourceRect.left;
		const long CutHeight = SourceRect.bottom - SourceRect.top;
		if ((CutWidth <= 0) || (CutHeight <= 0))
			return false;
		const long WindowWidth = WindowRect.right - WindowRect.left;
		const long WindowHeight = WindowRect.bottom - WindowRect.top;
		RECT rcDest = DestRect;
		int DestWidth = rcDest.right - rcDest.left;
		int DestHeight = rcDest.bottom - rcDest.top;

		rcDest.left   -= ::MulDiv(SourceRect.left, DestWidth, CutWidth);
		rcDest.right  += ::MulDiv(SourceWidth - SourceRect.right, DestWidth, CutWidth);
		rcDest.top    -= ::MulDiv(SourceRect.top, DestHeight, CutHeight);
		rcDest.bottom += ::MulDiv(SourceHeight - SourceRect.bottom, DestHeight, CutHeight);
		DestWidth = rcDest.right - rcDest.left;
		DestHeight = rcDest.bottom - rcDest.top;
		//::OffsetRect(&rcDest, (WindowWidth - DestWidth) / 2, (WindowHeight - DestHeight) / 2);

		m_BasicVideo->SetDefaultSourcePosition();
		m_BasicVideo->SetDestinationPosition(rcDest.left, rcDest.top, DestWidth, DestHeight);
		m_VideoWindow->SetWindowPosition(WindowRect.left, WindowRect.top, WindowWidth, WindowHeight);

		LIBISDB_TRACE(
			LIBISDB_STR("VideoRenderer_Basic::SetVideoPosition() : Src [{}, {}, {}, {}] Dest [{}, {}, {}, {}] -> [{}, {}, {}, {}]\n"),
			SourceRect.left, SourceRect.top, SourceRect.right, SourceRect.bottom,
			DestRect.left, DestRect.top, DestRect.right, DestRect.bottom,
			rcDest.left, rcDest.top, rcDest.right, rcDest.bottom);

		return true;
	}

	return VideoRenderer_Default::SetVideoPosition(SourceWidth, SourceHeight, SourceRect, DestRect, WindowRect);
}




VideoRenderer::~VideoRenderer()
{
	LIBISDB_TRACE(LIBISDB_STR("VideoRenderer::~VideoRenderer()\n"));
}


bool VideoRenderer::Finalize()
{
	m_Renderer.Release();
	m_GraphBuilder.Release();
	m_hwndVideo = nullptr;

	return true;
}


bool VideoRenderer::ShowProperty(HWND hwndOwner)
{
	if (m_Renderer)
		return ShowPropertyPage(m_Renderer.Get(), hwndOwner);
	return false;
}


bool VideoRenderer::HasProperty()
{
	if (m_Renderer)
		return HasPropertyPage(m_Renderer.Get());
	return false;
}


VideoRenderer * VideoRenderer::CreateRenderer(RendererType Type)
{
	switch (Type) {
	case RendererType::Default:
		return new VideoRenderer_Default;

	case RendererType::VMR7:
		return new VideoRenderer_VMR7;

	case RendererType::VMR9:
		return new VideoRenderer_VMR9;

	case RendererType::VMR7Renderless:
		return new VideoRenderer_VMR7Renderless;

	case RendererType::VMR9Renderless:
		return new VideoRenderer_VMR9Renderless;

	case RendererType::EVR:
		return new VideoRenderer_EVR;

	case RendererType::OverlayMixer:
		return new VideoRenderer_OverlayMixer;

	case RendererType::madVR:
		return new VideoRenderer_madVR;

	case RendererType::EVRCustomPresenter:
		return new VideoRenderer_EVRCustomPresenter;

	case RendererType::MPCVideoRenderer:
		return new VideoRenderer_MPCVideoRenderer;
	}

	return nullptr;
}


LPCTSTR VideoRenderer::EnumRendererName(int Index)
{
	static const LPCTSTR pszRendererName[] = {
		TEXT("Default"),
		TEXT("VMR7"),
		TEXT("VMR9"),
		TEXT("VMR7 Renderless"),
		TEXT("VMR9 Renderless"),
		TEXT("EVR"),
		TEXT("Overlay Mixer"),
		TEXT("madVR"),
		TEXT("EVR Custom Presenter"),
		TEXT("MPC Video Renderer"),
	};

	if (static_cast<unsigned int>(Index) >= std::size(pszRendererName))
		return nullptr;

	return pszRendererName[Index];
}


VideoRenderer::RendererType VideoRenderer::ParseName(LPCTSTR pszName)
{
	LPCTSTR pszRenderer;

	for (int i = 0; (pszRenderer = EnumRendererName(i)) != nullptr; i++) {
		if (StringCompareI(pszName, pszRenderer) == 0)
			return static_cast<RendererType>(i);
	}

	return RendererType::Invalid;
}


static bool TestCreateInstance(REFCLSID clsid)
{
	HRESULT hr;
	IBaseFilter *pRenderer;

	hr = ::CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pRenderer));
	if (FAILED(hr))
		return false;
	pRenderer->Release();
	return true;
}

bool VideoRenderer::IsAvailable(RendererType Type)
{
	switch (Type) {
	case RendererType::Default:
		return true;

	case RendererType::VMR7:
	case RendererType::VMR7Renderless:
		return TestCreateInstance(CLSID_VideoMixingRenderer);

	case RendererType::VMR9:
	case RendererType::VMR9Renderless:
		return TestCreateInstance(CLSID_VideoMixingRenderer9);

	case RendererType::EVR:
	case RendererType::EVRCustomPresenter:
		return TestCreateInstance(CLSID_EnhancedVideoRenderer);

	case RendererType::OverlayMixer:
		return TestCreateInstance(CLSID_OverlayMixer);

	case RendererType::madVR:
		return TestCreateInstance(VideoRenderer_madVR::GetCLSID());

	case RendererType::MPCVideoRenderer:
		return TestCreateInstance(VideoRenderer_MPCVideoRenderer::GetCLSID());
	}

	return false;
}


}	// namespace LibISDB::DirectShow
