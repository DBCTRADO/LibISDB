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
 @file   AACDecoder.hpp
 @brief  AAC デコーダ
 @author DBCTRADO
*/


#ifndef LIBISDB_AAC_DECODER_H
#define LIBISDB_AAC_DECODER_H


#include "AudioDecoder.hpp"
#include "../../../../MediaParsers/ADTSParser.hpp"
#include "../../../../../Thirdparty/faad2/include/neaacdec.h"


namespace LibISDB::DirectShow
{

	/** AAC デコーダクラス */
	class AACDecoder
		: public AudioDecoder
	{
	public:
		AACDecoder();
		~AACDecoder();

	// AudioDecoder
		bool Open() override;
		void Close() override;
		bool IsOpened() const override;
		bool Reset() override;
		bool Decode(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info) override;

		bool IsSPDIFSupported() const override { return true; }
		bool GetSPDIFFrameInfo(ReturnArg<SPDIFFrameInfo> Info) const override;
		int GetSPDIFBurstPayload(uint8_t *pBuffer, size_t BufferSize) const override;

		bool GetChannelMap(int Channels, int *pMap) const override;
		bool GetDownmixInfo(ReturnArg<DownmixInfo> Info) const override;

	private:
		bool OpenDecoder();
		void CloseDecoder();
		bool ResetDecoder();
		bool DecodeFrame(const ADTSFrame *pFrame, ReturnArg<DecodeFrameInfo> Info);

		ADTSParser m_ADTSParser;
		NeAACDecHandle m_hDecoder;
		bool m_InitRequest;
		uint8_t m_LastChannelConfig;
		const ADTSFrame *m_pADTSFrame;
		bool m_DecodeError;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_AAC_DECODER_H
