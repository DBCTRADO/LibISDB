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
 @file   VideoRenderer_madVR.cpp
 @brief  madVR 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_madVR.hpp"
#include <d3d9.h>
#include <initguid.h>
#include "../../../../../Thirdparty/madVR/mvrInterfaces.h"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


VideoRenderer_madVR::VideoRenderer_madVR()
	: VideoRenderer_Basic(CLSID_madVR, LIBISDB_STR("madVR"), true)
{
}


COMMemoryPointer<> VideoRenderer_madVR::GetCurrentImage()
{
	if (m_Renderer) {
		COMPointer<IMadVRFrameGrabber> FrameGrabber;

		if (SUCCEEDED(m_Renderer.QueryInterface(&FrameGrabber))) {
			LPVOID pDIB = nullptr;
			const HRESULT hr = FrameGrabber->GrabFrame(
				ZOOM_100_PERCENT,
				0,
				CHROMA_UPSCALING_USER_SELECTED,
				IMAGE_DOWNSCALING_USER_SELECTED,
				IMAGE_UPSCALING_USER_SELECTED,
				0,
				&pDIB,
				nullptr);
			if (SUCCEEDED(hr) && pDIB != nullptr) {
				const size_t Size = ::LocalSize(pDIB);
				if (Size > sizeof(BITMAPINFOHEADER)) {
#ifdef LIBISDB_ENABLE_TRACE
					const BITMAPINFOHEADER *pbmih = static_cast<const BITMAPINFOHEADER *>(pDIB);
					LIBISDB_TRACE(
						LIBISDB_STR("IMadVRFrameGrabber::GrabFrame() {} x {} ({})\n"),
						pbmih->biWidth, pbmih->biHeight, pbmih->biBitCount);
#endif

					void *pBuffer = ::CoTaskMemAlloc(Size);
					if (pBuffer != nullptr) {
						::CopyMemory(pBuffer, pDIB, Size);
						::LocalFree(pDIB);
						return COMMemoryPointer<>(static_cast<BYTE *>(pBuffer));
					}
				}
				::LocalFree(pDIB);
			}

			LIBISDB_TRACE(LIBISDB_STR("IMadVRFrameGrabber::GrabFrame() Failed {:x}\n"), hr);
		}
#ifdef LIBISDB_ENABLE_TRACE
		else {
			LIBISDB_TRACE(LIBISDB_STR("No IMadVRFrameGrabber interface\n"));
		}
#endif
	}

	return VideoRenderer_Basic::GetCurrentImage();
}


const CLSID & VideoRenderer_madVR::GetCLSID()
{
	return CLSID_madVR;
}


HWND VideoRenderer_madVR::FindVideoWindow()
{
	if (m_hwndRender == nullptr)
		return nullptr;
	return ::FindWindowEx(m_hwndRender, nullptr, TEXT("madVR"), nullptr);
}


}	// namespace LibISDB::DirectShow
