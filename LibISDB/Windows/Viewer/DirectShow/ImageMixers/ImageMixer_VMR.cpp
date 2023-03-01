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


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "ImageMixer_VMR.hpp"
#include "../../../../Base/DebugDef.hpp"

/**
 @file   ImageMixer_VMR.cpp
 @brief  VMR 画像合成
 @author DBCTRADO
*/


namespace LibISDB::DirectShow
{


ImageMixer_VMR::ImageMixer_VMR(IBaseFilter *pRenderer) noexcept
	: ImageMixer(pRenderer)
	, m_hdc(nullptr)
	, m_hbm(nullptr)
{
}


ImageMixer_VMR::~ImageMixer_VMR()
{
	if (m_hdc != nullptr) {
		::SelectObject(m_hdc, m_hbmOld);
		::DeleteDC(m_hdc);
	}
	if (m_hbm != nullptr)
		::DeleteObject(m_hbm);
}


bool ImageMixer_VMR::CreateMemDC()
{
	if (m_hdc == nullptr) {
		m_hdc = ::CreateCompatibleDC(nullptr);
		if (m_hdc == nullptr)
			return false;
		m_hbmOld = static_cast<HBITMAP>(::GetCurrentObject(m_hdc, OBJ_BITMAP));
	}
	return true;
}


bool ImageMixer_VMR::SetText(LPCTSTR pszText, int x, int y, HFONT hfont, COLORREF Color, int Opacity)
{
	if (pszText == nullptr || pszText[0] == _T('\0') || Opacity < 1)
		return false;

	if (!CreateMemDC())
		return false;

	HDC hdc = ::CreateDC(TEXT("DISPLAY"), nullptr, nullptr, nullptr);
	HFONT hfontOld = static_cast<HFONT>(::SelectObject(hdc, hfont));

	RECT rc = {};
	::DrawText(hdc, pszText, -1, &rc, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_CALCRECT);
	::OffsetRect(&rc, -rc.left, -rc.top);

	HBITMAP hbm = ::CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
	::SelectObject(hdc, hfontOld);
	::DeleteDC(hdc);
	if (hbm == nullptr)
		return false;
	::SelectObject(m_hdc, hbm);

	COLORREF TransColor, OldTextColor, OldBkColor;

#if 0
	// これだとなぜかEVRで透過されない
	//TransColor = Color ^ 0x00FFFFFF;
#else
	if (GetRValue(Color) < 8 && GetGValue(Color) < 8 && GetBValue(Color) < 8)
		Color = RGB(8, 8, 8);
	TransColor = RGB(0, 0, 0);
#endif
	hfontOld = static_cast<HFONT>(::SelectObject(m_hdc, hfont));
	OldTextColor = ::SetTextColor(m_hdc, Color);
	OldBkColor = ::SetBkColor(m_hdc, TransColor);
	const int OldBkMode = ::SetBkMode(m_hdc, OPAQUE);
	::DrawText(m_hdc, pszText, -1, &rc, DT_LEFT | DT_TOP | DT_NOPREFIX);
	::SetBkMode(m_hdc, OldBkMode);
	::SetBkColor(m_hdc, OldBkColor);
	::SetTextColor(m_hdc, OldTextColor);
	::SelectObject(m_hdc, hfontOld);
	::SelectObject(m_hdc, m_hbmOld);

	RECT rcDest;
	rcDest.left = x;
	rcDest.top = y;
	rcDest.right = x + rc.right;
	rcDest.bottom = y + rc.bottom;
	if (!SetBitmap(hbm, Opacity, TransColor, rcDest)) {
		::DeleteObject(hbm);
		return false;
	}

	if (m_hbm != nullptr)
		::DeleteObject(m_hbm);
	m_hbm = hbm;

	return true;
}


}	// namespace LibISDB::DirectShow
