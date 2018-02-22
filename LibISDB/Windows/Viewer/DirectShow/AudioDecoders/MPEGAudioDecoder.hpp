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
 @file   MPEGAudioDecoder.hpp
 @brief  MPEG 音声デコーダ
 @author DBCTRADO
*/


#ifndef LIBISDB_MPEG_AUDIO_DECODER_H
#define LIBISDB_MPEG_AUDIO_DECODER_H


#include "AudioDecoder.hpp"

#if defined(_WIN64)
#define FPM_64BIT
#elif defined(_M_IX86)
#define FPM_INTEL
#endif
#include "../../../../../Thirdparty/libmad/mad.h"


namespace LibISDB::DirectShow
{

	/** MPEG 音声デコーダクラス */
	class MPEGAudioDecoder
		: public AudioDecoder
	{
	public:
		MPEGAudioDecoder();
		~MPEGAudioDecoder();

	// AudioDecoder
		bool Open() override;
		void Close() override;
		bool IsOpened() const override;
		bool Reset() override;
		bool Decode(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info) override;

		bool IsSPDIFSupported() const override { return false; }

	private:
		bool OpenDecoder();
		void CloseDecoder();
		bool ResetDecoder();
		bool DecodeFrame(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info);

		static constexpr size_t INPUT_BUFFER_SIZE = 4096;
		static constexpr size_t PCM_BUFFER_LENGTH = 1152 * 2;

		struct mad_stream m_MadStream;
		struct mad_frame m_MadFrame;
		struct mad_synth m_MadSynth;
		bool m_Initialized;
		bool m_DecodeError;
		uint8_t m_InputBuffer[INPUT_BUFFER_SIZE];
		int16_t m_PCMBuffer[PCM_BUFFER_LENGTH];
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_MPEG_AUDIO_DECODER_H
