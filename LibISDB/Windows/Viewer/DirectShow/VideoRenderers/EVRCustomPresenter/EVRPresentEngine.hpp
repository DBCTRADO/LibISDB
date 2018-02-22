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
 @file   EVRPresentEngine.hpp
 @brief  EVR プレゼントエンジン
 @author DBCTRADO
*/


#ifndef LIBISDB_EVR_PRESENT_ENGINE_H
#define LIBISDB_EVR_PRESENT_ENGINE_H


#include "EVRScheduler.hpp"


namespace LibISDB::DirectShow
{

	/** EVR プレゼントエンジンクラス */
	class EVRPresentEngine
		: public EVRSchedulerCallback
	{
	public:
		enum class DeviceState {
			OK,
			Reset,
			Removed,
		};

		EVRPresentEngine(HRESULT &hr);
		virtual ~EVRPresentEngine();

		virtual HRESULT GetService(REFGUID guidService, REFIID riid, void **ppv);
		virtual HRESULT CheckFormat(D3DFORMAT Format);

		HRESULT SetVideoWindow(HWND hwnd);
		HWND GetVideoWindow() const { return m_hwnd; }
		HRESULT SetDestinationRect(const RECT &rcDest);
		RECT GetDestinationRect() const { return m_rcDestRect; };

		HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList &videoSampleQueue);
		void ReleaseResources();

		HRESULT CheckDeviceState(DeviceState *pState);

	// EVRSchedulerCallback
		HRESULT PresentSample(IMFSample* pSample, LONGLONG llTarget) override;

		HRESULT GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp);

		UINT RefreshRate() const { return m_DisplayMode.RefreshRate; }

	protected:
		HRESULT InitializeD3D();
		HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS *pPP);
		HRESULT CreateD3DDevice();
		HRESULT CreateD3DSample(IDirect3DSwapChain9 *pSwapChain, IMFSample **ppVideoSample);
		void InitPresentParameters(
			D3DPRESENT_PARAMETERS *pParameters,
			HWND hwnd, UINT Width, UINT Height, D3DFORMAT Format);
		HRESULT UpdateDestRect();
		HRESULT GetDibFromSurface(
			IDirect3DSurface9 *pSurface, const D3DSURFACE_DESC &Desc,
			BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib);

		virtual HRESULT OnCreateVideoSamples(D3DPRESENT_PARAMETERS &pp) { return S_OK; }
		virtual void OnReleaseResources() {}

		virtual HRESULT PresentSwapChain(IDirect3DSwapChain9 *pSwapChain, IDirect3DSurface9 *pSurface);
		virtual void PaintFrameWithGDI();

		static constexpr int PRESENTER_BUFFER_COUNT = 3;

		UINT m_DeviceResetToken;

		HWND m_hwnd;
		RECT m_rcDestRect;
		D3DDISPLAYMODE m_DisplayMode;

		MutexLock m_ObjectLock;

		COMPointer<IDirect3D9Ex> m_D3D9;
		COMPointer<IDirect3DDevice9Ex> m_Device;
		COMPointer<IDirect3DDeviceManager9> m_DeviceManager;
		COMPointer<IDirect3DSurface9> m_SurfaceRepaint;

		LONGLONG m_LastPresentTime;
		MutexLock m_RepaintSurfaceLock;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_EVR_PRESENT_ENGINE_H
