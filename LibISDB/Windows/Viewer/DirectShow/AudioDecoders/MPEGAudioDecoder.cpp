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
 @file   MPEGAudioDecoder.cpp
 @brief  MPEG 音声デコーダ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "MPEGAudioDecoder.hpp"
#include "../../../../Base/DebugDef.hpp"


#pragma comment(lib, "libmad.lib")
#pragma comment(lib, "legacy_stdio_definitions.lib")


namespace LibISDB::DirectShow
{


namespace
{

constexpr int16_t FixedToInt16(mad_fixed_t Value)
{
	Value += 1L << (MAD_F_FRACBITS - 16);
	if (Value >= MAD_F_ONE)
		Value = MAD_F_ONE - 1;
	else if (Value < -MAD_F_ONE)
		Value = -MAD_F_ONE;
	return static_cast<int16_t>(Value >> (MAD_F_FRACBITS + 1 - 16));
}

}	// namespace




MPEGAudioDecoder::MPEGAudioDecoder() noexcept
	: m_Initialized(false)
	, m_DecodeError(false)
{
}


MPEGAudioDecoder::~MPEGAudioDecoder()
{
	Close();
}


bool MPEGAudioDecoder::Open()
{
	if (!OpenDecoder())
		return false;

	ClearAudioInfo();

	return true;
}


void MPEGAudioDecoder::Close()
{
	CloseDecoder();
}


bool MPEGAudioDecoder::IsOpened() const
{
	return m_Initialized;
}


bool MPEGAudioDecoder::Reset()
{
	if (!ResetDecoder())
		return false;

	ClearAudioInfo();
	m_DecodeError = false;

	return true;
}


bool MPEGAudioDecoder::Decode(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info)
{
	if (!m_Initialized)
		return false;

	if (!DecodeFrame(pData, pDataSize, Info)) {
		m_DecodeError = true;
		return false;
	}

	m_DecodeError = false;

	return true;
}


bool MPEGAudioDecoder::OpenDecoder()
{
	CloseDecoder();

	mad_stream_init(&m_MadStream);
	mad_frame_init(&m_MadFrame);
	mad_synth_init(&m_MadSynth);

	m_Initialized = true;
	m_DecodeError = false;

	return true;
}


void MPEGAudioDecoder::CloseDecoder()
{
	if (m_Initialized) {
		mad_synth_finish(&m_MadSynth);
		mad_frame_finish(&m_MadFrame);
		mad_stream_finish(&m_MadStream);

		m_Initialized = false;
	}
}


bool MPEGAudioDecoder::ResetDecoder()
{
	if (!m_Initialized)
		return false;

	return OpenDecoder();
}


bool MPEGAudioDecoder::DecodeFrame(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info)
{
	if (!m_Initialized) {
		return false;
	}

	if ((m_MadStream.buffer == nullptr) || (m_MadStream.error == MAD_ERROR_BUFLEN)) {
		size_t ReadSize, Remain;

		if (m_MadStream.next_frame != nullptr) {
			Remain = m_MadStream.bufend - m_MadStream.next_frame;
			std::memcpy(m_InputBuffer, m_MadStream.next_frame, Remain);
			ReadSize = INPUT_BUFFER_SIZE - Remain;
		} else {
			ReadSize = INPUT_BUFFER_SIZE;
			Remain = 0;
		}

		if (ReadSize > *pDataSize)
			ReadSize = *pDataSize;
		*pDataSize = ReadSize;

		std::memcpy(&m_InputBuffer[Remain], pData, ReadSize);

		mad_stream_buffer(&m_MadStream, m_InputBuffer, static_cast<unsigned long>(ReadSize + Remain));
		m_MadStream.error = MAD_ERROR_NONE;
	} else {
		*pDataSize = 0;
	}

	if (mad_frame_decode(&m_MadFrame, &m_MadStream)) {
		Info->SampleCount = 0;

		if (m_MadStream.error == MAD_ERROR_BUFLEN)
			return true;
		if (MAD_RECOVERABLE(m_MadStream.error))
			return true;
		LIBISDB_TRACE(
			LIBISDB_STR("libmad error : ") LIBISDB_STR(LIBISDB_PRIs) LIBISDB_STR("\n"),
			mad_stream_errorstr(&m_MadStream));
		ResetDecoder();
		return false;
	}

	mad_synth_frame(&m_MadSynth, &m_MadFrame);

	const uint8_t Channels = MAD_NCHANNELS(&m_MadFrame.header);
	int16_t *p = m_PCMBuffer;

	if (Channels == 1) {
		for (int i = 0; i < m_MadSynth.pcm.length; i++) {
			*p++ = FixedToInt16(m_MadSynth.pcm.samples[0][i]);
		}
	} else {
		for (int i = 0; i < m_MadSynth.pcm.length; i++) {
			*p++ = FixedToInt16(m_MadSynth.pcm.samples[0][i]);
			*p++ = FixedToInt16(m_MadSynth.pcm.samples[1][i]);
		}
	}

	m_AudioInfo.Frequency = m_MadSynth.pcm.samplerate;
	m_AudioInfo.ChannelCount = Channels;
	m_AudioInfo.OriginalChannelCount = Channels;
	m_AudioInfo.DualMono = false;

	Info->pData = reinterpret_cast<const uint8_t *>(m_PCMBuffer);
	Info->SampleCount = m_MadSynth.pcm.length;
	Info->Info = m_AudioInfo;
	Info->Discontinuity = m_DecodeError;

	return true;
}


}	// namespace LibISDB::DirectShow
