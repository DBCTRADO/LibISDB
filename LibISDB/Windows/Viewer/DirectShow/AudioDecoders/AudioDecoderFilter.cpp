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
 @file   AudioDecoderFilter.cpp
 @brief  音声デコーダフィルタ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "AudioDecoderFilter.hpp"
#include "AACDecoder.hpp"
#include "MPEGAudioDecoder.hpp"
#include "AC3Decoder.hpp"
#include <mmreg.h>
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{

namespace
{


// デフォルト周波数(48kHz)
constexpr int FREQUENCY = 48000;

// フレーム当たりのサンプル数(最大)
// AAC        : 1024
// MPEG Audio : 1152
// AC-3       : 1536 == 256 * 6
constexpr int SAMPLES_PER_FRAME = 256 * 6;

// REFERENCE_TIMEの一秒
constexpr REFERENCE_TIME REFERENCE_TIME_SECOND = 10000000LL;

// サンプルのバッファサイズ
// (フレーム当たりのサンプル数 * 16ビット * 5.1ch)
constexpr long SAMPLE_BUFFER_SIZE = SAMPLES_PER_FRAME * 2 * 6;

// MediaSample用に確保するバッファ数
constexpr long NUM_SAMPLE_BUFFERS = 4;

// ジッタの許容上限
constexpr REFERENCE_TIME MAX_JITTER = REFERENCE_TIME_SECOND / 5LL;


template<typename T> constexpr int16_t ClampSample16(T v)
{
	return v > std::numeric_limits<int16_t>::max() ? std::numeric_limits<int16_t>::max() :
	       v < std::numeric_limits<int16_t>::min() ? std::numeric_limits<int16_t>::min() :
	       static_cast<T>(v);
}


}	// namespace




AudioDecoderFilter::AudioDecoderFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(TEXT("Audio Decoder Filter"), pUnk, __uuidof(AudioDecoderFilter))
	, m_DecoderType(DecoderType::Invalid)
	, m_CurChannelNum(0)
	, m_DualMono(false)

	, m_DualMonoMode(DualMonoMode::Main)
	, m_StereoMode(StereoMode::Stereo)
	, m_DownMixSurround(true)
	, m_EnableCustomMixingMatrix(false)
	, m_EnableCustomDownMixMatrix(false)

	, m_GainControl(false)
	, m_Gain(1.0f)
	, m_SurroundGain(1.0f)

	, m_JitterCorrection(false)
	, m_Delay(0)
	, m_DelayAdjustment(0)
	, m_StartTime(-1)
	, m_SampleCount(0)
	, m_Discontinuity(true)
	, m_InputDiscontinuity(true)

	, m_Passthrough(false)
	, m_PassthroughError(false)

	, m_pSampleCallback(nullptr)
{
	LIBISDB_TRACE(LIBISDB_STR("AudioDecoderFilter::AudioDecoderFilter %p\n"), this);

	*phr = S_OK;

	// メディアタイプ設定
	m_MediaType.InitMediaType();
	m_MediaType.SetType(&MEDIATYPE_Audio);
	m_MediaType.SetSubtype(&MEDIASUBTYPE_PCM);
	m_MediaType.SetTemporalCompression(FALSE);
	m_MediaType.SetSampleSize(0);
	m_MediaType.SetFormatType(&FORMAT_WaveFormatEx);

	 // フォーマット構造体確保
#if 1
	// 2ch
	WAVEFORMATEX *pWaveInfo = reinterpret_cast<WAVEFORMATEX *>(m_MediaType.AllocFormatBuffer(sizeof(WAVEFORMATEX)));
	if (pWaveInfo == nullptr) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	// WAVEFORMATEX構造体設定(48kHz 16bit ステレオ固定)
	pWaveInfo->wFormatTag = WAVE_FORMAT_PCM;
	pWaveInfo->nChannels = 2;
	pWaveInfo->nSamplesPerSec = FREQUENCY;
	pWaveInfo->wBitsPerSample = 16;
	pWaveInfo->nBlockAlign = pWaveInfo->wBitsPerSample * pWaveInfo->nChannels / 8;
	pWaveInfo->nAvgBytesPerSec = pWaveInfo->nSamplesPerSec * pWaveInfo->nBlockAlign;
	pWaveInfo->cbSize = 0;
#else
	// 5.1ch
	WAVEFORMATEXTENSIBLE *pWaveInfo = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(m_MediaType.AllocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE)));
	if (pWaveInfo == nullptr) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	// WAVEFORMATEXTENSIBLE構造体設定(48KHz 16bit 5.1ch固定)
	pWaveInfo->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	pWaveInfo->Format.nChannels = 6;
	pWaveInfo->Format.nSamplesPerSec = FREQUENCY;
	pWaveInfo->Format.wBitsPerSample = 16;
	pWaveInfo->Format.nBlockAlign = pWaveInfo->Format.wBitsPerSample * pWaveInfo->Format.nChannels / 8;
	pWaveInfo->Format.nAvgBytesPerSec = pWaveInfo->Format.nSamplesPerSec * pWaveInfo->Format.nBlockAlign;
	pWaveInfo->Format.cbSize  = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	pWaveInfo->dwChannelMask =
		SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
		SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
		SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
	pWaveInfo->Samples.wValidBitsPerSample = 16;
	pWaveInfo->SubFormat = MEDIASUBTYPE_PCM;
#endif

	if (m_OutData.AllocateBuffer(SAMPLE_BUFFER_SIZE) < SAMPLE_BUFFER_SIZE)
		*phr = E_OUTOFMEMORY;
}


AudioDecoderFilter::~AudioDecoderFilter()
{
	LIBISDB_TRACE(LIBISDB_STR("AudioDecoderFilter::~AudioDecoderFilter\n"));
}


IBaseFilter * WINAPI AudioDecoderFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	AudioDecoderFilter *pNewFilter = new AudioDecoderFilter(pUnk, phr);
	if (FAILED(*phr))
		goto OnError;

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_PPV_ARGS(&pFilter));
	if (FAILED(*phr))
		goto OnError;

	return pFilter;

OnError:
	delete pNewFilter;
	return nullptr;
}


HRESULT AudioDecoderFilter::CheckInputType(const CMediaType *mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	// 何でもOK

	return S_OK;
}


HRESULT AudioDecoderFilter::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Audio) {
		if (*mtOut->Subtype() == MEDIASUBTYPE_PCM) {

			// GUID_NULLではデバッグアサートが発生するのでダミーを設定して回避
			CMediaType MediaType;
			MediaType.InitMediaType();
			MediaType.SetType(&MEDIATYPE_Stream);
			MediaType.SetSubtype(&MEDIASUBTYPE_None);

			m_pInput->SetMediaType(&MediaType);

			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT AudioDecoderFilter::DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	if (pprop->cBuffers < NUM_SAMPLE_BUFFERS)
		pprop->cBuffers = NUM_SAMPLE_BUFFERS;

	if (pprop->cbBuffer < SAMPLE_BUFFER_SIZE)
		pprop->cbBuffer = SAMPLE_BUFFER_SIZE;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if (FAILED(hr))
		return hr;

	// 要求を受け入れられたか判定
	if ((Actual.cBuffers < pprop->cBuffers) || (Actual.cbBuffer < pprop->cbBuffer))
		return E_FAIL;

	return S_OK;
}


HRESULT AudioDecoderFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CheckPointer(pMediaType, E_POINTER);
	CAutoLock AutoLock(m_pLock);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}


HRESULT AudioDecoderFilter::StartStreaming()
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_Decoder) {
		m_Decoder->Open();
	}

	ResetSync();

	m_PassthroughError = false;

	return S_OK;
}


HRESULT AudioDecoderFilter::StopStreaming()
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_Decoder) {
		m_Decoder->Close();
	}

	return S_OK;
}


HRESULT AudioDecoderFilter::BeginFlush()
{
	HRESULT hr = CTransformFilter::BeginFlush();

	CAutoLock AutoLock(&m_cPropLock);

	if (m_Decoder) {
		m_Decoder->Reset();
	}

	ResetSync();

	return hr;
}


HRESULT AudioDecoderFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	HRESULT hr = CTransformFilter::NewSegment(tStart, tStop, dRate);

	CAutoLock AutoLock(&m_cPropLock);

	ResetSync();

	return hr;
}


HRESULT AudioDecoderFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	// 入力データポインタを取得する
	const size_t InSize = pIn->GetActualDataLength();
	BYTE *pInData = nullptr;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;

	{
		CAutoLock Lock(&m_cPropLock);

		if (!m_Decoder) {
			m_Decoder.reset(CreateDecoder(m_DecoderType));
			if (!m_Decoder)
				return E_UNEXPECTED;
			m_Decoder->Open();
		}

		REFERENCE_TIME rtStart, rtEnd;
		hr = pIn->GetTime(&rtStart, &rtEnd);
		if (FAILED(hr))
			rtStart = -1;
		if (pIn->IsDiscontinuity() == S_OK) {
			m_Discontinuity = true;
			m_InputDiscontinuity = true;
		} else if ((hr == S_OK) || (hr == VFW_S_NO_STOP_TIME)) {
			if (!m_JitterCorrection) {
				m_StartTime = rtStart;
			} else if ((m_StartTime >= 0)
					&& (std::llabs(rtStart - m_StartTime) > MAX_JITTER)) {
				LIBISDB_TRACE(
					LIBISDB_STR("Resync audio stream time (%lld -> %lld [%f])\n"),
					m_StartTime, rtStart,
					static_cast<double>(rtStart - m_StartTime) / static_cast<double>(REFERENCE_TIME_SECOND));
				m_StartTime = rtStart;
			}
		}
		if ((m_StartTime < 0) || m_Discontinuity) {
			LIBISDB_TRACE(LIBISDB_STR("Initialize audio stream time (%lld)\n"), rtStart);
			m_StartTime = rtStart;
		}
	}

	size_t InDataPos = 0;
	FrameSampleInfo SampleInfo;
	SampleInfo.pData = &m_OutData;

	hr = S_OK;

	while (InDataPos < InSize) {
		AudioDecoder::DecodeFrameInfo FrameInfo;

		{
			CAutoLock Lock(&m_cPropLock);

			const size_t DataSize = InSize - InDataPos;
			size_t DecodeSize = DataSize;
			if (!m_Decoder->Decode(&pInData[InDataPos], &DecodeSize, &FrameInfo)) {
				if (DecodeSize < DataSize) {
					InDataPos += DecodeSize;
					continue;
				}
				break;
			}
			InDataPos += DecodeSize;

			if (FrameInfo.SampleCount == 0)
				continue;

			if (FrameInfo.Discontinuity)
				m_Discontinuity = true;

			SampleInfo.MediaTypeChanged = false;

			hr = OnFrame(FrameInfo.pData, FrameInfo.SampleCount, FrameInfo.Info, &SampleInfo);
		}

		if (SUCCEEDED(hr)) {
			if (SampleInfo.MediaTypeChanged) {
				hr = ReconnectOutput(SampleInfo.MediaBufferSize, SampleInfo.MediaType);
				if (FAILED(hr))
					break;
				LIBISDB_TRACE(LIBISDB_STR("出力メディアタイプを更新します。\n"));
				hr = m_pOutput->SetMediaType(&SampleInfo.MediaType);
				if (FAILED(hr)) {
					LIBISDB_TRACE(LIBISDB_STR("出力メディアタイプを設定できません。(%08x)\n"), hr);
					break;
				}
				m_MediaType = SampleInfo.MediaType;
				m_Discontinuity = true;
				m_InputDiscontinuity = true;
			}

			IMediaSample *pOutSample = nullptr;
			hr = m_pOutput->GetDeliveryBuffer(&pOutSample, nullptr, nullptr, 0);
			if (FAILED(hr)) {
				LIBISDB_TRACE(LIBISDB_STR("出力メディアサンプルを取得できません。(%08x)\n"), hr);
				break;
			}

			if (SampleInfo.MediaTypeChanged)
				pOutSample->SetMediaType(&m_MediaType);

			// 出力ポインタ取得
			BYTE *pOutBuff = nullptr;
			hr = pOutSample->GetPointer(&pOutBuff);
			if (FAILED(hr)) {
				LIBISDB_TRACE(LIBISDB_STR("出力サンプルのバッファを取得できません。(%08x)\n"), hr);
				pOutSample->Release();
				break;
			}

			std::memcpy(pOutBuff, m_OutData.GetData(), m_OutData.GetSize());
			pOutSample->SetActualDataLength(static_cast<long>(m_OutData.GetSize()));

			if (m_StartTime >= 0) {
				REFERENCE_TIME rtDuration, rtStart, rtEnd;
				rtDuration = REFERENCE_TIME_SECOND * static_cast<LONGLONG>(SampleInfo.SampleCount) / FrameInfo.Info.Frequency;
				rtStart = m_StartTime;
				m_StartTime += rtDuration;
				// 音ずれ補正用時間シフト
				if (m_DelayAdjustment > 0) {
					// 最大2倍まで時間を遅らせる
					if (rtDuration >= m_DelayAdjustment) {
						rtDuration += m_DelayAdjustment;
						m_DelayAdjustment = 0;
					} else {
						m_DelayAdjustment -= rtDuration;
						rtDuration *= 2;
					}
				} else if (m_DelayAdjustment < 0) {
					// 最短1/2まで時間を早める
					if (rtDuration >= -m_DelayAdjustment * 2) {
						rtDuration += m_DelayAdjustment;
						m_DelayAdjustment = 0;
					} else {
						m_DelayAdjustment += rtDuration;
						rtDuration /= 2;
					}
				} else {
					rtStart += m_Delay;
				}
				rtEnd = rtStart + rtDuration;
				pOutSample->SetTime(&rtStart, &rtEnd);
			}
			pOutSample->SetMediaTime(nullptr, nullptr);
			pOutSample->SetPreroll(FALSE);
#if 0
			// Discontinuityを設定すると倍速再生がおかしくなる模様
			pOutSample->SetDiscontinuity(m_Discontinuity);
#else
			pOutSample->SetDiscontinuity(m_InputDiscontinuity);
#endif
			m_Discontinuity = false;
			m_InputDiscontinuity = false;
			pOutSample->SetSyncPoint(TRUE);

			hr = m_pOutput->Deliver(pOutSample);
#ifdef _DEBUG
			if (FAILED(hr)) {
				LIBISDB_TRACE(LIBISDB_STR("サンプルを送信できません。(%08x)\n"), hr);
				if (m_Passthrough && !m_PassthroughError) {
					m_PassthroughError = true;
					m_EventListenerList.CallEventListener(&EventListener::OnSPDIFPassthroughError, hr);
				}
			}
#endif
			pOutSample->Release();
			if (FAILED(hr))
				break;
		}
	}

	return hr;
}


HRESULT AudioDecoderFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	HRESULT hr;

	hr = Transform(pSample, nullptr);
	if (SUCCEEDED(hr)) {
		hr = S_OK;
		m_bSampleSkipped = FALSE;
	}

	return hr;
}


bool AudioDecoderFilter::SetDecoderType(DecoderType Type)
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_Decoder) {
		AudioDecoder *pDecoder = CreateDecoder(Type);
		if (!pDecoder)
			return false;
		m_Decoder.reset(pDecoder);
		m_Decoder->Open();
	}

	m_DecoderType = Type;

	return true;
}


uint8_t AudioDecoderFilter::GetCurrentChannelCount() const
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_CurChannelNum == 0)
		return ChannelCount_Invalid;
	if (m_DualMono)
		return ChannelCount_DualMono;
	return m_CurChannelNum;
}


bool AudioDecoderFilter::SetDualMonoMode(DualMonoMode Mode)
{
	CAutoLock AutoLock(&m_cPropLock);

	switch (Mode) {
	case DualMonoMode::Invalid:
	case DualMonoMode::Main:
	case DualMonoMode::Sub:
	case DualMonoMode::Both:
		LIBISDB_TRACE(LIBISDB_STR("AudioDecoderFilter::SetDualMonoMode() : Mode %d\n"), Mode);
		m_DualMonoMode = Mode;
		if (m_DualMono)
			SelectDualMonoStereoMode();
		return true;
	}

	return false;
}


bool AudioDecoderFilter::SetStereoMode(StereoMode Mode)
{
	CAutoLock AutoLock(&m_cPropLock);

	switch (Mode) {
	case StereoMode::Stereo:
	case StereoMode::Left:
	case StereoMode::Right:
		m_StereoMode = Mode;
		LIBISDB_TRACE(LIBISDB_STR("AudioDecoderFilter::SetStereoMode() : Stereo mode %d\n"), Mode);
		return true;
	}

	return false;
}


bool AudioDecoderFilter::SetDownMixSurround(bool DownMix)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_DownMixSurround = DownMix;
	return true;
}


bool AudioDecoderFilter::SetSurroundMixingMatrix(const SurroundMixingMatrix *pMatrix)
{
	CAutoLock AutoLock(&m_cPropLock);

	if (pMatrix) {
		m_EnableCustomMixingMatrix = true;
		m_MixingMatrix = *pMatrix;
	} else {
		m_EnableCustomMixingMatrix = false;
	}

	return true;
}


bool AudioDecoderFilter::SetDownMixMatrix(const DownMixMatrix *pMatrix)
{
	CAutoLock AutoLock(&m_cPropLock);

	if (pMatrix) {
		m_EnableCustomDownMixMatrix = true;
		m_DownMixMatrix = *pMatrix;
	} else {
		m_EnableCustomDownMixMatrix = false;
	}

	return true;
}


bool AudioDecoderFilter::SetGainControl(bool GainControl, float Gain, float SurroundGain)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_GainControl = GainControl;
	m_Gain = Gain;
	m_SurroundGain = SurroundGain;
	return true;
}


bool AudioDecoderFilter::GetGainControl(float *pGain, float *pSurroundGain) const
{
	CAutoLock AutoLock(&m_cPropLock);

	if (pGain)
		*pGain = m_Gain;
	if (pSurroundGain)
		*pSurroundGain = m_SurroundGain;
	return m_GainControl;
}


bool AudioDecoderFilter::SetSPDIFOptions(const SPDIFOptions &Options)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_SPDIFOptions = Options;

	return true;
}


bool AudioDecoderFilter::GetSPDIFOptions(SPDIFOptions *pOptions) const
{
	if (!pOptions)
		return false;

	CAutoLock AutoLock(&m_cPropLock);

	*pOptions = m_SPDIFOptions;

	return true;
}


bool AudioDecoderFilter::SetJitterCorrection(bool Enable)
{
	CAutoLock AutoLock(&m_cPropLock);

	if (m_JitterCorrection != Enable) {
		m_JitterCorrection = Enable;

		m_StartTime = -1;
		m_SampleCount = 0;
	}

	return true;
}


bool AudioDecoderFilter::SetDelay(LONGLONG Delay)
{
	CAutoLock AutoLock(&m_cPropLock);

	LIBISDB_TRACE(LIBISDB_STR("AudioDecoderFilter::SetDelay() : %lld\n"), Delay);

	m_DelayAdjustment += Delay - m_Delay;
	m_Delay = Delay;

	return true;
}


bool AudioDecoderFilter::SetSampleCallback(SampleCallback *pCallback)
{
	CAutoLock AutoLock(&m_cPropLock);

	m_pSampleCallback = pCallback;

	return true;
}


bool AudioDecoderFilter::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool AudioDecoderFilter::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


HRESULT AudioDecoderFilter::OnFrame(
	const uint8_t *pData, size_t Samples,
	const AudioDecoder::AudioInfo &Info,
	FrameSampleInfo *pSampleInfo)
{
	if (Info.ChannelCount != 1 && Info.ChannelCount != 2 && Info.ChannelCount != 6)
		return E_FAIL;

	const bool DualMono = (Info.ChannelCount == 2 && Info.DualMono);

	bool Passthrough = false;
	if (m_Decoder->IsSPDIFSupported()) {
		if (m_SPDIFOptions.Mode == SPDIFMode::Passthrough) {
			Passthrough = true;
		} else if (m_SPDIFOptions.Mode == SPDIFMode::Auto) {
			UINT ChannelFlag;
			if (DualMono) {
				ChannelFlag = SPDIFOptions::Channel_DualMono;
			} else {
				switch (Info.ChannelCount) {
				case 1: ChannelFlag = SPDIFOptions::Channel_Mono;     break;
				case 2: ChannelFlag = SPDIFOptions::Channel_Stereo;   break;
				case 6: ChannelFlag = SPDIFOptions::Channel_Surround; break;
				}
			}
			if ((m_SPDIFOptions.PassthroughChannels & ChannelFlag) != 0)
				Passthrough = true;
		}
	}

	if (m_Passthrough != Passthrough)
		m_PassthroughError = false;
	m_Passthrough = Passthrough;

	if (DualMono != m_DualMono) {
		m_DualMono = DualMono;
		if (DualMono) {
			SelectDualMonoStereoMode();
		} else {
			m_StereoMode = StereoMode::Stereo;
		}
	}

	m_CurChannelNum = Info.OriginalChannelCount;

	HRESULT hr;

	if (m_Passthrough) {
		hr = ProcessSPDIF(Info, pSampleInfo);
	} else {
		hr = ProcessPCM(pData, Samples, Info, pSampleInfo);
	}

	return hr;
}


HRESULT AudioDecoderFilter::ProcessPCM(
	const uint8_t *pData, size_t Samples,
	const AudioDecoder::AudioInfo &Info,
	FrameSampleInfo *pSampleInfo)
{
	const bool Surround = (Info.ChannelCount == 6 && !m_DownMixSurround);
	const int OutChannels = Surround ? 6 : 2;

	// メディアタイプの更新
	bool MediaTypeChanged = false;
	WAVEFORMATEX *pwfx = reinterpret_cast<WAVEFORMATEX *>(m_MediaType.Format());
	if ((*m_MediaType.FormatType() != FORMAT_WaveFormatEx)
			|| (!Surround && (pwfx->wFormatTag != WAVE_FORMAT_PCM))
			|| (Surround && (pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE))
			|| (pwfx->nSamplesPerSec != Info.Frequency)) {
		CMediaType &mt = pSampleInfo->MediaType;
		mt.SetType(&MEDIATYPE_Audio);
		mt.SetSubtype(&MEDIASUBTYPE_PCM);
		mt.SetFormatType(&FORMAT_WaveFormatEx);

		pwfx = reinterpret_cast<WAVEFORMATEX *>(
			mt.AllocFormatBuffer(Surround ? sizeof (WAVEFORMATEXTENSIBLE) : sizeof(WAVEFORMATEX)));
		if (pwfx == nullptr)
			return E_OUTOFMEMORY;
		if (!Surround) {
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->nChannels = 2;
			pwfx->cbSize = 0;
		} else {
			WAVEFORMATEXTENSIBLE *pExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(pwfx);
			pExtensible->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			pExtensible->Format.nChannels = 6;
			pExtensible->Format.cbSize  = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			pExtensible->dwChannelMask =
				SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
				SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
				SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
			pExtensible->Samples.wValidBitsPerSample = 16;
			pExtensible->SubFormat = MEDIASUBTYPE_PCM;
		}
		pwfx->nSamplesPerSec = Info.Frequency;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		mt.SetSampleSize(pwfx->nBlockAlign);

		pSampleInfo->MediaTypeChanged = true;
		pSampleInfo->MediaBufferSize = SAMPLES_PER_FRAME * pwfx->nBlockAlign;
	}

	const size_t BuffSize = Samples * OutChannels * sizeof(int16_t);
	if (pSampleInfo->pData->SetSize(BuffSize) < BuffSize)
		return E_OUTOFMEMORY;
	uint8_t *pOutBuff = pSampleInfo->pData->GetData();

	if (pData != nullptr) {
		size_t OutSize;

		// ダウンミックス
		switch (Info.ChannelCount) {
		case 1:
			OutSize = MonoToStereo(
				reinterpret_cast<int16_t *>(pOutBuff),
				reinterpret_cast<const int16_t *>(pData),
				Samples);
			break;

		case 2:
			OutSize = DownMixStereo(
				reinterpret_cast<int16_t *>(pOutBuff),
				reinterpret_cast<const int16_t *>(pData),
				Samples);
			break;

		case 6:
			if (Surround) {
				OutSize = MapSurroundChannels(
					reinterpret_cast<int16_t *>(pOutBuff),
					reinterpret_cast<const int16_t *>(pData),
					Samples);
			} else {
				OutSize = DownMixSurround(
					reinterpret_cast<int16_t *>(pOutBuff),
					reinterpret_cast<const int16_t *>(pData),
					Samples);
			}
			break;
		}

		if (m_GainControl && (Info.ChannelCount < 6 || Surround)) {
			GainControl(
				reinterpret_cast<int16_t *>(pOutBuff), OutSize / sizeof(int16_t),
				Surround ? m_SurroundGain : m_Gain);
		}
	} else {
		std::memset(pOutBuff, 0, BuffSize);
	}

	if (m_pSampleCallback) {
		m_pSampleCallback->OnSamples(reinterpret_cast<int16_t *>(pOutBuff), Samples, OutChannels);
	}

	pSampleInfo->SampleCount = Samples;

	return S_OK;
}


HRESULT AudioDecoderFilter::ProcessSPDIF(
	const AudioDecoder::AudioInfo &Info, FrameSampleInfo *pSampleInfo)
{
	static const int PREAMBLE_SIZE = sizeof(WORD) * 4;

	AudioDecoder::SPDIFFrameInfo FrameInfo;
	if (!m_Decoder->GetSPDIFFrameInfo(&FrameInfo))
		return E_FAIL;

	const int FrameSize = FrameInfo.FrameSize;
	const int DataBurstSize = PREAMBLE_SIZE + FrameSize;
	const int PacketSize = FrameInfo.SamplesPerFrame * 4;
	if (DataBurstSize > PacketSize) {
		LIBISDB_TRACE(
			LIBISDB_STR("S/PDIFビットレートが不正です。(Frame size %d / Data-burst size %d / Packet size %d)\n"),
			FrameSize, DataBurstSize, PacketSize);
		return E_FAIL;
	}

#ifdef LIBISDB_DEBUG
	static bool First = true;
	if (First) {
		LIBISDB_TRACE(
			LIBISDB_STR("S/PDIF出力開始(Frame size %d / Data-burst size %d / Packet size %d)\n"),
			FrameSize, DataBurstSize, PacketSize);
		First = false;
	}
#endif

	WAVEFORMATEX *pwfx = reinterpret_cast<WAVEFORMATEX*>(m_MediaType.Format());
	if ((*m_MediaType.FormatType() != FORMAT_WaveFormatEx)
			|| (pwfx->wFormatTag != WAVE_FORMAT_DOLBY_AC3_SPDIF)
			|| (pwfx->nSamplesPerSec != Info.Frequency)) {
		CMediaType &mt = pSampleInfo->MediaType;
		mt.SetType(&MEDIATYPE_Audio);
		mt.SetSubtype(&MEDIASUBTYPE_PCM);
		mt.SetFormatType(&FORMAT_WaveFormatEx);

		pwfx = reinterpret_cast<WAVEFORMATEX*>(mt.AllocFormatBuffer(sizeof(WAVEFORMATEX)));
		if (pwfx == nullptr)
			return E_OUTOFMEMORY;
		pwfx->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
		pwfx->nChannels = 2;
		pwfx->nSamplesPerSec = Info.Frequency;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		pwfx->cbSize = 0;
		/*
		// Windows 7 では WAVEFORMATEXTENSIBLE_IEC61937 を使った方がいい?
		WAVEFORMATEXTENSIBLE_IEC61937 *pwfx;
		...
		pwfx->FormatExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		...
		pwfx->FormatExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE_IEC61937) - sizeof(WAVEFORMATEX);
		pwfx->FormatExt.dwChannelMask =
			SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
			SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
			SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		pwfx->FormatExt.Samples.wValidBitsPerSample = 16;
		pwfx->FormatExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_AAC;
		pwfx->dwEncodedSamplesPerSec = Info.Frequency;
		pwfx->dwEncodedChannelCount = 6;
		pwfx->dwAverageBytesPerSec = 0;
		*/
		mt.SetSampleSize(pwfx->nBlockAlign);

		pSampleInfo->MediaTypeChanged = true;
		pSampleInfo->MediaBufferSize = PacketSize;
	}

	if (pSampleInfo->pData->SetSize(PacketSize) < static_cast<DWORD>(PacketSize))
		return E_OUTOFMEMORY;
	BYTE *pOutBuff = pSampleInfo->pData->GetData();
	WORD *pWordData = reinterpret_cast<WORD*>(pOutBuff);
	// Burst-preamble
	pWordData[0] = 0xF872;						// Pa(Sync word 1)
	pWordData[1] = 0x4E1F;						// Pb(Sync word 2)
	pWordData[2] = FrameInfo.Pc;				// Pc(Burst-info)
//	pWordData[3] = ((FrameSize + 1) & ~1) * 8;	// Pd(Length-code)
	pWordData[3] = FrameSize * 8;				// Pd(Length-code)
	// Burst-payload
	int PayloadSize = m_Decoder->GetSPDIFBurstPayload(
			&pOutBuff[PREAMBLE_SIZE],
			pSampleInfo->pData->GetBufferSize() - PREAMBLE_SIZE);
	if ((PayloadSize < 1) || (PREAMBLE_SIZE + PayloadSize > PacketSize)) {
		LIBISDB_TRACE(
			LIBISDB_STR("S/PDIF Burst-payload サイズが不正です。(Packet size %d / Payload size %d)\n"),
			PacketSize, PayloadSize);
		return E_FAIL;
	}
	// Stuffing
	PayloadSize += PREAMBLE_SIZE;
	if (PayloadSize < PacketSize)
		::ZeroMemory(&pOutBuff[PayloadSize], PacketSize - PayloadSize);

	pSampleInfo->SampleCount = FrameInfo.SamplesPerFrame;

	return S_OK;
}


HRESULT AudioDecoderFilter::ReconnectOutput(long BufferSize, const CMediaType &mt)
{
	HRESULT hr;

	IPin *pPin = m_pOutput->GetConnected();
	if (pPin == nullptr)
		return E_POINTER;

	IMemInputPin *pMemInputPin = nullptr;
	hr = pPin->QueryInterface(IID_IMemInputPin, reinterpret_cast<void**>(&pMemInputPin));
	if (FAILED(hr)) {
		LIBISDB_TRACE(LIBISDB_STR("IMemInputPinインターフェースが取得できません。(%08x)\n"), hr);
	} else {
		IMemAllocator *pAllocator = nullptr;
		hr = pMemInputPin->GetAllocator(&pAllocator);
		if (FAILED(hr)) {
			LIBISDB_TRACE(LIBISDB_STR("IMemAllocatorインターフェースが取得できません。(%08x)\n"), hr);
		} else {
			ALLOCATOR_PROPERTIES Props;
			hr = pAllocator->GetProperties(&Props);
			if (FAILED(hr)) {
				LIBISDB_TRACE(LIBISDB_STR("IMemAllocatorのプロパティが取得できません。(%08x)\n"), hr);
			} else {
				if ((mt != m_pOutput->CurrentMediaType())
						|| (Props.cBuffers < NUM_SAMPLE_BUFFERS)
						|| (Props.cbBuffer < BufferSize)) {
					hr = S_OK;
					if ((Props.cBuffers < NUM_SAMPLE_BUFFERS)
							|| (Props.cbBuffer < BufferSize)) {
						ALLOCATOR_PROPERTIES ActualProps;

						Props.cBuffers = NUM_SAMPLE_BUFFERS;
						Props.cbBuffer = BufferSize * 3 / 2;
						LIBISDB_TRACE(LIBISDB_STR("バッファサイズを設定します。(%ld bytes)\n"), Props.cbBuffer);
						if (SUCCEEDED(hr = m_pOutput->DeliverBeginFlush())
								&& SUCCEEDED(hr = m_pOutput->DeliverEndFlush())
								&& SUCCEEDED(hr = pAllocator->Decommit())
								&& SUCCEEDED(hr = pAllocator->SetProperties(&Props, &ActualProps))
								&& SUCCEEDED(hr = pAllocator->Commit())) {
							if ((ActualProps.cBuffers < Props.cBuffers)
									|| (ActualProps.cbBuffer < BufferSize)) {
								LIBISDB_TRACE(LIBISDB_STR("バッファサイズの要求が受け付けられません。(%ld / %ld)\n"),
										  ActualProps.cbBuffer, Props.cbBuffer);
								hr = E_FAIL;
								NotifyEvent(EC_ERRORABORT, hr, 0);
							} else {
								LIBISDB_TRACE(LIBISDB_STR("ピンの再接続成功\n"));
								hr = S_OK;
							}
						} else {
							LIBISDB_TRACE(LIBISDB_STR("ピンの再接続ができません。(%08x)\n"), hr);
						}
					}
				} else {
					hr = S_FALSE;
				}
			}

			pAllocator->Release();
		}

		pMemInputPin->Release();
	}

	return hr;
}


void AudioDecoderFilter::ResetSync()
{
	m_DelayAdjustment = 0;
	m_StartTime = -1;
	m_SampleCount = 0;
	m_Discontinuity = true;
	m_InputDiscontinuity = true;
}


size_t AudioDecoderFilter::MonoToStereo(int16_t *pDst, const int16_t *pSrc, size_t Samples)
{
	// 1ch → 2ch 二重化
	const int16_t *p = pSrc, *pEnd = pSrc + Samples;
	int16_t *q = pDst;

#ifdef LIBISDB_SSE2_SUPPORT
	if (Samples >= 16 && IsSSE2Enabled()) {
		const int16_t *pSimdEnd = pSrc + (Samples & ~15_z);
		do {
			__m128i v1, v2, r1, r2, r3, r4;
			v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 0);
			v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 1);
			r1 = _mm_unpacklo_epi16(v1, v1);
			r2 = _mm_unpackhi_epi16(v1, v1);
			r3 = _mm_unpacklo_epi16(v2, v2);
			r4 = _mm_unpackhi_epi16(v2, v2);
			_mm_store_si128(reinterpret_cast<__m128i *>(q) + 0, r1);
			_mm_store_si128(reinterpret_cast<__m128i *>(q) + 1, r2);
			_mm_store_si128(reinterpret_cast<__m128i *>(q) + 2, r3);
			_mm_store_si128(reinterpret_cast<__m128i *>(q) + 3, r4);
			q += 32;
			p += 16;
		} while (p < pSimdEnd);
	}
#endif

	while (p < pEnd) {
		int16_t Value = *p++;
		*q++ = Value;	// L
		*q++ = Value;	// R
	}

	return Samples * (sizeof(int16_t) * 2);
}


size_t AudioDecoderFilter::DownMixStereo(int16_t *pDst, const int16_t *pSrc, size_t Samples)
{
	if (m_StereoMode == StereoMode::Stereo) {
		// 2ch → 2ch スルー
		std::memcpy(pDst, pSrc, Samples * (sizeof(int16_t) * 2));
	} else {
		const size_t Length = Samples * 2;
		const int16_t *p = pSrc, *pEnd = pSrc + Length;
		int16_t *q = pDst;

		if (m_StereoMode == StereoMode::Left) {
			// 左のみ

#ifdef LIBISDB_SSE2_SUPPORT
			if (Length >= 32 && IsSSE2Enabled()) {
				const int16_t *pSimdEnd = pSrc + (Length & ~31_z);
				do {
					__m128i v1, v2, v3, v4;
					v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 0);
					v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 1);
					v3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 2);
					v4 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 3);
					v1 = _mm_shufflelo_epi16(v1, _MM_SHUFFLE(2, 2, 0, 0));
					v2 = _mm_shufflelo_epi16(v2, _MM_SHUFFLE(2, 2, 0, 0));
					v3 = _mm_shufflelo_epi16(v3, _MM_SHUFFLE(2, 2, 0, 0));
					v4 = _mm_shufflelo_epi16(v4, _MM_SHUFFLE(2, 2, 0, 0));
					v1 = _mm_shufflehi_epi16(v1, _MM_SHUFFLE(2, 2, 0, 0));
					v2 = _mm_shufflehi_epi16(v2, _MM_SHUFFLE(2, 2, 0, 0));
					v3 = _mm_shufflehi_epi16(v3, _MM_SHUFFLE(2, 2, 0, 0));
					v4 = _mm_shufflehi_epi16(v4, _MM_SHUFFLE(2, 2, 0, 0));
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 0, v1);
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 1, v2);
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 2, v3);
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 3, v4);
					q += 32;
					p += 32;
				} while (p < pSimdEnd);
			}
#endif

			while (p < pEnd) {
				int16_t Value = *p;
				*q++ = Value;	// L
				*q++ = Value;	// R
				p += 2;
			}
		} else {
			// 右のみ

#ifdef LIBISDB_SSE2_SUPPORT
			if (Length >= 32 && IsSSE2Enabled()) {
				const int16_t *pSimdEnd = pSrc + (Length & ~31_z);
				do {
					__m128i v1, v2, v3, v4;
					v1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 0);
					v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 1);
					v3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 2);
					v4 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p) + 3);
					v1 = _mm_shufflelo_epi16(v1, _MM_SHUFFLE(3, 3, 1, 1));
					v2 = _mm_shufflelo_epi16(v2, _MM_SHUFFLE(3, 3, 1, 1));
					v3 = _mm_shufflelo_epi16(v3, _MM_SHUFFLE(3, 3, 1, 1));
					v4 = _mm_shufflelo_epi16(v4, _MM_SHUFFLE(3, 3, 1, 1));
					v1 = _mm_shufflehi_epi16(v1, _MM_SHUFFLE(3, 3, 1, 1));
					v2 = _mm_shufflehi_epi16(v2, _MM_SHUFFLE(3, 3, 1, 1));
					v3 = _mm_shufflehi_epi16(v3, _MM_SHUFFLE(3, 3, 1, 1));
					v4 = _mm_shufflehi_epi16(v4, _MM_SHUFFLE(3, 3, 1, 1));
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 0, v1);
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 1, v2);
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 2, v3);
					_mm_store_si128(reinterpret_cast<__m128i *>(q) + 3, v4);
					q += 32;
					p += 32;
				} while (p < pSimdEnd);
			}
#endif

			while (p < pEnd) {
				int16_t Value = p[1];
				*q++ = Value;	// L
				*q++ = Value;	// R
				p += 2;
			}
		}
	}

	return Samples * (sizeof(int16_t) * 2);
}


size_t AudioDecoderFilter::DownMixSurround(int16_t *pDst, const int16_t *pSrc, size_t Samples)
{
	// 5.1ch → 2ch ダウンミックス

	const double Level = m_GainControl ? m_SurroundGain : 1.0;
	int ChannelMap[6];

	if (!m_Decoder->GetChannelMap(6, ChannelMap)) {
		for (int i = 0; i < 6; i++)
			ChannelMap[i] = i;
	}

	if (m_EnableCustomDownMixMatrix) {
		// カスタムマトリックスを使用
		for (size_t Pos = 0; Pos < Samples; Pos++) {
			double Data[6];

			for (int i = 0; i < 6; i++)
				Data[i] = static_cast<double>(pSrc[Pos * 6 + ChannelMap[i]]);

			for (int i = 0; i < 2; i++) {
				int Value = static_cast<int>((
					Data[0] * m_DownMixMatrix.Matrix[i][0] +
					Data[1] * m_DownMixMatrix.Matrix[i][1] +
					Data[2] * m_DownMixMatrix.Matrix[i][2] +
					Data[3] * m_DownMixMatrix.Matrix[i][3] +
					Data[4] * m_DownMixMatrix.Matrix[i][4] +
					Data[5] * m_DownMixMatrix.Matrix[i][5]
					) * Level);
				pDst[Pos * 2 + i] = ClampSample16(Value);
			}
		}
	} else {
		// デフォルトの係数を使用
		AudioDecoder::DownmixInfo Info;
		m_Decoder->GetDownmixInfo(&Info);

		for (size_t Pos = 0; Pos < Samples; Pos++) {
			int Left = static_cast<int>((
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_FL ]]) * Info.Front +
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_BL ]]) * Info.Rear +
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_FC ]]) * Info.Center +
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_LFE]]) * Info.LFE
				) * Level);

			int Right = static_cast<int>((
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_FR ]]) * Info.Front +
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_BR ]]) * Info.Rear +
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_FC ]]) * Info.Center +
				static_cast<double>(pSrc[Pos * 6 + ChannelMap[AudioDecoder::CHANNEL_6_LFE]]) * Info.LFE
				) * Level);

			pDst[Pos * 2 + 0] = ClampSample16(Left);
			pDst[Pos * 2 + 1] = ClampSample16(Right);
		}
	}

	// バッファサイズを返す
	return Samples * (sizeof(int16_t) * 2);
}


size_t AudioDecoderFilter::MapSurroundChannels(int16_t *pDst, const int16_t *pSrc, size_t Samples)
{
	if (m_EnableCustomMixingMatrix) {
		// カスタムマトリックスを使用
		int ChannelMap[6];

		if (!m_Decoder->GetChannelMap(6, ChannelMap)) {
			for (int i = 0; i < 6; i++)
				ChannelMap[i] = i;
		}

		for (size_t i = 0 ; i < Samples ; i++) {
			double Data[6];

			for (int j = 0; j < 6; j++)
				Data[j] = static_cast<double>(pSrc[i * 6 + ChannelMap[j]]);

			for (int j = 0; j < 6; j++) {
				int Value = static_cast<int>(
					Data[0] * m_MixingMatrix.Matrix[j][0] +
					Data[1] * m_MixingMatrix.Matrix[j][1] +
					Data[2] * m_MixingMatrix.Matrix[j][2] +
					Data[3] * m_MixingMatrix.Matrix[j][3] +
					Data[4] * m_MixingMatrix.Matrix[j][4] +
					Data[5] * m_MixingMatrix.Matrix[j][5]);
				pDst[i * 6 + j] = ClampSample16(Value);
			}
		}
	} else {
		// デフォルトの割り当てを使用
		int ChannelMap[6];

		if (m_Decoder->GetChannelMap(6, ChannelMap)) {
			for (size_t i = 0 ; i < Samples ; i++) {
				pDst[i * 6 + 0] = pSrc[i * 6 + ChannelMap[AudioDecoder::CHANNEL_6_FL]];
				pDst[i * 6 + 1] = pSrc[i * 6 + ChannelMap[AudioDecoder::CHANNEL_6_FR]];
				pDst[i * 6 + 2] = pSrc[i * 6 + ChannelMap[AudioDecoder::CHANNEL_6_FC]];
				pDst[i * 6 + 3] = pSrc[i * 6 + ChannelMap[AudioDecoder::CHANNEL_6_LFE]];
				pDst[i * 6 + 4] = pSrc[i * 6 + ChannelMap[AudioDecoder::CHANNEL_6_BL]];
				pDst[i * 6 + 5] = pSrc[i * 6 + ChannelMap[AudioDecoder::CHANNEL_6_BR]];
			}
		} else {
			std::memcpy(pDst, pSrc, Samples * 6 * sizeof(int16_t));
		}
	}

	return Samples * (sizeof(int16_t) * 6);
}


void AudioDecoderFilter::GainControl(int16_t *pBuffer, size_t Samples, float Gain)
{
	static const int Factor = 0x1000;
	const int Level = static_cast<int>(Gain * static_cast<float>(Factor));

	if (Level != Factor) {
		int16_t *p, *pEnd;

		p = pBuffer;
		pEnd= p + Samples;
		while (p < pEnd) {
			int Value = (static_cast<int>(*p) * Level) / Factor;
			*p++ = ClampSample16(Value);
		}
	}
}


void AudioDecoderFilter::SelectDualMonoStereoMode()
{
	switch (m_DualMonoMode) {
	case DualMonoMode::Main: m_StereoMode = StereoMode::Left;   break;
	case DualMonoMode::Sub:  m_StereoMode = StereoMode::Right;  break;
	case DualMonoMode::Both: m_StereoMode = StereoMode::Stereo; break;
	}
}


AudioDecoder * AudioDecoderFilter::CreateDecoder(DecoderType Type)
{
	switch (Type) {
	case DecoderType::AAC:
		return new AACDecoder;

	case DecoderType::MPEGAudio:
		return new MPEGAudioDecoder;

	case DecoderType::AC3:
		return new AC3Decoder;
	}

	return nullptr;
}


}	// namespace LibISDB::DirectShow
