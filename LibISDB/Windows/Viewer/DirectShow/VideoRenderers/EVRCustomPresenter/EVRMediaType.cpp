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
 @file   EVRMediaType.cpp
 @brief  EVR メディアタイプ
 @author DBCTRADO
*/


#include "../../../../../LibISDBPrivate.hpp"
#include "../../../../../LibISDBWindows.hpp"
#include "EVRPresenterBase.hpp"
#include "EVRMediaType.hpp"
#include "../../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


HRESULT GetFrameRate(IMFMediaType *pType, MFRatio *pRatio)
{
	return ::MFGetAttributeRatio(
		pType, MF_MT_FRAME_RATE,
		reinterpret_cast<UINT32 *>(&pRatio->Numerator),
		reinterpret_cast<UINT32 *>(&pRatio->Denominator));
}


HRESULT GetVideoDisplayArea(IMFMediaType *pType, MFVideoArea *pArea)
{
	HRESULT hr = S_OK;
	bool PanScan = ::MFGetAttributeUINT32(pType, MF_MT_PAN_SCAN_ENABLED, FALSE) != FALSE;

	if (PanScan) {
		hr = pType->GetBlob(MF_MT_PAN_SCAN_APERTURE, reinterpret_cast<UINT8 *>(pArea), sizeof(MFVideoArea), nullptr);
	}

	if (!PanScan || (hr == MF_E_ATTRIBUTENOTFOUND)) {
		hr = pType->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, reinterpret_cast<UINT8 *>(pArea), sizeof(MFVideoArea), nullptr);

		if (hr == MF_E_ATTRIBUTENOTFOUND) {
			hr = pType->GetBlob(MF_MT_GEOMETRIC_APERTURE, reinterpret_cast<UINT8 *>(pArea), sizeof(MFVideoArea), nullptr);
		}

		if (hr == MF_E_ATTRIBUTENOTFOUND) {
			UINT32 Width = 0, Height = 0;

			hr = ::MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &Width, &Height);
			if (SUCCEEDED(hr)) {
				*pArea = MakeArea(0.0f, 0.0f, Width, Height);
			}
		}
	}

	return hr;
}


HRESULT GetDefaultStride(IMFMediaType *pType, LONG *pStride)
{
	HRESULT hr;
	LONG Stride = 0;

	hr = pType->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&Stride);

	if (FAILED(hr)) {
		GUID Subtype = GUID_NULL;

		hr = pType->GetGUID(MF_MT_SUBTYPE, &Subtype);

		if (SUCCEEDED(hr)) {
			UINT32 Width = 0, Height = 0;

			hr = ::MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &Width, &Height);

			if (SUCCEEDED(hr)) {
				hr = ::MFGetStrideForBitmapInfoHeader(Subtype.Data1, Width, &Stride);

				if (SUCCEEDED(hr)) {
					pType->SetUINT32(MF_MT_DEFAULT_STRIDE, Stride);
				}
			}
		}
	}

	if (SUCCEEDED(hr)) {
		*pStride = Stride;
	}

	return hr;
}


}	// namespace LibISDB::DirectShow
