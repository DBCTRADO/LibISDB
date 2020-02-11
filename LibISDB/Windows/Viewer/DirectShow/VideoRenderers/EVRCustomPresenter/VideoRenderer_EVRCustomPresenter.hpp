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
 @file   VideoRenderer_EVRCustomPresenter.hpp
 @brief  EVR Custom Presenter 映像レンダラ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_RENDERER_EVR_CUSTOM_PRESENTER_H
#define LIBISDB_VIDEO_RENDERER_EVR_CUSTOM_PRESENTER_H


#include "../VideoRenderer_EVR.hpp"
#include <evr.h>


namespace LibISDB::DirectShow
{

	/** EVR Custom Presenter 映像レンダラクラス */
	class VideoRenderer_EVRCustomPresenter
		: public VideoRenderer_EVR
	{
	public:
		RendererType GetRendererType() const noexcept { return RendererType::EVRCustomPresenter; }
		bool Finalize() override;
		bool HasProperty() override { return false; }	// プロパティの数値が不定値になる

	private:
		HRESULT InitializePresenter(IBaseFilter *pFilter) override;

		COMPointer<IMFVideoPresenter> m_Presenter;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_RENDERER_EVR_CUSTOM_PRESENTER_H
