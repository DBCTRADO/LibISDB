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
 @file   ImageMixer.cpp
 @brief  画像合成
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "ImageMixer.hpp"
#ifdef LIBISDB_IMAGE_MIXER_VMR7_SUPPORT
#include "ImageMixer_VMR7.hpp"
#endif
#include "ImageMixer_VMR9.hpp"
#include "ImageMixer_EVR.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


ImageMixer::ImageMixer(IBaseFilter *pRenderer)
	: m_Renderer(pRenderer)
{
}


ImageMixer *ImageMixer::CreateImageMixer(
	VideoRenderer::RendererType RendererType, IBaseFilter *pRendererFilter)
{
	switch (RendererType) {
#ifdef LIBISDB_IMAGE_MIXER_VMR7_SUPPORT
	case VideoRenderer::RendererType::VMR7:
		return new ImageMixer_VMR7(pRendererFilter);
#endif
	case VideoRenderer::RendererType::VMR9:
		return new ImageMixer_VMR9(pRendererFilter);
	case VideoRenderer::RendererType::EVR:
		return new ImageMixer_EVR(pRendererFilter);
	}

	return nullptr;
}


bool ImageMixer::IsSupported(VideoRenderer::RendererType RendererType)
{
	switch (RendererType) {
#ifdef LIBISDB_IMAGE_MIXER_VMR7_SUPPORT
	case VideoRenderer::RendererType::VMR7:
#endif
	case VideoRenderer::RendererType::VMR9:
	case VideoRenderer::RendererType::EVR:
		return true;
	}

	return false;
}


}	// namespace LibISDB::DirectShow
