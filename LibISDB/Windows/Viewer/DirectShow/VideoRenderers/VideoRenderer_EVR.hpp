/*
  LibISDB
  Copyright(c) 2017-2019 DBCTRADO

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
 @file   VideoRenderer_EVR.hpp
 @brief  EVR 映像レンダラ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_RENDERER_EVR_H
#define LIBISDB_VIDEO_RENDERER_EVR_H


#include "VideoRenderer.hpp"


interface IMFVideoDisplayControl;


namespace LibISDB::DirectShow
{

/*
	EVRのバグ(?)によりウィンドウの一部を転送先に指定すると
	バックバッファがクリアされずにフリッカが発生するので、
	転送用のウィンドウを作成する
*/
#define LIBISDB_EVR_USE_VIDEO_WINDOW

	/** EVR 映像レンダラクラス */
	class VideoRenderer_EVR
		: public VideoRenderer
	{
	public:
		RendererType GetRendererType() const noexcept { return RendererType::EVR; }
		bool Initialize(
			IGraphBuilder *pGraphBuilder, IPin *pInputPin,
			HWND hwndRender, HWND hwndMessageDrain) override;
		bool Finalize() override;
		bool SetVideoPosition(
			int SourceWidth, int SourceHeight, const RECT &SourceRect,
		const RECT &DestRect, const RECT &WindowRect) override;
		bool GetDestPosition(ReturnArg<RECT> Rect) override;
		COMMemoryPointer<> GetCurrentImage() override;
		bool ShowCursor(bool Show) override;
		bool RepaintVideo(HWND hwnd, HDC hdc) override;
		bool DisplayModeChanged() override;
		bool SetVisible(bool Visible) override;

		bool SetClipToDevice(bool Clip);

	protected:
		virtual HRESULT InitializePresenter(IBaseFilter *pFilter) { return S_OK; }

	private:
		IMFVideoDisplayControl * GetVideoDisplayControl() const;
		HRESULT UpdateRenderingPrefs(IMFVideoDisplayControl *pDisplayControl);

#ifdef LIBISDB_EVR_USE_VIDEO_WINDOW
		HWND m_hwndVideo = nullptr;
		HWND m_hwndMessageDrain = nullptr;
		bool m_ShowCursor = true;

		static VideoRenderer_EVR * GetThis(HWND hwnd);
		static LRESULT CALLBACK VideoWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_RENDERER_EVR_H
