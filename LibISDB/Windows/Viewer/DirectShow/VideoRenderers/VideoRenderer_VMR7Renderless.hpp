/*
  LibISDB
  Copyright(c) 2017-2019 DBCTRADO

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
 @file   VideoRenderer_VMR7Renderless.hpp
 @brief  VMR-7 Renderless 映像レンダラ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_RENDERER_VMR7_RENDERLESS_H
#define LIBISDB_VIDEO_RENDERER_VMR7_RENDERLESS_H


#include "VideoRenderer.hpp"
#include <ddraw.h>
/*
#define D3D_OVERLOADS
#include <d3d.h>
*/


namespace LibISDB::DirectShow
{

	/** VMR-7 Renderless 映像レンダラ */
	class VideoRenderer_VMR7Renderless
		: public IVMRSurfaceAllocator
		, public IVMRSurfaceAllocatorNotify
		, public IVMRImagePresenter
		, public VideoRenderer
	{
	public:
		VideoRenderer_VMR7Renderless();

	// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
		STDMETHODIMP_(ULONG) AddRef() override;
		STDMETHODIMP_(ULONG) Release() override;

	// IVMRSurfaceAllocator
		STDMETHODIMP AllocateSurface(
			DWORD_PTR dwUserID, VMRALLOCATIONINFO *lpAllocInfo,
			DWORD *lpdwActualBackBuffers, LPDIRECTDRAWSURFACE7 *lplpSurface) override;
		STDMETHODIMP FreeSurface(DWORD_PTR dwUserID) override;
		STDMETHODIMP PrepareSurface(
			DWORD_PTR dwUserID, LPDIRECTDRAWSURFACE7 lpSurface,DWORD dwSurfaceFlags) override;
		STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify *lpIVMRSurfAllocNotify) override;

	// IVMRSurfaceAllocatorNotify
		STDMETHODIMP AdviseSurfaceAllocator(
			DWORD_PTR dwUserID, IVMRSurfaceAllocator *lpIVRMSurfaceAllocator) override;
		STDMETHODIMP SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor) override;
		STDMETHODIMP ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice, HMONITOR hMonitor) override;
		STDMETHODIMP RestoreDDrawSurfaces() override;
		STDMETHODIMP NotifyEvent(LONG EventCode, LONG_PTR lp1, LONG_PTR lp2) override;
		STDMETHODIMP SetBorderColor(COLORREF clr) override;

	// IVMRImagePresenter
		STDMETHODIMP StartPresenting(DWORD_PTR dwUserID) override;
		STDMETHODIMP StopPresenting(DWORD_PTR dwUserID) override;
		STDMETHODIMP PresentImage(DWORD_PTR dwUserID, VMRPRESENTATIONINFO* lpPresInfo) override;

	// VideoRenderer
		RendererType GetRendererType() const noexcept { return RendererType::VMR7Renderless; }
		bool Initialize(
			IGraphBuilder *pGraphBuilder, IPin *pInputPin,
			HWND hwndRender, HWND hwndMessageDrain) override;
		bool Finalize() override;
		bool SetVideoPosition(
			int SourceWidth, int SourceHeight, const RECT &SourceRect,
			const RECT &DestRect, const RECT &WindowRect) override;
		bool GetDestPosition(ReturnArg<RECT> Rect) override;
		COMMemoryPointer<> GetCurrentImage() override;
		bool RepaintVideo(HWND hwnd,HDC hdc) override;
		bool DisplayModeChanged() override;
		bool SetVisible(bool Visible) override;

	private:
		HRESULT CreateDefaultAllocatorPresenter(HWND hwndRender);
		HRESULT AddVideoMixingRendererToFG();
		HRESULT OnSetDDrawDevice(LPDIRECTDRAW7 pDD, HMONITOR hMon);
		void ReleaseSurfaces();

		LONG m_RefCount;

		COMPointer<IVMRImagePresenter> m_ImagePresenter;
		COMPointer<IVMRSurfaceAllocator> m_SurfaceAllocator;
		COMPointer<IVMRSurfaceAllocatorNotify> m_SurfaceAllocatorNotify;

		COMPointer<IDirectDrawSurface7> m_PrimarySurface;
		COMPointer<IDirectDrawSurface7> m_PriTextSurface;
		COMPointer<IDirectDrawSurface7> m_RenderTSurface;

		int m_Duration;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_RENDERER_VMR7_RENDERLESS_H
