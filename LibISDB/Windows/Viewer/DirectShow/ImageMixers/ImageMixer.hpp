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
 @file   ImageMixer.hpp
 @brief  画像合成
 @author DBCTRADO
*/


#ifndef LIBISDB_IMAGE_MIXER_H
#define LIBISDB_IMAGE_MIXER_H


#include "../VideoRenderers/VideoRenderer.hpp"


namespace LibISDB::DirectShow
{

	/** 画像合成基底クラス */
	class ImageMixer
	{
	public:
		ImageMixer(IBaseFilter *pRenderer);
		virtual ~ImageMixer() = default;

		virtual void Clear() = 0;
		virtual bool SetBitmap(HBITMAP hbm, int Opacity, COLORREF TransColor, const RECT &DestRect) = 0;
		virtual bool SetText(LPCTSTR pszText, int x, int y, HFONT hfont, COLORREF Color, int Opacity) = 0;
		virtual bool GetMapSize(ReturnArg<int> Width, ReturnArg<int> Height) = 0;

		static ImageMixer * CreateImageMixer(
			VideoRenderer::RendererType RendererType, IBaseFilter *pRendererFilter);
		static bool IsSupported(VideoRenderer::RendererType RendererType);

	protected:
		COMPointer<IBaseFilter> m_Renderer;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_IMAGE_MIXER_H
