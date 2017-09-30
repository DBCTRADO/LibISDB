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
 @file   ViewerFilter.hpp
 @brief  ビューアフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_VIEWER_FILTER_H
#define LIBISDB_VIEWER_FILTER_H


#include "../../Filters/FilterBase.hpp"
#include "DirectShow/DirectShowFilterGraph.hpp"
#include "DirectShow/AudioDecoders/AudioDecoderFilter.hpp"
#include "DirectShow/VideoRenderers/VideoRenderer.hpp"
#include "DirectShow/ImageMixers/ImageMixer.hpp"
#include "DirectShow/VideoParsers/VideoParser.hpp"
#include "DirectShow/KnownDecoderManager.hpp"
#include <memory>
#include <bdaiface.h>


namespace LibISDB
{

	namespace DirectShow
	{
		class TSSourceFilter;
	}

	/** ビューアフィルタクラス */
	class ViewerFilter
		: public SingleInputFilter
		, public DirectShow::FilterGraph
		, protected DirectShow::AudioDecoderFilter::EventListener
		, protected DirectShow::VideoParser::EventListener
	{
	public:
		/** イベントリスナ */
		class EventListener : public LibISDB::EventListener
		{
		public:
			virtual void OnVideoSizeChanged(ViewerFilter *pViewer, const DirectShow::VideoParser::VideoInfo &Info) {}
			virtual void OnFilterGraphInitialize(ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnFilterGraphInitialized(ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnFilterGraphFinalize(ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnFilterGraphFinalized(ViewerFilter *pViewer, IGraphBuilder *pGraphBuilder) {}
			virtual void OnSPDIFPassthroughError(ViewerFilter *pViewer, HRESULT hr) {}
		};

		/** オープン設定 */
		struct OpenSettings {
			HWND hwndRender;
			HWND hwndMessageDrain;
			DirectShow::VideoRenderer::RendererType VideoRenderer = DirectShow::VideoRenderer::RendererType::Default;
			uint8_t VideoStreamType = STREAM_TYPE_INVALID;
			LPCWSTR pszVideoDecoder = nullptr;
			LPCWSTR pszAudioDevice = nullptr;
			std::vector<String> AudioFilterList;
		};

		struct ClippingInfo {
			int Left = 0;
			int Right = 0;
			int HorzFactor = 0;
			int Top = 0;
			int Bottom = 0;
			int VertFactor = 0;

			void Reset() noexcept { *this = ClippingInfo(); }
		};

		enum class ViewStretchMode {
			KeepAspectRatio, // アスペクト比保持
			Crop,            // 全体表示(収まらない分はカット)
			Fit,             // ウィンドウサイズに合わせる
		};

		enum class PropertyFilterType {
			VideoDecoder,
			VideoRenderer,
			MPEG2Demultiplexer,
			AudioFilter,
			AudioRenderer,
		};

		ViewerFilter();
		~ViewerFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("ViewerFilter"); }

	// FilterBase
		void Finalize() override;
		void Reset() override;
		void SetActiveVideoPID(uint16_t PID, bool ServiceChanged) override;
		void SetActiveAudioPID(uint16_t PID, bool ServiceChanged) override;

	// SingleInputFilter
		bool ProcessData(DataStream *pData) override;

	// FilterGraph
		bool Play() override;
		bool Stop() override;
		bool Pause() override;

	// ViewerFilter
		bool OpenViewer(const OpenSettings &Settings);
		void CloseViewer();
		bool IsOpen() const;

		bool Flush();

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

		bool SetVisible(bool Visible);
		void HideCursor(bool Hide);
		bool RepaintVideo(HWND hwnd, HDC hdc);
		bool DisplayModeChanged();

		void Set1SegMode(bool OneSeg);
		bool Get1SegMode() const;

		uint16_t GetVideoPID() const;
		uint16_t GetAudioPID() const;

		bool SetViewSize(int Width, int Height);
		bool GetVideoSize(ReturnArg<int> Width, ReturnArg<int> Height) const;
		bool GetOriginalVideoSize(ReturnArg<int> Width, ReturnArg<int> Height) const;
		bool GetCroppedVideoSize(ReturnArg<int> Width, ReturnArg<int> Height) const;
		bool GetSourceRect(ReturnArg<RECT> Rect) const;
		bool GetDestRect(ReturnArg<RECT> Rect) const;
		bool GetDestSize(ReturnArg<int> Width, ReturnArg<int> Height) const;

		bool GetVideoAspectRatio(ReturnArg<int> AspectRatioX, ReturnArg<int> AspectRatioY) const;
		bool SetForcedAspectRatio(int AspectX, int AspectY);
		bool GetForcedAspectRatio(ReturnArg<int> AspectX, ReturnArg<int> AspectY) const;
		bool GetEffectiveAspectRatio(ReturnArg<int> AspectX, ReturnArg<int> AspectY) const;

		bool SetPanAndScan(int AspectX, int AspectY, const ClippingInfo *pClipping = nullptr);
		bool GetClippingInfo(ReturnArg<ClippingInfo> Clipping) const;
		bool SetViewStretchMode(ViewStretchMode Mode);
		ViewStretchMode GetViewStretchMode() const noexcept { return m_ViewStretchMode; }
		bool SetNoMaskSideCut(bool NoMask, bool Adjust = true);
		bool GetNoMaskSideCut() const noexcept { return m_NoMaskSideCut; }
		bool SetIgnoreDisplayExtension(bool Ignore);
		bool GetIgnoreDisplayExtension() const noexcept { return m_IgnoreDisplayExtension; }
		bool SetClipToDevice(bool Clip);

		COMPointer<IBaseFilter> GetVideoDecoderFilter() const;
		void SetVideoDecoderSettings(const DirectShow::KnownDecoderManager::VideoDecoderSettings &Settings);
		bool GetVideoDecoderSettings(DirectShow::KnownDecoderManager::VideoDecoderSettings *pSettings) const;
		void SaveVideoDecoderSettings();

		static constexpr uint8_t AudioChannelCount_DualMono = DirectShow::AudioDecoderFilter::ChannelCount_DualMono;
		static constexpr uint8_t AudioChannelCount_Invalid  = DirectShow::AudioDecoderFilter::ChannelCount_Invalid;
		uint8_t GetAudioChannelCount() const;
		bool SetDualMonoMode(DirectShow::AudioDecoderFilter::DualMonoMode Mode);
		DirectShow::AudioDecoderFilter::DualMonoMode GetDualMonoMode() const;
		bool SetStereoMode(DirectShow::AudioDecoderFilter::StereoMode Mode);
		DirectShow::AudioDecoderFilter::StereoMode GetStereoMode() const;
		bool SetSPDIFOptions(const DirectShow::AudioDecoderFilter::SPDIFOptions &Options);
		bool GetSPDIFOptions(DirectShow::AudioDecoderFilter::SPDIFOptions *pOptions) const;
		bool IsSPDIFPassthrough() const;
		bool SetDownMixSurround(bool DownMix);
		bool GetDownMixSurround() const;
		bool SetAudioGainControl(bool EnableGainControl, float Gain = 1.0f, float SurroundGain = 1.0f);
		bool SetAudioDelay(LONGLONG Delay);
		LONGLONG GetAudioDelay() const;
		COMPointer<DirectShow::AudioDecoderFilter> GetAudioDecoderFilter() const;
		bool SetAudioStreamType(uint8_t StreamType);

		bool GetVideoDecoderName(String *pName) const;
		bool GetVideoRendererName(String *pName) const;
		bool GetAudioRendererName(String *pName) const;
		DirectShow::VideoRenderer::RendererType GetVideoRendererType() const;
		uint8_t GetVideoStreamType() const;

		bool DisplayFilterProperty(HWND hwndOwner, PropertyFilterType Filter, int Index = 0);
		bool FilterHasProperty(PropertyFilterType Filter, int Index = 0) const;

		bool SetUseAudioRendererClock(bool Use);
		bool GetUseAudioRendererClock() const noexcept { return m_UseAudioRendererClock; }
		bool SetAdjustAudioStreamTime(bool Adjust);
		bool SetAudioSampleCallback(DirectShow::AudioDecoderFilter::SampleCallback *pCallback);
		void SetVideoStreamCallback(DirectShow::VideoParser::StreamCallback *pCallback);
		COMMemoryPointer<> GetCurrentImage();

		bool DrawText(LPCTSTR pszText, int x, int y, HFONT hfont, COLORREF Color, int Opacity);
		bool IsDrawTextSupported() const;
		bool ClearOSD();

		bool EnablePTSSync(bool Enable);
		bool IsPTSSyncEnabled() const;
		bool SetAdjust1SegVideoSample(bool AdjustTime, bool AdjustFrameRate);
		void ResetBuffer();
		bool SetBufferSize(size_t Size);
		bool SetInitialPoolPercentage(int Percentage);
		int GetBufferFillPercentage() const;
		bool SetPacketInputWait(DWORD Wait);

	protected:
		bool AdjustVideoPosition();
		bool CalcSourceRect(ReturnArg<RECT> Rect) const;

		void ConnectVideoDecoder(
			LPCTSTR pszCodecName, const GUID &MediaSubType,
			LPCTSTR pszDecoderName, COMPointer<IPin> *pOutputPin);

		bool MapVideoPID(uint16_t PID);
		bool MapAudioPID(uint16_t PID);
		void ApplyAdjustVideoSampleOptions();
		void SetAudioDecoderType(BYTE StreamType);

	// AudioDecoderFilter::EventListener
		void OnSPDIFPassthroughError(HRESULT hr) override;

	// VideoParser::EventListener
		void OnVideoInfoUpdated(const DirectShow::VideoParser::VideoInfo &Info) override;

		bool m_IsOpen;

		// ソースフィルタ
		COMPointer<DirectShow::TSSourceFilter> m_SourceFilter;
		// 映像デコーダ
		COMPointer<IBaseFilter> m_VideoDecoderFilter;
		// 音声デコーダ
		COMPointer<DirectShow::AudioDecoderFilter> m_AudioDecoder;
		// 音声フィルタ
		std::vector<COMPointer<IBaseFilter>> m_AudioFilterList;
		// 映像レンダラ
		std::unique_ptr<DirectShow::VideoRenderer> m_VideoRenderer;
		// 音声レンダラ
		COMPointer<IBaseFilter> m_AudioRenderer;
		// 映像パーサ
		COMPointer<IBaseFilter> m_VideoParserFilter;
		DirectShow::VideoParser *m_pVideoParser;

		// MPEG2Demultiplexerインタフェース
		COMPointer<IBaseFilter> m_MPEG2DemuxerFilter;

		// PIDマップ
		COMPointer<IMPEG2PIDMap> m_MPEG2DemuxerVideoMap;
		COMPointer<IMPEG2PIDMap> m_MPEG2DemuxerAudioMap;

		std::unique_ptr<DirectShow::ImageMixer> m_ImageMixer;

		uint16_t m_VideoPID;
		uint16_t m_AudioPID;
		uint16_t m_MapAudioPID;

		SIZE m_VideoWindowSize;
		DirectShow::VideoParser::VideoInfo m_VideoInfo;

		mutable MutexLock m_ResizeLock;
		DirectShow::VideoRenderer::RendererType m_VideoRendererType;
		String m_VideoDecoderName;
		String m_AudioRendererName;
		uint8_t m_VideoStreamType;
		uint8_t m_AudioStreamType;
		int m_ForcedAspectX;
		int m_ForcedAspectY;
		ClippingInfo m_Clipping;
		ViewStretchMode m_ViewStretchMode;
		bool m_NoMaskSideCut;
		bool m_IgnoreDisplayExtension;
		bool m_ClipToDevice;
		bool m_UseAudioRendererClock;
		bool m_1SegMode;
		bool m_AdjustAudioStreamTime;
		bool m_EnablePTSSync;
		bool m_Adjust1SegVideoSampleTime;
		bool m_Adjust1SegFrameRate;
		size_t m_BufferSize;
		int m_InitialPoolPercentage;
		DWORD m_PacketInputWait;
		EventListenerList<EventListener> m_EventListenerList;
		DirectShow::VideoParser::StreamCallback *m_pVideoStreamCallback;
		DirectShow::AudioDecoderFilter::SampleCallback *m_pAudioSampleCallback;
		DirectShow::KnownDecoderManager m_KnownDecoderManager;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_VIEWER_FILTER_H
