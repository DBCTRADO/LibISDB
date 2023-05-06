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
 @file   VideoRenderer_madVR.hpp
 @brief  madVR 映像レンダラ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_RENDERER_MADVR_H
#define LIBISDB_VIDEO_RENDERER_MADVR_H


#include "VideoRenderer.hpp"


namespace LibISDB::DirectShow
{

	/** madVR 映像レンダラクラス */
	class VideoRenderer_madVR
		: public VideoRenderer_Basic
	{
	public:
		VideoRenderer_madVR();

		RendererType GetRendererType() const noexcept override { return RendererType::madVR; }
		COMMemoryPointer<> GetCurrentImage() override;

		static const CLSID & GetCLSID();

	private:
		HWND FindVideoWindow() override;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_RENDERER_MADVR_H
