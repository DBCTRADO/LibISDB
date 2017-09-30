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
 @file   AudioDecoderFilter.hpp
 @brief  音声デコーダフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_AUDIO_DECODER_FILTER_H
#define LIBISDB_AUDIO_DECODER_FILTER_H


#include "AudioDecoder.hpp"
#include "../DirectShowBase.hpp"
#include "../../../../Base/DataBuffer.hpp"
#include "../../../../Base/SIMD.hpp"
#include "../../../../Base/EventListener.hpp"
#include <memory>


namespace LibISDB::DirectShow
{

	/** 音声デコーダフィルタクラス */
	class __declspec(uuid("2AD583EC-1D57-4d0d-8991-487F2A0A0E8B")) AudioDecoderFilter
		: public CTransformFilter
	{
	public:
		static IBaseFilter * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

		DECLARE_IUNKNOWN

	// CTransformFilter
		HRESULT CheckInputType(const CMediaType *mtIn) override;
		HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) override;
		HRESULT DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop) override;
		HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
		HRESULT StartStreaming() override;
		HRESULT StopStreaming() override;
		HRESULT BeginFlush() override;
		HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) override;
		HRESULT Receive(IMediaSample *pSample) override;

	// AudioDecoderFilter
		static constexpr BYTE ChannelCount_DualMono = 0x00;
		static constexpr BYTE ChannelCount_Invalid  = 0xFF;

		enum class DecoderType {
			Invalid,
			AAC,
			MPEGAudio,
			AC3,
		};

		enum class DualMonoMode {
			Invalid,
			Main,
			Sub,
			Both
		};

		enum class StereoMode {
			Stereo,
			Left,
			Right
		};

		enum class SPDIFMode {
			Disabled,
			Passthrough,
			Auto
		};

		struct SPDIFOptions {
			static constexpr UINT Channel_Mono     = 0x01U;
			static constexpr UINT Channel_Stereo   = 0x02U;
			static constexpr UINT Channel_DualMono = 0x04U;
			static constexpr UINT Channel_Surround = 0x08U;

			SPDIFMode Mode = SPDIFMode::Disabled;
			UINT PassthroughChannels = 0;

			SPDIFOptions() = default;
			SPDIFOptions(SPDIFMode mode, UINT channels) noexcept
				: Mode(mode), PassthroughChannels(channels) {}
			bool operator == (const SPDIFOptions &rhs) const noexcept {
				return (Mode == rhs.Mode) && (PassthroughChannels == rhs.PassthroughChannels);
			}
			bool operator != (const SPDIFOptions &rhs) const noexcept { return !(*this == rhs); }
		};

		struct SurroundMixingMatrix {
			double Matrix[6][6];
		};

		struct DownMixMatrix {
			double Matrix[2][6];
		};

		class EventListener : public LibISDB::EventListener
		{
		public:
			virtual void OnSPDIFPassthroughError(HRESULT hr) {}
		};

		class SampleCallback
		{
		public:
			virtual ~SampleCallback() = default;
			virtual void OnSamples(short *pData, size_t Length, int Channels) = 0;
		};

		bool SetDecoderType(DecoderType Type);
		uint8_t GetCurrentChannelCount() const;
		bool SetDualMonoMode(DualMonoMode Mode);
		DualMonoMode GetDualMonoMode() const noexcept { return m_DualMonoMode; }
		bool SetStereoMode(StereoMode Mode);
		StereoMode GetStereoMode() const noexcept { return m_StereoMode; }
		bool SetDownMixSurround(bool DownMix);
		bool GetDownMixSurround() const noexcept { return m_DownMixSurround; }
		bool SetSurroundMixingMatrix(const SurroundMixingMatrix *pMatrix);
		bool SetDownMixMatrix(const DownMixMatrix *pMatrix);
		bool SetGainControl(bool GainControl, float Gain = 1.0f, float SurroundGain = 1.0f);
		bool GetGainControl(float *pGain = nullptr, float *pSurroundGain = nullptr) const;
		bool SetJitterCorrection(bool Enable);
		bool GetJitterCorrection() const noexcept { return m_JitterCorrection; }
		bool SetDelay(LONGLONG Delay);
		LONGLONG GetDelay() const noexcept { return m_Delay; }
		bool SetSPDIFOptions(const SPDIFOptions &Options);
		bool GetSPDIFOptions(SPDIFOptions *pOptions) const;
		bool IsSPDIFPassthrough() const noexcept { return m_Passthrough; }

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);
		bool SetSampleCallback(SampleCallback *pCallback);

	private:
		AudioDecoderFilter(LPUNKNOWN pUnk, HRESULT *phr);
		~AudioDecoderFilter();

	// CTransformFilter
		HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut) override;

	// AudioDecoderFilter
		struct FrameSampleInfo {
			DataBuffer *pData;
			size_t SampleCount;
			bool MediaTypeChanged;
			CMediaType MediaType;
			long MediaBufferSize;
		};

		HRESULT OnFrame(
			const uint8_t *pData, size_t Samples,
			const AudioDecoder::AudioInfo &Info,
			FrameSampleInfo *pSampleInfo);
		HRESULT ProcessPCM(
			const uint8_t *pData, size_t Samples,
			const AudioDecoder::AudioInfo &Info,
			FrameSampleInfo *pSampleInfo);
		HRESULT ProcessSPDIF(
			const AudioDecoder::AudioInfo &Info,
			FrameSampleInfo *pSampleInfo);
		HRESULT ReconnectOutput(long BufferSize, const CMediaType &mt);
		void ResetSync();
		size_t MonoToStereo(int16_t *pDst, const int16_t *pSrc, size_t Samples);
		size_t DownMixStereo(int16_t *pDst, const int16_t *pSrc, size_t Samples);
		size_t DownMixSurround(int16_t *pDst, const int16_t *pSrc, size_t Samples);
		size_t MapSurroundChannels(int16_t *pDst, const int16_t *pSrc, size_t Samples);
		void GainControl(int16_t *pBuffer, size_t Samples, float Gain);
		void SelectDualMonoStereoMode();
		AudioDecoder * CreateDecoder(DecoderType Type);

		DecoderType m_DecoderType;
		std::unique_ptr<AudioDecoder> m_Decoder;
		mutable CCritSec m_cPropLock;
		CMediaType m_MediaType;
		SSEDataBuffer m_OutData;
		uint8_t m_CurChannelNum;
		bool m_DualMono;

		DualMonoMode m_DualMonoMode;
		StereoMode m_StereoMode;
		bool m_DownMixSurround;
		bool m_EnableCustomMixingMatrix;
		SurroundMixingMatrix m_MixingMatrix;
		bool m_EnableCustomDownMixMatrix;
		DownMixMatrix m_DownMixMatrix;

		bool m_GainControl;
		float m_Gain;
		float m_SurroundGain;

		bool m_JitterCorrection;
		LONGLONG m_Delay;
		LONGLONG m_DelayAdjustment;
		REFERENCE_TIME m_StartTime;
		LONGLONG m_SampleCount;
		bool m_Discontinuity;
		bool m_InputDiscontinuity;

		SPDIFOptions m_SPDIFOptions;
		bool m_Passthrough;
		bool m_PassthroughError;

		EventListenerList<EventListener> m_EventListenerList;

		SampleCallback *m_pSampleCallback;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_AUDIO_DECODER_FILTER_H
