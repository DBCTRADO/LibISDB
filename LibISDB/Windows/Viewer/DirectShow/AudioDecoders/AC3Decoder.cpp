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
 @file   AC3Decoder.hpp
 @brief  AC3 デコーダ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "AC3Decoder.hpp"

extern "C" {
//#include "../../../../../Thirdparty/a52dec/include/mm_accel.h"
#include "../../../../../Thirdparty/a52dec/liba52/a52_internal.h"
}

#include "../../../../Base/DebugDef.hpp"


#pragma comment(lib, "liba52.lib")


namespace LibISDB::DirectShow
{


namespace
{

inline int16_t SampleToInt16(float Sample)
{
	static_assert(sizeof(float) == sizeof(uint32_t));
	const int32_t i = *reinterpret_cast<int32_t *>(&Sample);
	if (i > 0x43C07FFF_i32)
		return 32767;
	if (i < 0x43BF8000_i32)
		return -32768;
	return static_cast<int16_t>(i - 0x43C00000);
}

}	// namespace




AC3Decoder::AC3Decoder()
	: m_pA52State(nullptr)
	, m_DecodeError(false)
{
}


AC3Decoder::~AC3Decoder()
{
	Close();
}


bool AC3Decoder::Open()
{
	if (!OpenDecoder())
		return false;

	ClearAudioInfo();

	return true;
}


void AC3Decoder::Close()
{
	CloseDecoder();
}


bool AC3Decoder::IsOpened() const
{
	return m_pA52State != nullptr;
}


bool AC3Decoder::Reset()
{
	if (!ResetDecoder())
		return false;

	ClearAudioInfo();
	m_DecodeError = false;

	return true;
}


bool AC3Decoder::Decode(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info)
{
	if (!m_pA52State)
		return false;

	if (!DecodeFrame(pData, pDataSize, Info)) {
		m_DecodeError = true;
		return false;
	}

	m_DecodeError = false;

	return true;
}


bool AC3Decoder::GetSPDIFFrameInfo(ReturnArg<SPDIFFrameInfo> Info) const
{
	if (!Info || (m_FrameLength == 0))
		return false;

	Info->Pc = 0x0001_u16;	// AC-3
	Info->FrameSize = m_FrameLength;
	Info->SamplesPerFrame = 256 * 6;

	return true;
}


int AC3Decoder::GetSPDIFBurstPayload(uint8_t *pBuffer, size_t BufferSize) const
{
	if ((pBuffer == nullptr) || (m_FrameLength == 0) || (m_FramePos != m_FrameLength))
		return 0;

	const int FrameSize = m_FrameLength;
	const int DataBurstSize = (FrameSize + 1) & ~1;
	if (BufferSize < static_cast<size_t>(DataBurstSize))
		return 0;

	::_swab(
		reinterpret_cast<char *>(const_cast<uint8_t *>(m_FrameBuffer)),
		reinterpret_cast<char *>(pBuffer), FrameSize & ~1);
	if (FrameSize & 1) {
		pBuffer[FrameSize - 1] = 0;
		pBuffer[FrameSize] = m_FrameBuffer[FrameSize - 1];
	}

	return DataBurstSize;
}


bool AC3Decoder::GetChannelMap(int Channels, int *pMap) const
{
	switch (Channels) {
	case 2:
		pMap[CHANNEL_2_L] = 0;
		pMap[CHANNEL_2_R] = 1;
		break;

	case 6:
		pMap[CHANNEL_6_FL ] = 1;
		pMap[CHANNEL_6_FR ] = 3;
		pMap[CHANNEL_6_FC ] = 2;
		pMap[CHANNEL_6_LFE] = 0;
		pMap[CHANNEL_6_BL ] = 4;
		pMap[CHANNEL_6_BR ] = 5;
		break;

	default:
		return false;
	}

	return true;
}


bool AC3Decoder::GetDownmixInfo(ReturnArg<DownmixInfo> Info) const
{
	if (!Info)
		return false;

	Info->Front  = 1.0;
	Info->LFE    = 0.0;
	if (m_pA52State != nullptr) {
		Info->Center = m_pA52State->clev;
		Info->Rear   = m_pA52State->slev;
	} else {
		Info->Center = LEVEL_3DB;
		Info->Rear   = LEVEL_3DB;
	}

	return true;
}


bool AC3Decoder::OpenDecoder()
{
	CloseDecoder();

	m_pA52State = a52_init(0);
	if (!m_pA52State)
		return false;

	m_SyncWord = 0;
	m_FrameLength = 0;
	m_FramePos = 0;

	m_DecodeError = false;

	return true;
}


void AC3Decoder::CloseDecoder()
{
	if (m_pA52State) {
		a52_free(m_pA52State);
		m_pA52State = nullptr;
	}
}


bool AC3Decoder::ResetDecoder()
{
	if (!m_pA52State)
		return false;

	return OpenDecoder();
}


bool AC3Decoder::DecodeFrame(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info)
{
	if (!m_pA52State) {
		return false;
	}

	Info->SampleCount = 0;

	const size_t DataSize = *pDataSize;
	size_t Pos = 0;

	if (m_FramePos == m_FrameLength)
		m_FramePos = 0;

	if (m_FramePos == 0) {
		// syncword検索
		uint16_t SyncWord = m_SyncWord;

		while (Pos < DataSize) {
			SyncWord = (SyncWord << 8) | pData[Pos++];
			if (SyncWord == 0x0B77) {
				m_FrameLength = 0;
				m_FramePos = 2;
				m_FrameBuffer[0] = 0x0B;
				m_FrameBuffer[1] = 0x77;
				break;
			}
		}

		if (Pos == DataSize) {
			m_SyncWord = SyncWord;
			return true;
		}
	}

	if (m_FramePos < 7) {
		// a52_syncinfo に7バイト必要
		int Remain = std::min(static_cast<int>(DataSize - Pos), 7 - m_FramePos);
		std::memcpy(&m_FrameBuffer[m_FramePos], &pData[Pos], Remain);
		m_FramePos += Remain;
		if (m_FramePos < 7)
			return true;
		Pos += Remain;

		m_FrameLength = a52_syncinfo(m_FrameBuffer, &m_A52Info.Flags, &m_A52Info.SampleRate, &m_A52Info.BitRate);

		if (m_FrameLength == 0) {
			LIBISDB_TRACE(LIBISDB_STR("a52_syncinfo() error\n"));

			m_FramePos = 0;

			// syncword再検索
			for (int i = 2; i < 7; i++) {
				if (m_FrameBuffer[i] == 0x0B) {
					if (i == 6) {
						m_SyncWord = 0x000B;
						break;
					}
					if (m_FrameBuffer[i + 1] == 0x77) {
						m_FramePos = 7 - i;
						std::memcpy(&m_FrameBuffer[0], &m_FrameBuffer[i], m_FramePos);
						break;
					}
				}
			}

			*pDataSize = Pos;

			return true;
		}
	}

	int Remain = std::min(static_cast<int>(DataSize - Pos), m_FrameLength - m_FramePos);
	std::memcpy(&m_FrameBuffer[m_FramePos], &pData[Pos], Remain);
	m_FramePos += Remain;
	if (m_FramePos < m_FrameLength)
		return true;
	Pos += Remain;
	*pDataSize = Pos;

	const bool LFE = (m_A52Info.Flags & A52_LFE) != 0;
	uint8_t Channels, OutChannels;
	bool DualMono = false;

	switch (m_A52Info.Flags & A52_CHANNEL_MASK) {
	case A52_CHANNEL:  // Dual mono
		Channels = 2;
		DualMono = true;
		break;

	case A52_MONO:     // Mono
	case A52_CHANNEL1: // 1st channel of Dual mono
	case A52_CHANNEL2: // 2nd channel of Dual mono
		Channels = 1;
		break;

	case A52_STEREO:   // Stereo
	case A52_DOLBY:    // Dolby surround compatible stereo
		Channels = 2;
		break;

	case A52_3F:       // 3 front (L, C, R)
	case A52_2F1R:     // 2 front, 1 rear (L, R, S)
		Channels = 3;
		break;

	case A52_3F1R:     // 3 front, 1 rear (L, C, R, S)
	case A52_2F2R:     // 2 front, 2 rear (L, R, LS, RS)
		Channels = 4;
		break;

	case A52_3F2R:     // 3 front, 2 rear (L, C, R, LS, RS)
		Channels = 5;
		break;

	default:
		return false;
	}

	int FrameFlags;

	if (LFE) {
		Channels++;
		OutChannels = 6;
		FrameFlags = A52_3F2R | A52_LFE;
	} else if (Channels <= 2) {
		OutChannels = Channels;
		FrameFlags = m_A52Info.Flags & A52_CHANNEL_MASK;
	} else {
		OutChannels = 6;
		FrameFlags = A52_3F2R;
	}

	sample_t Level = 1.0f;
	if (a52_frame(m_pA52State, m_FrameBuffer, &FrameFlags, &Level, 384.0f)) {
		LIBISDB_TRACE(LIBISDB_STR("a52_frame() error\n"));
		ResetDecoder();
		return false;
	}

	// Disable dynamic range compression
	a52_dynrng(m_pA52State, nullptr, nullptr);

	for (int i = 0; i < 6; i++) {
		if (a52_block(m_pA52State)) {
			LIBISDB_TRACE(LIBISDB_STR("a52_block() error\n"));
			ResetDecoder();
			return false;
		}

		const sample_t *pSamples = a52_samples(m_pA52State);
		const sample_t *p = pSamples;
		int16_t *pOutBuffer = m_PCMBuffer + (i * (OutChannels * 256));
		int j = 0;

		if ((Channels > 2) && !LFE) {
			for (int k = 0; k < 256; k++) {
				pOutBuffer[k * OutChannels] = 0;
			}
			j = 1;
		}

		for (; j < OutChannels; j++) {
			for (int k = 0; k < 256; k++) {
				pOutBuffer[k * OutChannels + j] = SampleToInt16(*p++);
			}
		}
	}

	m_AudioInfo.Frequency = m_A52Info.SampleRate;
	m_AudioInfo.ChannelCount = OutChannels;
	m_AudioInfo.OriginalChannelCount = Channels;
	m_AudioInfo.DualMono = DualMono;

	Info->pData = reinterpret_cast<const uint8_t *>(m_PCMBuffer);
	Info->SampleCount = 256 * 6;
	Info->Info = m_AudioInfo;
	Info->Discontinuity = m_DecodeError;

	return true;
}


}	// namespace LibISDB::DirectShow
