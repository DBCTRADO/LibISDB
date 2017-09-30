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
 @file   KnownDecoderManager.hpp
 @brief  既知のデコーダ管理
 @author DBCTRADO
*/


#ifndef LIBISDB_KNOWN_DECODER_MANAGER_H
#define LIBISDB_KNOWN_DECODER_MANAGER_H


#include <dshow.h>
#include "ITVTestVideoDecoder.hpp"


namespace LibISDB::DirectShow
{

	/** 既知のデコーダ管理クラス */
	class KnownDecoderManager
	{
	public:
		struct VideoDecoderSettings {
			bool bEnableDeinterlace = true;
			TVTVIDEODEC_DeinterlaceMethod DeinterlaceMethod = TVTVIDEODEC_DEINTERLACE_BLEND;
			bool bAdaptProgressive = true;
			bool bAdaptTelecine = true;
			bool bSetInterlacedFlag = true;
			int Brightness = 0;
			int Contrast = 0;
			int Hue = 0;
			int Saturation = 0;
			int NumThreads = 0;
			bool bEnableDXVA2 = true;
		};

		KnownDecoderManager();
		~KnownDecoderManager();

		HRESULT CreateInstance(const GUID &MediaSubType, IBaseFilter **ppFilter);
		void SetVideoDecoderSettings(const VideoDecoderSettings &Settings);
		bool GetVideoDecoderSettings(VideoDecoderSettings *pSettings) const;
		bool SaveVideoDecoderSettings(IBaseFilter *pFilter);

		static bool IsMediaSupported(const GUID &MediaSubType);
		static bool IsDecoderAvailable(const GUID &MediaSubType);
		static LPCWSTR GetDecoderName(const GUID &MediaSubType);
		static CLSID GetDecoderCLSID(const GUID &MediaSubType);

	private:
		VideoDecoderSettings m_VideoDecoderSettings;
		HMODULE m_hLib;

		HRESULT LoadDecoderModule();
		void FreeDecoderModule();
		static bool GetModulePath(LPTSTR pszPath);
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_KNOWN_DECODER_MANAGER_H
