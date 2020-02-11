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
 @file   ImageMixer_VMR7.hpp
 @brief  VMR-7 画像合成
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "ImageMixer_VMR7.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


ImageMixer_VMR7::ImageMixer_VMR7(IBaseFilter *pRenderer) noexcept
	: ImageMixer_VMR(pRenderer)
{
}


ImageMixer_VMR7::~ImageMixer_VMR7()
{
}


void ImageMixer_VMR7::Clear()
{
	if (m_hdc!=nullptr) {
		IVMRMixerBitmap *pMixerBitmap;

		if (SUCCEEDED(m_Renderer.QueryInterface(&pMixerBitmap))) {
			VMRALPHABITMAP ab;

			ab.dwFlags = VMRBITMAP_DISABLE;
			ab.fAlpha = 0.0f;
			pMixerBitmap->UpdateAlphaBitmapParameters(&ab);
			pMixerBitmap->Release();
		}
	}
}


bool ImageMixer_VMR7::SetBitmap(HBITMAP hbm, int Opacity, COLORREF TransColor, const RECT &DestRect)
{
	if (!CreateMemDC())
		return false;

	HRESULT hr;

	IVMRMixerBitmap *pMixerBitmap;
	hr = m_Renderer.QueryInterface(&pMixerBitmap);
	if (FAILED(hr))
		return false;

	IVMRWindowlessControl *pWindowlessControl;
	hr = m_Renderer.QueryInterface(&pWindowlessControl);
	if (SUCCEEDED(hr)) {
		LONG NativeWidth = 0, NativeHeight = 0;
		hr = pWindowlessControl->GetNativeVideoSize(&NativeWidth, &NativeHeight, nullptr, nullptr);
		if (FAILED(hr) || (NativeWidth == 0) || (NativeHeight == 0)) {
			NativeWidth = 1440;
			NativeHeight = 1080;
		}
		pWindowlessControl->Release();

		BITMAP bm;
		VMRALPHABITMAP ab;

		::SelectObject(m_hdc,hbm);
		::GetObject(hbm, sizeof(BITMAP), &bm);

		ab.dwFlags = VMRBITMAP_HDC;
		if (TransColor != CLR_INVALID)
			ab.dwFlags |= VMRBITMAP_SRCCOLORKEY;
		ab.hdc = m_hdc;
		ab.pDDS = nullptr;
		::SetRect(&ab.rSrc, 0, 0, bm.bmWidth, bm.bmHeight);
		ab.rDest.left = static_cast<float>(DestRect.left) / static_cast<float>(NativeWidth);
		ab.rDest.top = static_cast<float>(DestRect.top) / static_cast<float>(NativeHeight);
		ab.rDest.right = static_cast<float>(DestRect.right) / static_cast<float>(NativeWidth);
		ab.rDest.bottom = static_cast<float>(DestRect.bottom) / static_cast<float>(NativeHeight);
		ab.fAlpha = static_cast<float>(Opacity) / 100.0f;
		ab.clrSrcKey = TransColor;

		hr = pMixerBitmap->SetAlphaBitmap(&ab);
		if (FAILED(hr))
			::SelectObject(m_hdc, m_hbmOld);
	}

	pMixerBitmap->Release();

	return SUCCEEDED(hr);
}


bool ImageMixer_VMR7::GetMapSize(ReturnArg<int> Width, ReturnArg<int> Height)
{
	bool OK = false;
	IVMRWindowlessControl *pWindowlessControl;

	if (SUCCEEDED(m_Renderer.QueryInterface(&pWindowlessControl))) {
		LONG NativeWidth, NativeHeight;

		if (SUCCEEDED(pWindowlessControl->GetNativeVideoSize(&NativeWidth, &NativeHeight, nullptr, nullptr))) {
			Width = NativeWidth;
			Height = NativeHeight;
			OK = true;
		}

		pWindowlessControl->Release();
	}

	return OK;
}


}	// namespace LibISDB::DirectShow
