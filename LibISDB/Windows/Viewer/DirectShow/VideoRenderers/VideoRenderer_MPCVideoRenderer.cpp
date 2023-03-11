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
 @file   VideoRenderer_MPCVideoRenderer.cpp
 @brief  MPC Video Renderer 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_MPCVideoRenderer.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


// {71F080AA-8661-4093-B15E-4F6903E77D0A}
static const CLSID CLSID_MPCVideoRenderer = {0x71F080AA, 0x8661, 0x4093, {0xB1, 0x5E, 0x4F, 0x69, 0x03, 0xE7, 0x7D, 0x0A}};

static const LPCTSTR MPCVR_VIDEO_WINDOW_CLASS = TEXT("LibISDB MPCVR Video Window");


VideoRenderer_MPCVideoRenderer::VideoRenderer_MPCVideoRenderer()
	: VideoRenderer_Basic(CLSID_MPCVideoRenderer, LIBISDB_STR("MPC Video Renderer"), true)
{
}


bool VideoRenderer_MPCVideoRenderer::Initialize(
	IGraphBuilder *pGraphBuilder, IPin *pInputPin, HWND hwndRender, HWND hwndMessageDrain)
{
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
		wc.lpszClassName = MPCVR_VIDEO_WINDOW_CLASS;
		if (::RegisterClass(&wc) == 0) {
			SetWin32Error(::GetLastError(), LIBISDB_STR("MPCVRウィンドウクラスを登録できません。"));
			return false;
		}
		Registered = true;
	}

	m_hwndVideo = ::CreateWindowEx(
		0, MPCVR_VIDEO_WINDOW_CLASS, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 0, 0,
		hwndRender, nullptr, hinst, this);
	if (m_hwndVideo == nullptr) {
		SetWin32Error(::GetLastError(), LIBISDB_STR("MPCVRウィンドウを作成できません。"));
		return false;
	}

	if (!VideoRenderer_Basic::Initialize(pGraphBuilder, pInputPin, m_hwndVideo, m_hwndVideo)) {
		::DestroyWindow(m_hwndVideo);
		m_hwndVideo = nullptr;
		return false;
	}

	m_hwndRender = hwndRender;
	m_hwndMessageDrain = hwndMessageDrain;

	return true;
}


bool VideoRenderer_MPCVideoRenderer::Finalize()
{
	VideoRenderer_Basic::Finalize();

	if (m_hwndVideo != nullptr) {
		::DestroyWindow(m_hwndVideo);
		m_hwndVideo = nullptr;
	}

	return true;
}


bool VideoRenderer_MPCVideoRenderer::SetVideoPosition(
	int SourceWidth, int SourceHeight, const RECT &SourceRect,
	const RECT &DestRect, const RECT &WindowRect)
{
	if (m_hwndVideo == nullptr)
		return false;

	RECT rcDest = DestRect;
	::OffsetRect(&rcDest, WindowRect.left, WindowRect.top);

	::SetWindowPos(
		m_hwndVideo, HWND_BOTTOM,
		rcDest.left, rcDest.top,
		rcDest.right - rcDest.left, rcDest.bottom - rcDest.top,
		SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS | SWP_NOREDRAW);

	::OffsetRect(&rcDest, -rcDest.left, -rcDest.top);

	return VideoRenderer_Basic::SetVideoPosition(SourceWidth, SourceHeight, SourceRect, rcDest, rcDest);
}


bool VideoRenderer_MPCVideoRenderer::GetDestPosition(ReturnArg<RECT> Rect)
{
	if (!Rect)
		return false;

	if (m_hwndVideo != nullptr) {
		if (::GetWindowRect(m_hwndVideo, &*Rect)) {
			::MapWindowRect(nullptr, m_hwndRender, &*Rect);
			return true;
		}
	}

	::SetRectEmpty(&*Rect);

	return false;
}


bool VideoRenderer_MPCVideoRenderer::ShowCursor(bool Show)
{
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

	return true;
}


bool VideoRenderer_MPCVideoRenderer::SetVisible(bool Visible)
{
	if (m_hwndVideo == nullptr)
		return false;
	return ::ShowWindow(m_hwndVideo, Visible ? SW_SHOW : SW_HIDE) != FALSE;
}


const CLSID & VideoRenderer_MPCVideoRenderer::GetCLSID()
{
	return CLSID_MPCVideoRenderer;
}


VideoRenderer_MPCVideoRenderer * VideoRenderer_MPCVideoRenderer::GetThis(HWND hwnd)
{
	return reinterpret_cast<VideoRenderer_MPCVideoRenderer *>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CALLBACK VideoRenderer_MPCVideoRenderer::VideoWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			VideoRenderer_MPCVideoRenderer *pThis = static_cast<VideoRenderer_MPCVideoRenderer *>(
				reinterpret_cast<const CREATESTRUCT *>(lParam)->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
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
			VideoRenderer_MPCVideoRenderer *pThis = GetThis(hwnd);

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
			VideoRenderer_MPCVideoRenderer *pThis = GetThis(hwnd);

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
			VideoRenderer_MPCVideoRenderer *pThis = GetThis(hwnd);

			::SetCursor(pThis->m_ShowCursor ? ::LoadCursor(nullptr, IDC_ARROW) : nullptr);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			VideoRenderer_MPCVideoRenderer *pThis = GetThis(hwnd);

			pThis->m_hwndVideo = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


}	// namespace LibISDB::DirectShow
