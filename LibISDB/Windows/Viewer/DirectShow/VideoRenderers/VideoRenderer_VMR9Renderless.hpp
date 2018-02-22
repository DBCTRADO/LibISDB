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
 @file   VideoRenderer_VMR9Renderless.hpp
 @brief  VMR-9 Renderless 映像レンダラ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_RENDERER_VMR9_RENDERLESS_H
#define LIBISDB_VIDEO_RENDERER_VMR9_RENDERLESS_H


#include "VideoRenderer.hpp"


namespace LibISDB::DirectShow
{

	class VMR9Allocator;

	/** VMR-9 Renderless 映像レンダラクラス */
	class VideoRenderer_VMR9Renderless
		: public VideoRenderer
	{
	public:
		VideoRenderer_VMR9Renderless();
		~VideoRenderer_VMR9Renderless();

		RendererType GetRendererType() const noexcept { return RendererType::VMR9Renderless; }
		bool Initialize(
			IGraphBuilder *pGraphBuilder, IPin *pInputPin,
			HWND hwndRender, HWND hwndMessageDrain) override;
		bool Finalize() override;
		bool SetVideoPosition(
			int SourceWidth, int SourceHeight, const RECT &SourceRect,
			const RECT &DestRect, const RECT &WindowRect) override;
		bool GetDestPosition(ReturnArg<RECT> Rect) override;
		COMMemoryPointer<> GetCurrentImage() override;
		bool RepaintVideo(HWND hwnd, HDC hdc) override;
		bool DisplayModeChanged() override;
		bool SetVisible(bool Visible) override;

	private:
		VMR9Allocator *m_pAllocator;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_RENDERER_VMR9_RENDERLESS_H
