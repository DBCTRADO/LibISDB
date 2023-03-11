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
 @file   VideoRenderer_MPCVideoRenderer.hpp
 @brief  MPC Video Renderer 映像レンダラ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_RENDERER_MPC_VIDEO_RENDERER_H
#define LIBISDB_VIDEO_RENDERER_MPC_VIDEO_RENDERER_H


#include "VideoRenderer.hpp"


namespace LibISDB::DirectShow
{

	/** MPC Video Renderer 映像レンダラクラス */
	class VideoRenderer_MPCVideoRenderer
		: public VideoRenderer_Basic
	{
	public:
		VideoRenderer_MPCVideoRenderer();

		RendererType GetRendererType() const noexcept override { return RendererType::MPCVideoRenderer; }
		bool Initialize(
			IGraphBuilder *pGraphBuilder, IPin *pInputPin,
			HWND hwndRender, HWND hwndMessageDrain) override;
		bool Finalize() override;
		bool SetVideoPosition(
			int SourceWidth, int SourceHeight, const RECT &SourceRect,
			const RECT &DestRect, const RECT &WindowRect) override;
		bool GetDestPosition(ReturnArg<RECT> Rect) override;
		bool ShowCursor(bool Show) override;
		bool SetVisible(bool Visible) override;

		static const CLSID & GetCLSID();

	private:
		static VideoRenderer_MPCVideoRenderer * GetThis(HWND hwnd);
		static LRESULT CALLBACK VideoWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		HWND m_hwndVideo = nullptr;
		HWND m_hwndMessageDrain = nullptr;
		bool m_ShowCursor = true;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_RENDERER_MPC_VIDEO_RENDERER_H
