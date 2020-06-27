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
 @file   AACDecoder_FDK.hpp
 @brief  AAC デコーダ (FDK AAC)
 @author DBCTRADO
*/


#ifndef LIBISDB_AAC_DECODER_FDK_H
#define LIBISDB_AAC_DECODER_FDK_H


#include "AACDecoder.hpp"
#include "../../../../MediaParsers/ADTSParser.hpp"


typedef struct AAC_DECODER_INSTANCE *HANDLE_AACDECODER;


namespace LibISDB::DirectShow
{

	/** AAC デコーダクラス (FDK AAC) */
	class AACDecoder_FDK
		: public AACDecoder
	{
	public:
		AACDecoder_FDK() noexcept;
		~AACDecoder_FDK();

	// AudioDecoder
		bool Open() override;
		void Close() override;
		bool IsOpened() const override;

		bool GetChannelMap(int Channels, int *pMap) const override;
		bool GetDownmixInfo(ReturnArg<DownmixInfo> Info) const override;

	private:
		bool OpenDecoder() override;
		void CloseDecoder() override;
		bool DecodeFrame(const ADTSFrame *pFrame, ReturnArg<DecodeFrameInfo> Info) override;

		HANDLE_AACDECODER m_hDecoder;
		uint8_t m_LastChannelConfig;
		DataBuffer m_PCMBuffer;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_AAC_DECODER_FDK_H
