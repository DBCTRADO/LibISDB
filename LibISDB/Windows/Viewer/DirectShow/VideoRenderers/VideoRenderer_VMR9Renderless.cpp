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
 @file   VideoRenderer_VMR9Renderless.cpp
 @brief  VMR-9 Renderless 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_VMR9Renderless.hpp"
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>
#include <shlwapi.h>
#include <vector>
#include "../DirectShowUtilities.hpp"
#include "../../../../Utilities/Lock.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


class VMR9Allocator
	: public IVMRSurfaceAllocator9
	, public IVMRImagePresenter9
{
public:
	VMR9Allocator(HRESULT *phr, HWND wnd, IDirect3D9 *d3d = nullptr, IDirect3DDevice9 *d3dd = nullptr);
	virtual ~VMR9Allocator();

// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;

// IVMRSurfaceAllocator9
	STDMETHODIMP InitializeDevice(
		DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers) override;
	STDMETHODIMP TerminateDevice(DWORD_PTR dwID) override;
	STDMETHODIMP GetSurface(
		DWORD_PTR dwUserID, DWORD SurfaceIndex, DWORD SurfaceFlags,
		IDirect3DSurface9 **lplpSurface) override;
	STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify) override;

// IVMRImagePresenter9
	STDMETHODIMP StartPresenting(DWORD_PTR dwUserID) override;
	STDMETHODIMP StopPresenting(DWORD_PTR dwUserID) override;
	STDMETHODIMP PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo) override;

// VMR9Allocator
	//bool GetNativeVideoSize(LONG *pWidth, LONG *pHeight);
	bool SetVideoPosition(
		int SourceWidth, int SourceHeight, const RECT *pSourceRect, const RECT *pDestRect, const RECT &WindowRect);
	bool GetVideoPosition(RECT *pSrc, RECT *pDst);
	bool RepaintVideo();
	bool SetCapture(bool fCapture);
	bool WaitCapture(DWORD TimeOut);
	bool GetCaptureSurface(IDirect3DSurface9 **ppSurface);
	void SetCrop1088To1080(bool Crop) { m_Crop1088To1080 = Crop; }

protected:
	HRESULT CreateDevice();
	void DeleteSurfaces();

	bool NeedToHandleDisplayChange();

	HRESULT PresentHelper(VMR9PresentationInfo *lpPresInfo);
	void CalcTransferRect(int SurfaceWidth, int SurfaceHeight, RECT *pSourceRect, RECT *pDestRect) const;

private:
	MutexLock m_ObjectLock;
	LONG m_RefCount;
	HWND m_window;
	SIZE m_WindowSize;
	SIZE m_SourceSize;
	SIZE m_NativeVideoSize;
	RECT m_SourceRect;
	RECT m_DestRect;
	bool m_Crop1088To1080;

	HMODULE m_hD3D9Lib;
	COMPointer<IDirect3D9> m_D3D;
	COMPointer<IDirect3DDevice9> m_D3DDev;
	COMPointer<IVMRSurfaceAllocatorNotify9> m_SurfaceAllocatorNotify;
	std::vector<COMPointer<IDirect3DSurface9>> m_Surfaces;
	//COMPointer<IDirect3DSurface9> m_RenderTarget;
	//COMPointer<IDirect3DTexture9> m_PrivateTexture;

	HANDLE m_hCaptureEvent;
	HANDLE m_hCaptureCompleteEvent;
	COMPointer<IDirect3DSurface9> m_CaptureSurface;
};


VMR9Allocator::VMR9Allocator(HRESULT *phr, HWND wnd, IDirect3D9 *d3d, IDirect3DDevice9 *d3dd)
	: m_RefCount(1)
	, m_D3D(d3d)
	, m_D3DDev(d3dd)
	, m_window(wnd)

	, m_SourceSize()
	, m_NativeVideoSize()
	, m_SourceRect()
	, m_DestRect()

	, m_Crop1088To1080(true)
{
	RECT rc;
	::GetClientRect(wnd, &rc);
	m_WindowSize.cx = rc.right;
	m_WindowSize.cy = rc.bottom;

	m_hCaptureEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_hCaptureCompleteEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

	*phr = S_OK;

	TCHAR szPath[MAX_PATH];
	::GetSystemDirectory(szPath, _countof(szPath));
	::PathAppend(szPath, TEXT("d3d9.dll"));
	m_hD3D9Lib = ::LoadLibrary(szPath);
	if (m_hD3D9Lib == nullptr) {
		*phr = E_FAIL;
		return;
	}

	if (!m_D3D) {
		auto pDirect3DCreate9 =
			reinterpret_cast<decltype(Direct3DCreate9) *>(::GetProcAddress(m_hD3D9Lib, "Direct3DCreate9"));
		if (pDirect3DCreate9 == nullptr) {
			*phr = E_FAIL;
			return;
		}

		m_D3D.Attach((*pDirect3DCreate9)(D3D_SDK_VERSION));
		if (!m_D3D) {
			*phr = E_FAIL;
			return;
		}
	}

	if (!m_D3DDev)
		*phr = CreateDevice();
}


VMR9Allocator::~VMR9Allocator()
{
	DeleteSurfaces();
	m_SurfaceAllocatorNotify.Release();
	//m_RenderTarget.Release();
	m_D3DDev.Release();
	m_D3D.Release();

	::CloseHandle(m_hCaptureEvent);
	::CloseHandle(m_hCaptureCompleteEvent);

	if (m_hD3D9Lib)
		::FreeLibrary(m_hD3D9Lib);
}


STDMETHODIMP VMR9Allocator::QueryInterface(REFIID riid, void **ppvObject)
{
	if (ppvObject == nullptr)
		return E_POINTER;

	if (riid == IID_IVMRSurfaceAllocator9) {
		*ppvObject = static_cast<IVMRSurfaceAllocator9 *>(this);
	} else if (riid == IID_IVMRImagePresenter9) {
		*ppvObject = static_cast<IVMRImagePresenter9 *>(this);
	} else if (riid == IID_IUnknown) {
		*ppvObject = static_cast<IUnknown *>(static_cast<IVMRSurfaceAllocator9 *>(this));
	} else {
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


STDMETHODIMP_(ULONG) VMR9Allocator::AddRef()
{
	return ::InterlockedIncrement(&m_RefCount);
}


STDMETHODIMP_(ULONG) VMR9Allocator::Release()
{
	LONG Result = ::InterlockedDecrement(&m_RefCount);
	if (Result == 0)
		delete this;
	return Result;
}


HRESULT VMR9Allocator::CreateDevice()
{
	HRESULT hr;

	m_D3DDev.Release();

	D3DDISPLAYMODE dm;
	hr = m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm);
	if (FAILED(hr))
		return hr;

	D3DPRESENT_PARAMETERS pp = {};
	pp.BackBufferWidth = 1920;
	pp.BackBufferHeight = 1080;
	pp.BackBufferFormat = dm.Format;
	pp.SwapEffect = D3DSWAPEFFECT_COPY;
	pp.hDeviceWindow = m_window;
	pp.Windowed = TRUE;
	//pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

	hr = m_D3D->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		m_window,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
		&pp,
		m_D3DDev.GetPP());
	if (FAILED(hr)) {
		hr = m_D3D->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_REF,
			m_window,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
			&pp,
			m_D3DDev.GetPP());
		if (FAILED(hr))
			return hr;
	}

	//m_RenderTarget.Release();
	//hr = m_D3DDev->GetRenderTarget(0, m_RenderTarget.GetPP());

	return hr;
}


void VMR9Allocator::DeleteSurfaces()
{
	BlockLock Lock(m_ObjectLock);

	//m_PrivateTexture.Release();

	m_Surfaces.clear();

	m_CaptureSurface.Release();
}


STDMETHODIMP VMR9Allocator::InitializeDevice(
	DWORD_PTR dwUserID, VMR9AllocationInfo *lpAllocInfo, DWORD *lpNumBuffers)
{
	if (lpNumBuffers == nullptr)
		return E_POINTER;
	if (!m_SurfaceAllocatorNotify)
		return E_FAIL;

	LIBISDB_TRACE(
		LIBISDB_STR("CVMRAllocator::InitializeDevice() : %lu x %lu (%lu buffers)\n"),
		(unsigned long)lpAllocInfo->dwWidth,
		(unsigned long)lpAllocInfo->dwHeight,
		(unsigned long)*lpNumBuffers);

	HRESULT hr;

	hr = m_SurfaceAllocatorNotify->SetD3DDevice(
		m_D3DDev.Get(),
		::MonitorFromWindow(m_window, MONITOR_DEFAULTTOPRIMARY));
	if (FAILED(hr))
		return hr;

#if 0	// テクスチャ・サーフェスを作成する場合
	D3DCAPS9 d3dcaps;
	m_D3DDev->GetDeviceCaps(&d3dcaps);
	if (d3dcaps.TextureCaps & D3DPTEXTURECAPS_POW2) {
		DWORD Width = 1, Height = 1;

		while (Width < lpAllocInfo->dwWidth)
			Width <<= 1;
		while (Height < lpAllocInfo->dwHeight)
			Height <<= 1;
		lpAllocInfo->dwWidth = Width;
		lpAllocInfo->dwHeight = Height;
	}
	lpAllocInfo->dwFlags |= VMR9AllocFlag_TextureSurface;
#endif

	DeleteSurfaces();
	m_Surfaces.resize(*lpNumBuffers);
	hr = m_SurfaceAllocatorNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, m_Surfaces[0].GetPP());

	/*
	if (FAILED(hr) && !(lpAllocInfo->dwFlags & VMR9AllocFlag_3DRenderTarget)) {
		DeleteSurfaces();

		// is surface YUV ?
		if (lpAllocInfo->Format > '0000') {
			D3DDISPLAYMODE dm;

			hr = m_D3DDev->GetDisplayMode(nullptr, &dm);
			if (FAILED(hr))
				return hr;

			// create the private texture
			hr = m_D3DDev->CreateTexture(
				lpAllocInfo->dwWidth, lpAllocInfo->dwHeight,
				1,
				D3DUSAGE_RENDERTARGET,
				dm.Format,
				D3DPOOL_DEFAULT,
				m_PrivateTexture.GetPP(),
				nullptr);
			if (FAILED(hr))
				return hr;
		}

		lpAllocInfo->dwFlags &= ~VMR9AllocFlag_TextureSurface;
		lpAllocInfo->dwFlags |= VMR9AllocFlag_OffscreenSurface;

		hr = m_SurfaceAllocatorNotify->AllocateSurfaceHelper(lpAllocInfo, lpNumBuffers, &m_Surfaces[0]);
		if (FAILED(hr))
			return hr;
	}
	*/

	return hr;
}


STDMETHODIMP VMR9Allocator::TerminateDevice(DWORD_PTR dwID)
{
	DeleteSurfaces();
	return S_OK;
}


STDMETHODIMP VMR9Allocator::GetSurface(
	DWORD_PTR dwUserID, DWORD SurfaceIndex,
	DWORD SurfaceFlags, IDirect3DSurface9 **lplpSurface)
{
	if (lplpSurface == nullptr)
		return E_POINTER;

	if (SurfaceIndex >= m_Surfaces.size())
		return E_INVALIDARG;

	BlockLock Lock(m_ObjectLock);

	IDirect3DSurface9 *pSurface = m_Surfaces[SurfaceIndex].Get();
	if (pSurface == nullptr)
		return E_INVALIDARG;

	*lplpSurface = pSurface;
	pSurface->AddRef();

	return S_OK;
}


STDMETHODIMP VMR9Allocator::AdviseNotify(IVMRSurfaceAllocatorNotify9 *lpIVMRSurfAllocNotify)
{
	BlockLock Lock(m_ObjectLock);
	HRESULT hr;

	m_SurfaceAllocatorNotify = lpIVMRSurfAllocNotify;

	//HMONITOR hMonitor = m_D3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
	HMONITOR hMonitor = ::MonitorFromWindow(m_window, MONITOR_DEFAULTTOPRIMARY);
	hr = m_SurfaceAllocatorNotify->SetD3DDevice(m_D3DDev.Get(), hMonitor);

	return hr;
}


STDMETHODIMP VMR9Allocator::StartPresenting(DWORD_PTR dwUserID)
{
	BlockLock Lock(m_ObjectLock);

	if (!m_D3DDev)
		return E_FAIL;

	return S_OK;
}


STDMETHODIMP VMR9Allocator::StopPresenting(DWORD_PTR dwUserID)
{
	return S_OK;
}


STDMETHODIMP VMR9Allocator::PresentImage(DWORD_PTR dwUserID, VMR9PresentationInfo *lpPresInfo)
{
	BlockLock Lock(m_ObjectLock);
	HRESULT hr;

	if (NeedToHandleDisplayChange()) {
		// NOTE: this piece of code is left as a user exercise.
		// The D3DDevice here needs to be switched
		// to the device that is using another adapter
	}

	hr = PresentHelper(lpPresInfo);

	if (hr == D3DERR_DEVICELOST) {
		if (m_D3DDev->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
			DeleteSurfaces();
			hr = CreateDevice();
			if (FAILED(hr))
				return hr;

			//HMONITOR hMonitor = m_D3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);
			HMONITOR hMonitor = ::MonitorFromWindow(m_window, MONITOR_DEFAULTTOPRIMARY);

			hr = m_SurfaceAllocatorNotify->ChangeD3DDevice(m_D3DDev.Get(), hMonitor);
			if (FAILED(hr))
				return hr;
		}
		hr = S_OK;
	}

	return hr;
}


HRESULT VMR9Allocator::PresentHelper(VMR9PresentationInfo *lpPresInfo)
{
	if (lpPresInfo == nullptr || lpPresInfo->lpSurf == nullptr)
		return E_POINTER;

	HRESULT hr;
	D3DSURFACE_DESC desc;

	hr = lpPresInfo->lpSurf->GetDesc(&desc);
	if (FAILED(hr))
		return hr;

	m_NativeVideoSize.cx = desc.Width;
	m_NativeVideoSize.cy = desc.Height;

	if (::WaitForSingleObject(m_hCaptureEvent, 0) == WAIT_OBJECT_0) {
		m_CaptureSurface.Release();
		hr = m_D3DDev->CreateOffscreenPlainSurface(
			desc.Width, desc.Height, D3DFMT_X8R8G8B8,//desc.Format,
			D3DPOOL_DEFAULT, m_CaptureSurface.GetPP(), nullptr);
		if (SUCCEEDED(hr)) {
			hr = m_D3DDev->StretchRect(lpPresInfo->lpSurf, nullptr, m_CaptureSurface.Get(), nullptr, D3DTEXF_NONE);
			if (SUCCEEDED(hr)) {
				::SetEvent(m_hCaptureCompleteEvent);
			} else {
				m_CaptureSurface.Release();
			}
		}
	}

	//m_D3DDev->SetRenderTarget(0, m_RenderTarget.Get());

	IDirect3DSurface9 *pDstSurface;

	hr = m_D3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pDstSurface);
	if (SUCCEEDED(hr)) {
		hr = m_D3DDev->BeginScene();
		if (SUCCEEDED(hr)) {
			pDstSurface->GetDesc(&desc);
			RECT rcSource, rcDest;
			CalcTransferRect(desc.Width, desc.Height, &rcSource, &rcDest);
			m_D3DDev->StretchRect(lpPresInfo->lpSurf, nullptr, pDstSurface, nullptr, D3DTEXF_NONE);
			hr = m_D3DDev->EndScene();
			if (SUCCEEDED(hr)) {
				hr = m_D3DDev->Present(&rcSource, &rcDest, nullptr, nullptr);
			}
		}
		pDstSurface->Release();
	}

	return hr;
}


void VMR9Allocator::CalcTransferRect(int SurfaceWidth, int SurfaceHeight, RECT *pSourceRect, RECT *pDestRect) const
{
	if (!::IsRectEmpty(&m_SourceRect)) {
		*pSourceRect = m_SourceRect;
		if (m_SourceSize.cx > 0 && m_SourceSize.cy > 0) {
			*pSourceRect = MapRect(
				*pSourceRect,
				m_NativeVideoSize.cx, m_SourceSize.cx,
				m_NativeVideoSize.cy, m_SourceSize.cy);
		}
		*pSourceRect = MapRect(
			*pSourceRect,
			SurfaceWidth, m_NativeVideoSize.cx,
			SurfaceHeight, m_NativeVideoSize.cy);
	} else {
		::SetRect(pSourceRect, 0, 0, SurfaceWidth, SurfaceHeight);
	}
	if (m_Crop1088To1080 && m_NativeVideoSize.cy == 1088) {
		pSourceRect->top    = ::MulDiv(pSourceRect->top,    1080, 1088);
		pSourceRect->bottom = ::MulDiv(pSourceRect->bottom, 1080, 1088);
	}
	if (!::IsRectEmpty(&m_DestRect)) {
		*pDestRect = m_DestRect;
	} else {
		::SetRect(pDestRect, 0, 0, m_WindowSize.cx, m_WindowSize.cy);
	}
}


bool VMR9Allocator::NeedToHandleDisplayChange()
{
	if (!m_SurfaceAllocatorNotify)
		return false;

	D3DDEVICE_CREATION_PARAMETERS Parameters;
	if (FAILED(m_D3DDev->GetCreationParameters(&Parameters)))
		return false;

	HMONITOR hCurrentMonitor = m_D3D->GetAdapterMonitor(Parameters.AdapterOrdinal);

	HMONITOR hMonitor = m_D3D->GetAdapterMonitor(D3DADAPTER_DEFAULT);

	return hMonitor != hCurrentMonitor;
}


bool VMR9Allocator::RepaintVideo()
{
	BlockLock Lock(m_ObjectLock);
	HRESULT hr;
	IDirect3DSurface9 *pDstSurface;
	D3DSURFACE_DESC desc;
	RECT rcSource, rcDest;

	if (m_NativeVideoSize.cx == 0 || m_NativeVideoSize.cy == 0)
		return false;
	hr = m_D3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pDstSurface);
	if (FAILED(hr))
		return false;
	pDstSurface->GetDesc(&desc);
	CalcTransferRect(desc.Width, desc.Height, &rcSource, &rcDest);
	hr = m_D3DDev->Present(&rcSource, &rcDest, nullptr, nullptr);
	pDstSurface->Release();
	return SUCCEEDED(hr);
}


bool VMR9Allocator::SetCapture(bool fCapture)
{
	::ResetEvent(m_hCaptureCompleteEvent);
	if (fCapture) {
		m_CaptureSurface.Release();
		::SetEvent(m_hCaptureEvent);
	} else {
		::ResetEvent(m_hCaptureEvent);
	}
	return true;
}


bool VMR9Allocator::WaitCapture(DWORD TimeOut)
{
	return ::WaitForSingleObject(m_hCaptureCompleteEvent, TimeOut) == WAIT_OBJECT_0;
}


bool VMR9Allocator::GetCaptureSurface(IDirect3DSurface9 **ppSurface)
{
	return m_CaptureSurface.QueryInterface(ppSurface);
}


/*
bool VMR9Allocator::GetNativeVideoSize(LONG *pWidth, LONG *pHeight)
{
	BlockLock Lock(m_ObjectLock);

	if (m_NativeVideoSize.cx == 0 || m_NativeVideoSize.cy == 0)
		return false;
	if (pWidth)
		*pWidth = m_NativeVideoSize.cx;
	if (pHeight)
		*pHeight = m_NativeVideoSize.cy;
	return true;
}
*/


bool VMR9Allocator::SetVideoPosition(
	int SourceWidth, int SourceHeight, const RECT *pSourceRect,
	const RECT *pDestRect, const RECT &WindowRect)
{
	BlockLock Lock(m_ObjectLock);

	m_SourceSize.cx = SourceWidth;
	m_SourceSize.cy = SourceHeight;

	if (pSourceRect) {
		m_SourceRect = *pSourceRect;
	} else {
		::SetRectEmpty(&m_SourceRect);
	}
	if (pDestRect) {
		m_DestRect = *pDestRect;
	} else {
		::SetRectEmpty(&m_DestRect);
	}

	if (WindowRect.right - WindowRect.left != m_WindowSize.cx
			|| WindowRect.bottom - WindowRect.top != m_WindowSize.cy) {
		/*
		if (m_D3DDev && m_SurfaceAllocatorNotify) {
			HRESULT hr;
			D3DDISPLAYMODE dm;

			DeleteSurfaces();
			hr = m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm);
			D3DPRESENT_PARAMETERS pp = {};
			pp.BackBufferWidth = rc.right;
			pp.BackBufferHeight = rc.bottom;
			pp.BackBufferFormat = dm.Format;
			pp.SwapEffect = D3DSWAPEFFECT_COPY;
			pp.hDeviceWindow = m_window;
			pp.Windowed = TRUE;
			hr = m_D3DDev->Reset(&pp);
			if (SUCCEEDED(hr)) {
				HMONITOR hMonitor = ::MonitorFromWindow(m_window, MONITOR_DEFAULTTOPRIMARY);
				hr = m_SurfaceAllocatorNotify->ChangeD3DDevice(m_D3DDev.Get(), hMonitor);
				if (FAILED(hr))
					return false;
			}
		}
		*/

		m_WindowSize.cx = WindowRect.right - WindowRect.left;
		m_WindowSize.cy = WindowRect.bottom - WindowRect.top;
	}

	return true;
}


bool VMR9Allocator::GetVideoPosition(RECT *pSrc, RECT *pDst)
{
	BlockLock Lock(m_ObjectLock);

	if (pSrc) {
		if (m_NativeVideoSize.cx == 0 || m_NativeVideoSize.cy == 0)
			return false;
		if (!::IsRectEmpty(&m_SourceRect)) {
			*pSrc = m_SourceRect;
		} else {
			pSrc->left = 0;
			pSrc->top = 0;
			pSrc->right = m_NativeVideoSize.cx;
			pSrc->bottom = m_NativeVideoSize.cy;
		}
	}

	if (pDst) {
		if (m_NativeVideoSize.cx == 0 || m_NativeVideoSize.cy == 0)
			return false;
		if (!::IsRectEmpty(&m_DestRect)) {
			*pDst = m_DestRect;
		} else {
			pDst->left = 0;
			pDst->top = 0;
			pDst->right = m_WindowSize.cx;
			pDst->bottom = m_WindowSize.cy;
		}
	}

	return true;
}




VideoRenderer_VMR9Renderless::VideoRenderer_VMR9Renderless()
	: m_pAllocator(nullptr)
{
}


VideoRenderer_VMR9Renderless::~VideoRenderer_VMR9Renderless()
{
	SafeRelease(m_pAllocator);
}


bool VideoRenderer_VMR9Renderless::Initialize(
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
		SetHRESULTError(hr, LIBISDB_STR("VMR-9のインスタンスを作成できません。"));
		return false;
	}
	hr = pGraphBuilder->AddFilter(m_Renderer.Get(), L"VMR9");
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("VMR-9をフィルタグラフに追加できません。"));
		return false;
	}

	IVMRFilterConfig9 *pFilterConfig;
	hr = m_Renderer.QueryInterface(&pFilterConfig);
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("IVMRFilterConfig9 を取得できません。"));
		return false;
	}
	pFilterConfig->SetRenderingMode(VMR9Mode_Renderless);
	pFilterConfig->Release();

	IVMRSurfaceAllocatorNotify9 *pSurfaceAllocatorNotify;
	hr = m_Renderer.QueryInterface(&pSurfaceAllocatorNotify);
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("IVMRSurfaceAllocatorNotify9を取得できません。"));
		return false;
	}
	m_pAllocator = new VMR9Allocator(&hr, hwndRender);
	m_pAllocator->SetCrop1088To1080(m_Crop1088To1080);
	pSurfaceAllocatorNotify->AdviseSurfaceAllocator(12345, m_pAllocator);
	m_pAllocator->AdviseNotify(pSurfaceAllocatorNotify);
	pSurfaceAllocatorNotify->Release();

	IFilterGraph2 *pFilterGraph2;
	hr = pGraphBuilder->QueryInterface(IID_PPV_ARGS(&pFilterGraph2));
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


bool VideoRenderer_VMR9Renderless::Finalize()
{
	SafeRelease(m_pAllocator);

	VideoRenderer::Finalize();

	return true;
}


bool VideoRenderer_VMR9Renderless::SetVideoPosition(
	int SourceWidth, int SourceHeight,const RECT &SourceRect,
	const RECT &DestRect, const RECT &WindowRect)
{
	if (!m_Renderer || !m_pAllocator)
		return false;

	RECT rcDest;

	rcDest = DestRect;
	::OffsetRect(&rcDest, WindowRect.left, WindowRect.top);
	m_pAllocator->SetVideoPosition(SourceWidth, SourceHeight, &SourceRect, &rcDest, WindowRect);
	::InvalidateRect(m_hwndRender, nullptr, TRUE);

	return true;
}


bool VideoRenderer_VMR9Renderless::GetDestPosition(ReturnArg<RECT> Rect)
{
	if (!m_Renderer || !m_pAllocator || !Rect)
		return false;

	return m_pAllocator->GetVideoPosition(nullptr, &*Rect);
}


COMMemoryPointer<> VideoRenderer_VMR9Renderless::GetCurrentImage()
{
	BYTE *pDib = nullptr;

	if (m_Renderer && m_pAllocator) {
		if (m_pAllocator->SetCapture(true)) {
			if (m_pAllocator->WaitCapture(1000)) {
				IDirect3DSurface9 *pSurface;

				if (m_pAllocator->GetCaptureSurface(&pSurface)) {
					D3DSURFACE_DESC desc;

					pSurface->GetDesc(&desc);

					int Height = desc.Height;
					if (m_Crop1088To1080 && (Height == 1088))
						Height = 1080;

					BITMAPINFO bmi;

					::ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
					bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bmi.bmiHeader.biWidth = desc.Width;
					bmi.bmiHeader.biHeight = Height;
					bmi.bmiHeader.biPlanes = 1;
					bmi.bmiHeader.biBitCount = 24;

					void *pBits;
					HBITMAP hbm = ::CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

					if (hbm != nullptr) {
						HRESULT hr;
#if 1
						// DCを取得して転送する
						HDC hdcSurface;

						hr = pSurface->GetDC(&hdcSurface);
						if (SUCCEEDED(hr)) {
							HDC hdcMem = ::CreateCompatibleDC(hdcSurface);
							if (hdcMem != nullptr) {
								HBITMAP hbmOld = static_cast<HBITMAP>(::SelectObject(hdcMem, hbm));
								::BitBlt(hdcMem, 0, 0, desc.Width, Height, hdcSurface, 0, 0, SRCCOPY);
								::SelectObject(hdcMem, hbmOld);
								::DeleteDC(hdcMem);
							}
							pSurface->ReleaseDC(hdcSurface);
#else
						// サーフェスをロックして転送する
						D3DLOCKED_RECT rect;

						hr = pSurface->LockRect(&rect,nullptr,0);
						if (SUCCEEDED(hr)) {
							BYTE *p = static_cast<BYTE *>(rect.pBits), *q;
							const size_t DIBRowBytes = (desc.Width * 3 + 3) / 4 * 4;

							for (int y = 0; y < Height; y++) {
								q = static_cast<BYTE *>(pBits) + (Height - 1 - y) * DIBRowBytes;
								for (DWORD x = 0; x < desc.Width; x++) {
									q[0] = p[3];
									q[1] = p[2];
									q[2] = p[1];
									p += 4;
									q += 3;
								}
								p += rect.Pitch - desc.Width * 4;
							}
							pSurface->UnlockRect();
#endif

							size_t BitsSize = (desc.Width * 3 + 3) / 4 * 4 * Height;
							pDib = static_cast<BYTE *>(::CoTaskMemAlloc(sizeof(BITMAPINFOHEADER) + BitsSize));
							if (pDib != nullptr) {
								::CopyMemory(pDib, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
								::CopyMemory(pDib + sizeof(BITMAPINFOHEADER), pBits, BitsSize);
							}
						}

						::DeleteObject(hbm);
					}
				}

				pSurface->Release();
			}

			m_pAllocator->SetCapture(false);
		}
	}

	return COMMemoryPointer<>(pDib);
}


bool VideoRenderer_VMR9Renderless::RepaintVideo(HWND hwnd,HDC hdc)
{
	if (!m_Renderer || !m_pAllocator)
		return false;
	return m_pAllocator->RepaintVideo();
}


bool VideoRenderer_VMR9Renderless::DisplayModeChanged()
{
	// 未実装
	return false;
}


bool VideoRenderer_VMR9Renderless::SetVisible(bool Visible)
{
	// ウィンドウを再描画させる
	if (m_hwndRender != nullptr)
		return ::InvalidateRect(m_hwndRender, nullptr, TRUE) != FALSE;
	return false;
}


}	// namespace LibISDB::DirectShow
