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
 @file   AACDecoder_FDK.cpp
 @brief  AAC デコーダ (FDK_AAC)
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "AACDecoder_FDK.hpp"
#include "../../../../../Thirdparty/fdk-aac/libAACdec/include/aacdecoder_lib.h"
#include "../../../../Base/DebugDef.hpp"


#pragma comment(lib, "fdk-aac.lib")


namespace LibISDB::DirectShow
{


AACDecoder_FDK::AACDecoder_FDK() noexcept
	: m_hDecoder(nullptr)
	, m_LastChannelConfig(0xFF)
{
}


AACDecoder_FDK::~AACDecoder_FDK()
{
	Close();
}


bool AACDecoder_FDK::Open()
{
	if (!AACDecoder::Open())
		return false;

	constexpr size_t PCM_BUFFER_SIZE = 6 * sizeof(int16_t) * 4096;
	if (m_PCMBuffer.AllocateBuffer(PCM_BUFFER_SIZE) < PCM_BUFFER_SIZE) {
		Close();
		return false;
	}

	return true;
}


void AACDecoder_FDK::Close()
{
	AACDecoder::Close();

	m_PCMBuffer.FreeBuffer();
}


bool AACDecoder_FDK::IsOpened() const
{
	return m_hDecoder != nullptr;
}


bool AACDecoder_FDK::GetChannelMap(int Channels, int *pMap) const
{
	switch (Channels) {
	case 2:
		pMap[CHANNEL_2_L] = 0;
		pMap[CHANNEL_2_R] = 1;
		break;

	case 6:
		pMap[CHANNEL_6_FL]  = 0;
		pMap[CHANNEL_6_FR]  = 1;
		pMap[CHANNEL_6_FC]  = 2;
		pMap[CHANNEL_6_LFE] = 3;
		pMap[CHANNEL_6_BL]  = 4;
		pMap[CHANNEL_6_BR]  = 5;
		break;

	default:
		return false;
	}

	return true;
}


bool AACDecoder_FDK::GetDownmixInfo(ReturnArg<DownmixInfo> Info) const
{
	if (!Info)
		return false;

	constexpr double PSQR = 1.0 / 1.4142135623730950488016887242097;

	Info->Center = PSQR;
	Info->Front  = 1.0;
	Info->Rear   = PSQR;
	Info->LFE    = 0.0;

	return true;
}


bool AACDecoder_FDK::OpenDecoder()
{
	CloseDecoder();

	// FDK AAC オープン
	m_hDecoder = ::aacDecoder_Open(TT_MP4_ADTS, 1);
	if (m_hDecoder == nullptr)
		return false;

	// デコーダのパラメータを設定
	::aacDecoder_SetParam(m_hDecoder, AAC_PCM_OUTPUT_CHANNEL_MAPPING, 1);
	::aacDecoder_SetParam(m_hDecoder, AAC_PCM_MAX_OUTPUT_CHANNELS, 6);
	::aacDecoder_SetParam(m_hDecoder, AAC_PCM_LIMITER_ENABLE, 0);

	m_LastChannelConfig = 0xFF;
	m_DecodeError = false;

	return true;
}


void AACDecoder_FDK::CloseDecoder()
{
	// ADK AAC クローズ
	if (m_hDecoder != nullptr) {
		::aacDecoder_Close(m_hDecoder);
		m_hDecoder = nullptr;
	}
}


bool AACDecoder_FDK::DecodeFrame(const ADTSFrame *pFrame, ReturnArg<DecodeFrameInfo> Info)
{
	if (m_hDecoder == nullptr) {
		return false;
	}

	// 初回フレーム解析
	if (pFrame->GetChannelConfig() != m_LastChannelConfig) {
		// チャンネル設定が変化した、デコーダリセット
		LIBISDB_TRACE(
			LIBISDB_STR("AACDecoder_FDK::DecodeFrame() Channel config changed {} -> {}\n"),
			m_LastChannelConfig,
			pFrame->GetChannelConfig());
		if (!ResetDecoder())
			return false;

		m_LastChannelConfig = pFrame->GetChannelConfig();
	}

	// デコード
	bool OK = false;
	UCHAR *p = const_cast<UCHAR *>(pFrame->GetData());
	UINT Avail = static_cast<UINT>(pFrame->GetSize());

	while (Avail > 0) {
		const UINT Size = Avail;
		AAC_DECODER_ERROR Err = ::aacDecoder_Fill(m_hDecoder, &p, &Size, &Avail);
		if (Err != AAC_DEC_OK) {
			LIBISDB_TRACE(
				LIBISDB_STR("aacDecoder_Fill() error {:#X}\n"),
				static_cast<std::underlying_type_t<AAC_DECODER_ERROR>>(Err));
			break;
		}
		p += Size - Avail;

		Err = ::aacDecoder_DecodeFrame(
			m_hDecoder,
			reinterpret_cast<INT_PCM *>(m_PCMBuffer.GetBuffer()),
			static_cast<INT>(m_PCMBuffer.GetBufferSize() / sizeof(int16_t)),
			m_DecodeError ? AACDEC_INTR : 0);
		if (Err == AAC_DEC_TRANSPORT_SYNC_ERROR || Err == AAC_DEC_NOT_ENOUGH_BITS)
			continue;

		if (Err == AAC_DEC_OK) {
			const ::CStreamInfo *pStreamInfo = ::aacDecoder_GetStreamInfo(m_hDecoder);

			m_AudioInfo.Frequency = pStreamInfo->sampleRate;
			m_AudioInfo.ChannelCount = pStreamInfo->numChannels;
			m_AudioInfo.OriginalChannelCount = pStreamInfo->numChannels;
			m_AudioInfo.DualMono = (pStreamInfo->numChannels == 2) && (m_LastChannelConfig == 0);

			Info->pData = m_PCMBuffer.GetBuffer();
			Info->SampleCount = pStreamInfo->frameSize;
			Info->Info = m_AudioInfo;
			Info->Discontinuity = m_DecodeError;

			OK = true;
			break;
		} else {
			// エラー発生
			LIBISDB_TRACE(
				LIBISDB_STR("aacDecoder_DecodeFrame() error {:#X}\n"),
				static_cast<std::underlying_type_t<AAC_DECODER_ERROR>>(Err));

			// リセットする
			//ResetDecoder();
		}
	}

	return OK;
}


}	// namespace LibISDB::DirectShow
