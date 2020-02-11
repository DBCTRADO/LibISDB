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
 @file   VideoRenderer_OverlayMixer.hpp
 @brief  Overlay Mixer 映像レンダラ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_RENDERER_OVERLAY_MIXER_H
#define LIBISDB_VIDEO_RENDERER_OVERLAY_MIXER_H


#include "VideoRenderer.hpp"


namespace LibISDB::DirectShow
{

	/** Overlay Mixer 映像レンダラクラス */
	class VideoRenderer_OverlayMixer
		: public VideoRenderer_Default
	{
	public:
		RendererType GetRendererType() const noexcept { return RendererType::OverlayMixer; }
		bool Initialize(
			IGraphBuilder *pGraphBuilder, IPin *pInputPin,
			HWND hwndRender, HWND hwndMessageDrain) override;
		bool Finalize() override;

	private:
		COMPointer<ICaptureGraphBuilder2> m_CaptureGraphBuilder2;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_RENDERER_OVERLAY_MIXER_H
