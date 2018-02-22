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
 @file   ImageMixer_EVR.cpp
 @brief  EVR 画像合成
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "ImageMixer_EVR.hpp"
#include <d3d9.h>
#include <evr.h>
#include <evr9.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


ImageMixer_EVR::ImageMixer_EVR(IBaseFilter *pRenderer)
	: ImageMixer_VMR(pRenderer)
{
}


ImageMixer_EVR::~ImageMixer_EVR()
{
}


void ImageMixer_EVR::Clear()
{
	if (m_hdc != nullptr) {
		IMFGetService *pGetService;

		if (SUCCEEDED(m_Renderer->QueryInterface(IID_PPV_ARGS(&pGetService)))) {
			IMFVideoMixerBitmap *pMixerBitmap;

			if (SUCCEEDED(pGetService->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pMixerBitmap)))) {
				pMixerBitmap->ClearAlphaBitmap();
				pMixerBitmap->Release();
			}
			pGetService->Release();
		}
	}
}


bool ImageMixer_EVR::SetBitmap(HBITMAP hbm, int Opacity, COLORREF TransColor, const RECT &DestRect)
{
	if (!CreateMemDC())
		return false;

	HRESULT hr;

	IMFGetService *pGetService;
	hr = m_Renderer.QueryInterface(&pGetService);
	if (FAILED(hr))
		return false;

	IMFVideoMixerBitmap *pMixerBitmap;
	hr = pGetService->GetService(MR_VIDEO_MIXER_SERVICE, IID_PPV_ARGS(&pMixerBitmap));
	if (SUCCEEDED(hr)) {
		IMFVideoDisplayControl *pDisplayControl;

		hr = pGetService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplayControl));
		if (SUCCEEDED(hr)) {
			SIZE NativeSize;

			hr = pDisplayControl->GetNativeVideoSize(&NativeSize, nullptr);
			if (SUCCEEDED(hr)) {
				BITMAP bm;
				MFVideoAlphaBitmap ab;

				::SelectObject(m_hdc, hbm);
				::GetObject(hbm, sizeof(BITMAP), &bm);

				ab.GetBitmapFromDC = TRUE;
				ab.bitmap.hdc = m_hdc;
				ab.params.dwFlags =
					MFVideoAlphaBitmap_SrcRect |
					MFVideoAlphaBitmap_DestRect |
					MFVideoAlphaBitmap_Alpha;
				if (TransColor != CLR_INVALID) {
					ab.params.dwFlags |= MFVideoAlphaBitmap_SrcColorKey;
					ab.params.clrSrcKey = TransColor;
				}
				::SetRect(&ab.params.rcSrc, 0, 0, bm.bmWidth, bm.bmHeight);
				ab.params.nrcDest.left = static_cast<float>(DestRect.left) / static_cast<float>(NativeSize.cx);
				ab.params.nrcDest.top = static_cast<float>(DestRect.top) / static_cast<float>(NativeSize.cy);
				ab.params.nrcDest.right = static_cast<float>(DestRect.right) / static_cast<float>(NativeSize.cx);
				ab.params.nrcDest.bottom = static_cast<float>(DestRect.bottom) / static_cast<float>(NativeSize.cy);
				ab.params.fAlpha = static_cast<float>(Opacity) / 100.0f;

				hr = pMixerBitmap->SetAlphaBitmap(&ab);

				if (FAILED(hr))
					::SelectObject(m_hdc, m_hbmOld);
			}

			pDisplayControl->Release();
		}

		pMixerBitmap->Release();
	}

	pGetService->Release();

	return SUCCEEDED(hr);
}


bool ImageMixer_EVR::GetMapSize(ReturnArg<int> Width, ReturnArg<int> Height)
{
	bool OK = false;
	IMFGetService *pGetService;

	if (SUCCEEDED(m_Renderer.QueryInterface(&pGetService))) {
		IMFVideoDisplayControl *pDisplayControl;

		if (SUCCEEDED(pGetService->GetService(MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&pDisplayControl)))) {
			SIZE Size;

			if (SUCCEEDED(pDisplayControl->GetNativeVideoSize(&Size, nullptr))) {
				Width = Size.cx;
				Height = Size.cy;
				OK = true;
			}

			pDisplayControl->Release();
		}

		pGetService->Release();
	}

	return OK;
}


}	// namespace LibISDB::DirectShow
