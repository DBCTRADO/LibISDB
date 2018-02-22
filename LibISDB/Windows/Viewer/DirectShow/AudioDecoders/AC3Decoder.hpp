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
 @file   AC3Decoder.hpp
 @brief  AC3 デコーダ
 @author DBCTRADO
*/


#ifndef LIBISDB_AC3_DECODER_H
#define LIBISDB_AC3_DECODER_H


#include "AudioDecoder.hpp"

extern "C" {
#include "../../../../../Thirdparty/a52dec/vc++/config.h"
#include "../../../../../Thirdparty/a52dec/include/a52.h"
}


namespace LibISDB::DirectShow
{

	/** AC3 デコーダクラス */
	class AC3Decoder
		: public AudioDecoder
	{
	public:
		AC3Decoder();
		~AC3Decoder();

	// CAudioDecoder
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
		bool DecodeFrame(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info);

		struct A52Info {
			int Flags;
			int SampleRate;
			int BitRate;
		};

		static constexpr size_t MAX_FRAME_SIZE = 3840;
		static constexpr size_t PCM_BUFFER_LENGTH = 256 * 6 * 6;

		a52_state_t *m_pA52State;
		A52Info m_A52Info;
		bool m_DecodeError;
		uint16_t m_SyncWord;
		int m_FrameLength;
		int m_FramePos;
		uint8_t m_FrameBuffer[MAX_FRAME_SIZE];
		int16_t m_PCMBuffer[PCM_BUFFER_LENGTH];
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_AC3_DECODER_H
