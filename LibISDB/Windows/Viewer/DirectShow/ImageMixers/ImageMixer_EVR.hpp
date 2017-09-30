/*
  LibISDB
  Copyright(c) 2017 DBCTRADO

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
 @file   ImageMixer_EVR.hpp
 @brief  EVR 画像合成
 @author DBCTRADO
*/


#ifndef LIBISDB_IMAGE_MIXER_EVR_H
#define LIBISDB_IMAGE_MIXER_EVR_H


#include "ImageMixer_VMR.hpp"


namespace LibISDB::DirectShow
{

	/** EVR 画像合成クラス */
	class ImageMixer_EVR
		: public ImageMixer_VMR
	{
	public:
		ImageMixer_EVR(IBaseFilter *pRenderer);
		~ImageMixer_EVR();

		void Clear() override;
		bool SetBitmap(HBITMAP hbm, int Opacity, COLORREF TransColor, const RECT &DestRect) override;
		bool GetMapSize(ReturnArg<int> Width, ReturnArg<int> Height) override;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_IMAGE_MIXER_EVR_H
