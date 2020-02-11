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
 @file   ViewerFilter.cpp
 @brief  ビューアフィルタ
 @author DBCTRADO
*/


#include "../../LibISDBPrivate.hpp"
#include "../../LibISDBWindows.hpp"
#include "ViewerFilter.hpp"
#include "DirectShow/SourceFilter/TSSourceFilter.hpp"
#include "DirectShow/VideoParsers/MPEG2ParserFilter.hpp"
#include "DirectShow/VideoParsers/H264ParserFilter.hpp"
#include "DirectShow/VideoParsers/H265ParserFilter.hpp"
#include "../../Utilities/StringUtilities.hpp"
#include <numeric>	// for std::gcd()
#include <dvdmedia.h>
#include "../../Base/DebugDef.hpp"

// LAV Video Decoder で一度再生を停止すると再開が正常に行われない現象の回避策を行う
#define LAV_VIDEO_DECODER_WORKAROUND


//const CLSID CLSID_NullRenderer = {0xc1f400a4, 0x3f08, 0x11d3, {0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37}};
EXTERN_C const CLSID CLSID_NullRenderer;


namespace LibISDB
{


namespace
{


const std::chrono::milliseconds LOCK_TIMEOUT(2000);


HRESULT SetVideoMediaType(CMediaType *pMediaType, BYTE VideoStreamType, int Width, int Height)
{
	static const REFERENCE_TIME TIME_PER_FRAME =
		static_cast<REFERENCE_TIME>(10000000.0 / 29.97 + 0.5);

	switch (VideoStreamType) {
	case STREAM_TYPE_MPEG2_VIDEO:
		// MPEG-2
		{
			// 映像メディアフォーマット設定
			pMediaType->InitMediaType();
			pMediaType->SetType(&MEDIATYPE_Video);
			pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_VIDEO);
			pMediaType->SetVariableSize();
			pMediaType->SetTemporalCompression(TRUE);
			pMediaType->SetSampleSize(0);
			pMediaType->SetFormatType(&FORMAT_MPEG2Video);
			// フォーマット構造体確保
			MPEG2VIDEOINFO *pVideoInfo =
				reinterpret_cast<MPEG2VIDEOINFO *>(pMediaType->AllocFormatBuffer(sizeof(MPEG2VIDEOINFO)));
			if (!pVideoInfo)
				return E_OUTOFMEMORY;
			::ZeroMemory(pVideoInfo, sizeof(MPEG2VIDEOINFO));
			// ビデオヘッダ設定
			VIDEOINFOHEADER2 &VideoHeader = pVideoInfo->hdr;
			//::SetRect(&VideoHeader.rcSource, 0, 0, Width, Height);
			VideoHeader.AvgTimePerFrame = TIME_PER_FRAME;
			VideoHeader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
			VideoHeader.bmiHeader.biWidth = Width;
			VideoHeader.bmiHeader.biHeight = Height;
		}
		break;

	case STREAM_TYPE_H264:
		// H.264
		{
			pMediaType->InitMediaType();
			pMediaType->SetType(&MEDIATYPE_Video);
			pMediaType->SetSubtype(&MEDIASUBTYPE_H264);
			pMediaType->SetVariableSize();
			pMediaType->SetTemporalCompression(TRUE);
			pMediaType->SetSampleSize(0);
			pMediaType->SetFormatType(&FORMAT_VideoInfo);
			VIDEOINFOHEADER *pVideoInfo =
				reinterpret_cast<VIDEOINFOHEADER *>(pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
			if (!pVideoInfo)
				return E_OUTOFMEMORY;
			::ZeroMemory(pVideoInfo, sizeof(VIDEOINFOHEADER));
			pVideoInfo->dwBitRate = 32000000;
			pVideoInfo->AvgTimePerFrame = TIME_PER_FRAME;
			pVideoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pVideoInfo->bmiHeader.biWidth = Width;
			pVideoInfo->bmiHeader.biHeight = Height;
			pVideoInfo->bmiHeader.biCompression = MAKEFOURCC('h','2','6','4');
		}
		break;

	case STREAM_TYPE_H265:
		// H.265
		{
			pMediaType->InitMediaType();
			pMediaType->SetType(&MEDIATYPE_Video);
			pMediaType->SetSubtype(&MEDIASUBTYPE_HEVC);
			pMediaType->SetVariableSize();
			pMediaType->SetTemporalCompression(TRUE);
			pMediaType->SetSampleSize(0);
			pMediaType->SetFormatType(&FORMAT_VideoInfo);
			VIDEOINFOHEADER *pVideoInfo =
				reinterpret_cast<VIDEOINFOHEADER *>(pMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
			if (!pVideoInfo)
				return E_OUTOFMEMORY;
			::ZeroMemory(pVideoInfo, sizeof(VIDEOINFOHEADER));
			pVideoInfo->dwBitRate = 32000000;
			pVideoInfo->AvgTimePerFrame = TIME_PER_FRAME;
			pVideoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pVideoInfo->bmiHeader.biWidth = Width;
			pVideoInfo->bmiHeader.biHeight = Height;
			pVideoInfo->bmiHeader.biCompression = MAKEFOURCC('H','E','V','C');
		}
		break;

	default:
		return E_UNEXPECTED;
	}

	return S_OK;
}


#ifdef LAV_VIDEO_DECODER_WORKAROUND


interface __declspec(uuid("8B81E022-52C7-4B89-9F11-ACFD063AABB4")) IPinSegmentEx : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE EndOfSegment(void) = 0;
};


bool IsLAVVideoDecoderName(const String &Name)
{
	return StringCompareI(Name.c_str(), LIBISDB_STR("LAV Video Decoder")) == 0;
}


void LAVVideoDecoder_NotifyEndOfSegment(COMPointer<IBaseFilter> &Filter, const String &Name)
{
	if (Filter && IsLAVVideoDecoderName(Name)) {
		IPin *pPin = DirectShow::GetFilterPin(Filter.Get(), PINDIR_INPUT);
		if (pPin != nullptr) {
			IPinSegmentEx *pPinSegmentEx;
			if (SUCCEEDED(pPin->QueryInterface(IID_PPV_ARGS(&pPinSegmentEx)))) {
				LIBISDB_TRACE(LIBISDB_STR("Call IPinSegmentEx::EndOfSegment()\n"));
				pPinSegmentEx->EndOfSegment();
				pPinSegmentEx->Release();
			}
		}
	}
}


void LAVVideoDecoder_NotifyNewSegment(COMPointer<IBaseFilter> &Filter, const String &Name)
{
	if (Filter && IsLAVVideoDecoderName(Name)) {
		IPin *pPin = DirectShow::GetFilterPin(Filter.Get(), PINDIR_INPUT);
		if (pPin != nullptr) {
			LIBISDB_TRACE(LIBISDB_STR("Call IPin::NewSegment()\n"));
			pPin->NewSegment(0, 0, 1.0);
			pPin->Release();
		}
	}
}


#endif	// LAV_VIDEO_DECODER_WORKAROUND


}	// namespace




ViewerFilter::ViewerFilter() noexcept
	: m_IsOpen(false)

	, m_pVideoParser(nullptr)

	, m_VideoPID(PID_INVALID)
	, m_AudioPID(PID_INVALID)
	, m_MapAudioPID(PID_INVALID)

	, m_VideoWindowSize()

	, m_VideoRendererType(DirectShow::VideoRenderer::RendererType::Invalid)
	, m_VideoStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_AudioStreamType(STREAM_TYPE_UNINITIALIZED)
	, m_ForcedAspectX(0)
	, m_ForcedAspectY(0)
	, m_ViewStretchMode(ViewStretchMode::KeepAspectRatio)
	, m_NoMaskSideCut(false)
	, m_IgnoreDisplayExtension(false)
	, m_ClipToDevice(true)
	, m_UseAudioRendererClock(true)
	, m_1SegMode(false)
	, m_AdjustAudioStreamTime(false)
	, m_EnablePTSSync(false)
	, m_Adjust1SegVideoSampleTime(true)
	, m_Adjust1SegFrameRate(true)
	, m_BufferSize(0)
	, m_InitialPoolPercentage(0)
	, m_PacketInputWait(0)
	, m_pVideoStreamCallback(nullptr)
	, m_pAudioSampleCallback(nullptr)
{
}


ViewerFilter::~ViewerFilter()
{
	CloseViewer();
}


void ViewerFilter::Finalize()
{
	CloseViewer();
}


void ViewerFilter::Reset()
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::Reset()\n"));

	TryBlockLock Lock(m_FilterLock);
	Lock.TryLock(LOCK_TIMEOUT);

	Flush();

	SetActiveVideoPID(PID_INVALID, false);
	SetActiveAudioPID(PID_INVALID, false);

	//m_VideoInfo.Reset();
}


void ViewerFilter::SetActiveVideoPID(uint16_t PID, bool ServiceChanged)
{
	// 映像出力ピンにPIDをマッピングする

	BlockLock Lock(m_FilterLock);

	if (PID == m_VideoPID)
		return;

	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::SetActiveVideoPID() : %04X <- %04X\n"), PID, m_VideoPID);

	if (m_MPEG2DemuxerVideoMap) {
		// 現在のPIDをアンマップ
		if (m_VideoPID != PID_INVALID) {
			ULONG OldPID = m_VideoPID;
			if (m_MPEG2DemuxerVideoMap->UnmapPID(1, &OldPID) != S_OK)
				return;
		}
	}

	if (!MapVideoPID(PID)) {
		m_VideoPID = PID_INVALID;
		return;
	}

	m_VideoPID = PID;
}


void ViewerFilter::SetActiveAudioPID(uint16_t PID, bool ServiceChanged)
{
	// 音声出力ピンにPIDをマッピングする

	BlockLock Lock(m_FilterLock);

	const bool UseMap = !ServiceChanged;

	if ((PID == m_AudioPID)
			&& (UseMap || PID == m_MapAudioPID))
		return;

	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::SetActiveAudioPID() : %04X <- %04X\n"), PID, m_AudioPID);

	if (UseMap && (PID != PID_INVALID) && (m_MapAudioPID != PID_INVALID)) {
		/*
			UseMap が true の場合、PID を書き換えて音声ストリームを変更する
			IMPEG2PIDMap::MapPID() を呼ぶと再生が一瞬止まるので、それを回避するため
		*/
		if (m_SourceFilter)
			m_SourceFilter->MapAudioPID(PID, m_MapAudioPID);
	} else {
		if (m_MPEG2DemuxerAudioMap) {
			// 現在のPIDをアンマップ
			if (m_MapAudioPID != PID_INVALID) {
				ULONG OldPID = m_MapAudioPID;
				if (m_MPEG2DemuxerAudioMap->UnmapPID(1, &OldPID) != S_OK)
					return;
				m_MapAudioPID = PID_INVALID;
			}
		}

		if (!MapAudioPID(PID)) {
			m_AudioPID = PID_INVALID;
			return;
		}
	}

	m_AudioPID = PID;
}


bool ViewerFilter::ProcessData(DataStream *pData)
{
	if (m_SourceFilter && pData->Is<TSPacket>()) {
		do {
			TSPacket *pPacket = pData->Get<TSPacket>();

			if ((pPacket->GetPID() != PID_NULL)
					&& !pPacket->IsScrambled()) {
				// フィルタグラフに入力
				m_SourceFilter->InputMedia(pPacket);
			}
		} while (pData->Next());
	}

	return true;
}


bool ViewerFilter::OpenViewer(const OpenSettings &Settings)
{
	bool NoVideo = false;

	switch (Settings.VideoStreamType) {
	case STREAM_TYPE_INVALID:
		NoVideo = true;
		break;
	case STREAM_TYPE_MPEG2_VIDEO:
	case STREAM_TYPE_H264:
	case STREAM_TYPE_H265:
		break;
	default:
		SetError(HRESULTErrorCode(E_FAIL), LIBISDB_STR("対応していない映像形式です。"));
		return false;
	}

	TryBlockLock Lock(m_FilterLock);
	if (!Lock.TryLock(LOCK_TIMEOUT)) {
		SetError(HRESULTErrorCode(E_FAIL), LIBISDB_STR("タイムアウトエラーです。"));
		return false;
	}

	if (m_IsOpen) {
		SetError(HRESULTErrorCode(E_UNEXPECTED), LIBISDB_STR("既にフィルタグラフが構築されています。"));
		return false;
	}

	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::OpenViewer() フィルタグラフ作成開始\n"));

	HRESULT hr = S_OK;

	COMPointer<IPin> OutputPin;
	COMPointer<IPin> OutputVideoPin;
	COMPointer<IPin> OutputAudioPin;

	try {
		// フィルタグラフマネージャを作成する
		hr = CreateGraphBuilder();
		if (FAILED(hr)) {
			throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("フィルタグラフマネージャを作成できません。"));
		}

		m_EventListenerList.CallEventListener(&ViewerFilter::EventListener::OnFilterGraphInitialize, this, m_GraphBuilder.Get());

		Log(Logger::LogType::Information, LIBISDB_STR("ソースフィルタの接続中..."));

		// TSSourceFilter
		{
			// インスタンス作成
			m_SourceFilter.Attach(static_cast<DirectShow::TSSourceFilter *>(DirectShow::TSSourceFilter::CreateInstance(nullptr, &hr)));
			if (!m_SourceFilter || FAILED(hr))
				throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("ソースフィルタを作成できません。"));
			m_SourceFilter->SetOutputWhenPaused(Settings.VideoRenderer == DirectShow::VideoRenderer::RendererType::Default);
			// フィルタグラフに追加
			hr = m_GraphBuilder->AddFilter(m_SourceFilter.Get(), L"TSSourceFilter");
			if (FAILED(hr))
				throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("ソースフィルタをフィルタグラフに追加できません。"));
			// 出力ピンを取得
			OutputPin.Attach(DirectShow::GetFilterPin(m_SourceFilter.Get(), PINDIR_OUTPUT));
			if (!OutputPin)
				throw ErrorDescription(HRESULTErrorCode(E_UNEXPECTED), LIBISDB_STR("ソースフィルタの出力ピンを取得できません。"));
			m_SourceFilter->EnableSync(m_EnablePTSSync, m_1SegMode);
			if (m_BufferSize != 0)
				m_SourceFilter->SetBufferSize(m_BufferSize);
			m_SourceFilter->SetInitialPoolPercentage(m_InitialPoolPercentage);
			m_SourceFilter->SetInputWait(m_PacketInputWait);
		}

		Log(Logger::LogType::Information, LIBISDB_STR("MPEG-2 Demultiplexerフィルタの接続中..."));

		// MPEG-2 Demultiplexer
		{
			hr = ::CoCreateInstance(
				CLSID_MPEG2Demultiplexer, nullptr, CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(m_MPEG2DemuxerFilter.GetPP()));
			if (FAILED(hr)) {
				throw ErrorDescription(
					HRESULTErrorCode(hr),
					LIBISDB_STR("MPEG-2 Demultiplexerフィルタを作成できません。"),
					LIBISDB_STR("MPEG-2 Demultiplexerフィルタがインストールされているか確認してください。"));
			}
			hr = DirectShow::AppendFilterAndConnect(m_GraphBuilder.Get(), m_MPEG2DemuxerFilter.Get(), L"MPEG2Demultiplexer", &OutputPin);
			if (FAILED(hr))
				throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("MPEG-2 Demultiplexerをフィルタグラフに追加できません。"));

			// IMpeg2Demultiplexerインタフェースのクエリー
			IMpeg2Demultiplexer *pMpeg2Demuxer;
			hr = m_MPEG2DemuxerFilter.QueryInterface(&pMpeg2Demuxer);
			if (FAILED(hr)) {
				throw ErrorDescription(
					HRESULTErrorCode(hr),
					LIBISDB_STR("MPEG-2 Demultiplexerインターフェースを取得できません。"),
					LIBISDB_STR("互換性のないスプリッタの優先度がMPEG-2 Demultiplexerより高くなっている可能性があります。"));
			}

			if (!NoVideo) {
				CMediaType MediaTypeVideo;

				// 映像メディアフォーマット設定
				hr = SetVideoMediaType(&MediaTypeVideo, Settings.VideoStreamType, 1920, 1080);
				if (FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr));
				// 映像出力ピン作成
				WCHAR szName[] = L"Video";
				hr = pMpeg2Demuxer->CreateOutputPin(&MediaTypeVideo, szName, OutputVideoPin.GetPP());
				if (FAILED(hr)) {
					pMpeg2Demuxer->Release();
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("MPEG-2 Demultiplexerの映像出力ピンを作成できません。"));
				}
			}

			// 音声メディアフォーマット設定
			CMediaType MediaTypeAudio;
			MediaTypeAudio.InitMediaType();
			MediaTypeAudio.SetType(&MEDIATYPE_Audio);
			MediaTypeAudio.SetSubtype(&MEDIASUBTYPE_NULL);
			MediaTypeAudio.SetVariableSize();
			MediaTypeAudio.SetTemporalCompression(TRUE);
			MediaTypeAudio.SetSampleSize(0);
			MediaTypeAudio.SetFormatType(&FORMAT_None);
			// 音声出力ピン作成
			WCHAR szName[] = L"Audio";
			hr = pMpeg2Demuxer->CreateOutputPin(&MediaTypeAudio, szName, OutputAudioPin.GetPP());
			pMpeg2Demuxer->Release();
			if (FAILED(hr))
				throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("MPEG-2 Demultiplexerの音声出力ピンを作成できません。"));
			if (OutputVideoPin) {
				// 映像出力ピンのIMPEG2PIDMapインタフェースのクエリー
				hr = OutputVideoPin.QueryInterface(&m_MPEG2DemuxerVideoMap);
				if (FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("映像出力ピンのIMPEG2PIDMapを取得できません。"));
			}
			// 音声出力ピンのIMPEG2PIDMapインタフェースのクエリ
			hr = OutputAudioPin.QueryInterface(&m_MPEG2DemuxerAudioMap);
			if (FAILED(hr))
				throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("音声出力ピンのIMPEG2PIDMapを取得できません。"));
		}

		// 映像パーサフィルタの接続
		switch (Settings.VideoStreamType) {
		case STREAM_TYPE_MPEG2_VIDEO:
			{
				Log(Logger::LogType::Information, LIBISDB_STR("MPEG-2パーサフィルタの接続中..."));

				// インスタンス作成
				DirectShow::MPEG2ParserFilter *pMPEG2Parser =
					static_cast<DirectShow::MPEG2ParserFilter *>(DirectShow::MPEG2ParserFilter::CreateInstance(nullptr, &hr));
				if (!pMPEG2Parser || FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("MPEG-2パーサフィルタを作成できません。"));
				m_VideoParserFilter.Attach(pMPEG2Parser);
				m_pVideoParser = pMPEG2Parser;
				// フィルタの追加と接続
				hr = DirectShow::AppendFilterAndConnect(
					m_GraphBuilder.Get(), pMPEG2Parser, L"MPEG2ParserFilter", &OutputVideoPin);
				if (FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("MPEG-2パーサフィルタをフィルタグラフに追加できません。"));
			}
			break;

		case STREAM_TYPE_H264:
			{
				Log(Logger::LogType::Information, LIBISDB_STR("H.264パーサフィルタの接続中..."));

				// インスタンス作成
				DirectShow::H264ParserFilter *pH264Parser =
					static_cast<DirectShow::H264ParserFilter *>(DirectShow::H264ParserFilter::CreateInstance(nullptr, &hr));
				if (!pH264Parser || FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("H.264パーサフィルタを作成できません。"));
				m_VideoParserFilter.Attach(pH264Parser);
				m_pVideoParser = pH264Parser;
				// フィルタの追加と接続
				hr = DirectShow::AppendFilterAndConnect(
					m_GraphBuilder.Get(), pH264Parser, L"H264ParserFilter", &OutputVideoPin);
				if (FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("H.264パーサフィルタをフィルタグラフに追加できません。"));
			}
			break;

		case STREAM_TYPE_H265:
			{
				Log(Logger::LogType::Information, LIBISDB_STR("H.265パーサフィルタの接続中..."));

				// インスタンス作成
				DirectShow::H265ParserFilter *pH265Parser =
					static_cast<DirectShow::H265ParserFilter *>(DirectShow::H265ParserFilter::CreateInstance(nullptr, &hr));
				if (!pH265Parser || FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("H.265パーサフィルタを作成できません。"));
				m_VideoParserFilter.Attach(pH265Parser);
				m_pVideoParser = pH265Parser;
				// フィルタの追加と接続
				hr = DirectShow::AppendFilterAndConnect(
					m_GraphBuilder.Get(), pH265Parser, L"H265ParserFilter", &OutputVideoPin);
				if (FAILED(hr))
					throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("H.265パーサフィルタをフィルタグラフに追加できません。"));
			}
			break;
		}

		Log(Logger::LogType::Information, LIBISDB_STR("音声デコーダの接続中..."));

		// AudioDecoderFilter
		{
			// AudioDecoderFilterインスタンス作成
			m_AudioDecoder.Attach(static_cast<DirectShow::AudioDecoderFilter *>(DirectShow::AudioDecoderFilter::CreateInstance(nullptr, &hr)));
			if (!m_AudioDecoder || FAILED(hr))
				throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("音声デコーダフィルタを作成できません。"));
			// フィルタの追加と接続
			hr = DirectShow::AppendFilterAndConnect(
				m_GraphBuilder.Get(), m_AudioDecoder.Get(), L"AudioDecoderFilter", &OutputAudioPin);
			if (FAILED(hr))
				throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("音声デコーダフィルタをフィルタグラフに追加できません。"));

			SetAudioDecoderType(m_AudioStreamType);

			m_AudioDecoder->AddEventListener(this);
			m_AudioDecoder->SetJitterCorrection(m_AdjustAudioStreamTime);
			if (m_pAudioSampleCallback)
				m_AudioDecoder->SetSampleCallback(m_pAudioSampleCallback);
		}

		// 音声フィルタの接続
		if (!Settings.AudioFilterList.empty()) {
			Log(Logger::LogType::Information, LIBISDB_STR("音声フィルタの接続中..."));

			// 検索
			DirectShow::FilterFinder FilterFinder;
			DirectShow::FilterFinder::FilterList FilterList;

			if (FilterFinder.FindFilters(&MEDIATYPE_Audio, &MEDIASUBTYPE_PCM)
					&& FilterFinder.GetFilterList(&FilterList)) {
				for (auto &Name : Settings.AudioFilterList) {
					bool Connected = false;

					for (const auto &Filter : FilterList) {
						if (StringCompareI(Name, Filter.FriendlyName) == 0) {
							COMPointer<IBaseFilter> FilterInterface;

							hr = DirectShow::AppendFilterAndConnect(
								m_GraphBuilder.Get(), Filter.clsid, Filter.FriendlyName.c_str(),
								&FilterInterface, &OutputAudioPin, true);
							if (SUCCEEDED(hr)) {
								LIBISDB_TRACE(LIBISDB_STR("Audio filter connected : %") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("\n"), Filter.FriendlyName.c_str());
								m_AudioFilterList.emplace_back(std::move(FilterInterface));
								Connected = true;
							} else {
								throw ErrorDescription(
									HRESULTErrorCode(hr),
									LIBISDB_STR("音声フィルタをフィルタグラフに追加できません。"),
									LIBISDB_STR("指定された音声フィルタが音声デバイスに対応していない可能性があります。"));
							}
							break;
						}
					}

					if (!Connected) {
						throw ErrorDescription(
							HRESULTErrorCode(E_NOINTERFACE),
							LIBISDB_STR("音声フィルタをフィルタグラフに追加できません。"),
							LIBISDB_STR("指定された音声フィルタが見付かりません。"));
					}
				}
			} else {
				throw ErrorDescription(
					HRESULTErrorCode(E_NOINTERFACE),
					LIBISDB_STR("音声フィルタをフィルタグラフに追加できません。"),
					LIBISDB_STR("利用可能な音声フィルタがありません。"));
			}
		}

		// 映像デコーダの接続
		switch (Settings.VideoStreamType) {
		case STREAM_TYPE_MPEG2_VIDEO:
			ConnectVideoDecoder(
				LIBISDB_STR("MPEG-2"), MEDIASUBTYPE_MPEG2_VIDEO,
				Settings.pszVideoDecoder, &OutputVideoPin);
			break;

		case STREAM_TYPE_H264:
			ConnectVideoDecoder(
				LIBISDB_STR("H.264"), MEDIASUBTYPE_H264,
				Settings.pszVideoDecoder, &OutputVideoPin);
			break;

		case STREAM_TYPE_H265:
			ConnectVideoDecoder(
				LIBISDB_STR("H.265"), MEDIASUBTYPE_HEVC,
				Settings.pszVideoDecoder, &OutputVideoPin);
			break;
		}

		m_VideoStreamType = Settings.VideoStreamType;

		if (m_pVideoParser) {
			m_pVideoParser->AddEventListener(this);
			// madVR は映像サイズの変化時に MediaType を設定しないと新しいサイズが適用されない
			m_pVideoParser->SetAttachMediaType(Settings.VideoRenderer == DirectShow::VideoRenderer::RendererType::madVR);
			if (m_pVideoStreamCallback)
				m_pVideoParser->SetStreamCallback(m_pVideoStreamCallback);
			ApplyAdjustVideoSampleOptions();
		}

		if (!NoVideo) {
			Log(Logger::LogType::Information, LIBISDB_STR("映像レンダラの構築中..."));

			m_VideoRenderer.reset(DirectShow::VideoRenderer::CreateRenderer(Settings.VideoRenderer));
			if (!m_VideoRenderer) {
				throw ErrorDescription(
					HRESULTErrorCode(E_FAIL),
					LIBISDB_STR("映像レンダラを作成できません。"),
					LIBISDB_STR("設定で有効なレンダラが選択されているか確認してください。"));
			}
			m_VideoRenderer->SetClipToDevice(m_ClipToDevice);
			if (!m_VideoRenderer->Initialize(
					m_GraphBuilder.Get(), OutputVideoPin.Get(), Settings.hwndRender, Settings.hwndMessageDrain)) {
				throw ErrorDescription(m_VideoRenderer->GetLastErrorDescription());
			}
			m_VideoRendererType = Settings.VideoRenderer;
		}

		Log(Logger::LogType::Information, LIBISDB_STR("音声レンダラの構築中..."));

		// 音声レンダラ構築
		{
			bool OK = false;

			if (!StringIsEmpty(Settings.pszAudioDevice)) {
				DirectShow::DeviceEnumerator DevEnum;

				if (DevEnum.CreateFilter(CLSID_AudioRendererCategory, Settings.pszAudioDevice, m_AudioRenderer.GetPP())) {
					m_AudioRendererName = Settings.pszAudioDevice;
					OK = true;
				}
			}
			if (!OK) {
				hr = ::CoCreateInstance(
					CLSID_DSoundRender, nullptr, CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(m_AudioRenderer.GetPP()));
				if (SUCCEEDED(hr)) {
					m_AudioRendererName = LIBISDB_STR("DirectSound Renderer");
					OK = true;
				}
			}
			if (OK) {
				hr = DirectShow::AppendFilterAndConnect(
					m_GraphBuilder.Get(), m_AudioRenderer.Get(), L"Audio Renderer", &OutputAudioPin);
				if (SUCCEEDED(hr)) {
#ifdef _DEBUG
					if (!StringIsEmpty(Settings.pszAudioDevice))
						LIBISDB_TRACE(LIBISDB_STR("音声デバイス %") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR(" を接続\n"), Settings.pszAudioDevice);
#endif
					if (m_UseAudioRendererClock) {
						IMediaFilter *pMediaFilter;

						if (SUCCEEDED(m_GraphBuilder.QueryInterface(&pMediaFilter))) {
							IReferenceClock *pReferenceClock;

							if (SUCCEEDED(m_AudioRenderer.QueryInterface(&pReferenceClock))) {
								pMediaFilter->SetSyncSource(pReferenceClock);
								pReferenceClock->Release();
								LIBISDB_TRACE(LIBISDB_STR("グラフのクロックに音声レンダラを選択\n"));
							}
							pMediaFilter->Release();
						}
					}
					OK = true;
				} else {
					OK = false;
				}
				if (!OK) {
					hr = m_GraphBuilder->Render(OutputAudioPin.Get());
					if (FAILED(hr)) {
						throw ErrorDescription(
							HRESULTErrorCode(hr),
							LIBISDB_STR("音声レンダラを接続できません。"),
							LIBISDB_STR("設定で有効な音声デバイスが選択されているか確認してください。"));
					}
				}
			} else {
				// 音声デバイスが無い?
				// Nullレンダラを繋げておく
				hr = ::CoCreateInstance(
					CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER,
					IID_PPV_ARGS(m_AudioRenderer.GetPP()));
				if (SUCCEEDED(hr)) {
					hr = DirectShow::AppendFilterAndConnect(
						m_GraphBuilder.Get(), m_AudioRenderer.Get(), L"Null Audio Renderer", &OutputAudioPin);
					if (FAILED(hr)) {
						throw ErrorDescription(HRESULTErrorCode(hr), LIBISDB_STR("Null音声レンダラを接続できません。"));
					}
					m_AudioRendererName = LIBISDB_STR("Null Renderer");
					LIBISDB_TRACE(LIBISDB_STR("Nullレンダラを接続\n"));
				}
			}
		}

		/*
			デフォルトでMPEG-2 Demultiplexerがグラフのクロックに
			設定されるらしいが、一応設定しておく
		*/
		if (!m_UseAudioRendererClock) {
			IMediaFilter *pMediaFilter;

			if (SUCCEEDED(m_GraphBuilder.QueryInterface(&pMediaFilter))) {
				IReferenceClock *pReferenceClock;

				if (SUCCEEDED(m_MPEG2DemuxerFilter.QueryInterface(&pReferenceClock))) {
					pMediaFilter->SetSyncSource(pReferenceClock);
					pReferenceClock->Release();
					LIBISDB_TRACE(LIBISDB_STR("グラフのクロックにMPEG-2 Demultiplexerを選択\n"));
				}
				pMediaFilter->Release();
			}
		}

		RECT rc;
		::GetClientRect(Settings.hwndRender, &rc);
		m_VideoWindowSize.cx = rc.right;
		m_VideoWindowSize.cy = rc.bottom;

		m_IsOpen = true;

		if (m_MPEG2DemuxerVideoMap && (m_VideoPID != PID_INVALID)) {
			if (!MapVideoPID(m_VideoPID))
				m_VideoPID = PID_INVALID;
		}
		if (m_AudioPID != PID_INVALID) {
			if (!MapAudioPID(m_AudioPID))
				m_AudioPID = PID_INVALID;
		}
	} catch (const ErrorDescription &Error) {
		SetError(Error);

		if (Error.GetErrorCode().value() != 0) {
			TCHAR szText[MAX_ERROR_TEXT_LEN];

			if (::AMGetErrorText(Error.GetErrorCode().value(), szText, (DWORD)std::size(szText)) > 0)
				SetErrorSystemMessage(szText);
		}

		OutputPin.Release();
		OutputAudioPin.Release();
		OutputVideoPin.Release();

		CloseViewer();

		LIBISDB_TRACE(LIBISDB_STR("フィルタグラフ構築失敗 : %") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("\n"), GetLastErrorText());
		return false;
	}

	m_EventListenerList.CallEventListener(&ViewerFilter::EventListener::OnFilterGraphInitialized, this, m_GraphBuilder.Get());

	ResetError();

	LIBISDB_TRACE(LIBISDB_STR("フィルタグラフ構築成功\n"));

	return true;
}


void ViewerFilter::CloseViewer()
{
	TryBlockLock Lock(m_FilterLock);
	Lock.TryLock(LOCK_TIMEOUT);

	if (m_GraphBuilder) {
		Log(Logger::LogType::Information, LIBISDB_STR("フィルタグラフを停止しています..."));
		m_GraphBuilder->Abort();
		Stop();

		m_EventListenerList.CallEventListener(&ViewerFilter::EventListener::OnFilterGraphFinalize, this, m_GraphBuilder.Get());
	}

	Log(Logger::LogType::Information, LIBISDB_STR("COMインスタンスを解放しています..."));

	// COMインスタンスを開放する

	if (m_VideoRenderer)
		m_VideoRenderer->Finalize();

	m_ImageMixer.reset();

	m_SourceFilter.Release();

	m_VideoDecoderFilter.Release();
	m_AudioDecoder.Release();
	m_AudioFilterList.clear();
	m_AudioRenderer.Release();

	m_VideoParserFilter.Release();
	m_pVideoParser = nullptr;

	m_MPEG2DemuxerAudioMap.Release();
	m_MPEG2DemuxerVideoMap.Release();
	m_MPEG2DemuxerFilter.Release();
	m_MapAudioPID = PID_INVALID;

	if (m_GraphBuilder) {
		Log(Logger::LogType::Information, LIBISDB_STR("フィルタグラフを解放しています..."));
		m_EventListenerList.CallEventListener(&ViewerFilter::EventListener::OnFilterGraphFinalized, this, m_GraphBuilder.Get());
		DestroyGraphBuilder();
	}

	m_VideoRenderer.reset();

	m_VideoDecoderName.clear();
	m_AudioRendererName.clear();

	m_VideoStreamType = STREAM_TYPE_UNINITIALIZED;

	m_IsOpen = false;
}


bool ViewerFilter::IsOpen() const
{
	return m_IsOpen;
}


bool ViewerFilter::Play()
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::Play()\n"));

	TryBlockLock Lock(m_FilterLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

#ifdef LAV_VIDEO_DECODER_WORKAROUND
	LAVVideoDecoder_NotifyNewSegment(m_VideoDecoderFilter, m_VideoDecoderName);
#endif

	return FilterGraph::Play();
}


bool ViewerFilter::Stop()
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::Stop()\n"));

	TryBlockLock Lock(m_FilterLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (m_SourceFilter) {
		//m_SourceFilter->Reset();
		m_SourceFilter->Flush();
	}

#ifdef LAV_VIDEO_DECODER_WORKAROUND
	LAVVideoDecoder_NotifyEndOfSegment(m_VideoDecoderFilter, m_VideoDecoderName);
#endif

	return FilterGraph::Stop();
}


bool ViewerFilter::Pause()
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::Pause()\n"));

	TryBlockLock Lock(m_FilterLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (m_SourceFilter) {
		//m_SourceFilter->Reset();
		m_SourceFilter->Flush();
	}

#ifdef LAV_VIDEO_DECODER_WORKAROUND
	LAVVideoDecoder_NotifyEndOfSegment(m_VideoDecoderFilter, m_VideoDecoderName);
#endif

	return FilterGraph::Pause();
}


bool ViewerFilter::Flush()
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::Flush()\n"));

	/*
	TryBlockLock Lock(m_FilterLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;
	*/

	if (!m_SourceFilter)
		return false;

	m_SourceFilter->Flush();

	return true;
}


bool ViewerFilter::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool ViewerFilter::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


bool ViewerFilter::SetVisible(bool Visible)
{
	if (m_VideoRenderer)
		return m_VideoRenderer->SetVisible(Visible);
	return false;
}


void ViewerFilter::HideCursor(bool Hide)
{
	if (m_VideoRenderer)
		m_VideoRenderer->ShowCursor(!Hide);
}


bool ViewerFilter::RepaintVideo(HWND hwnd, HDC hdc)
{
	if (m_VideoRenderer)
		return m_VideoRenderer->RepaintVideo(hwnd,hdc);
	return false;
}


bool ViewerFilter::DisplayModeChanged()
{
	if (m_VideoRenderer)
		return m_VideoRenderer->DisplayModeChanged();
	return false;
}


void ViewerFilter::Set1SegMode(bool OneSeg)
{
	if (m_1SegMode != OneSeg) {
		LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::Set1SegMode(%d)\n"), OneSeg);

		m_1SegMode = OneSeg;

		if (m_SourceFilter)
			m_SourceFilter->EnableSync(m_EnablePTSSync, m_1SegMode);
		ApplyAdjustVideoSampleOptions();
	}
}


bool ViewerFilter::Get1SegMode() const
{
	return m_1SegMode;
}


uint16_t ViewerFilter::GetVideoPID() const
{
	return m_VideoPID;
}


uint16_t ViewerFilter::GetAudioPID() const
{
	return m_AudioPID;
}


bool ViewerFilter::AdjustVideoPosition()
{
	// 映像の位置を調整する
	if (m_VideoRenderer
			&& (m_VideoWindowSize.cx > 0) && (m_VideoWindowSize.cy > 0)
			&& (m_VideoInfo.OriginalWidth > 0) && (m_VideoInfo.OriginalHeight > 0)) {
		const long WindowWidth = m_VideoWindowSize.cx;
		const long WindowHeight = m_VideoWindowSize.cy;
		long DestWidth, DestHeight;

		if (m_ViewStretchMode == ViewStretchMode::Fit) {
			// ウィンドウサイズに合わせる
			DestWidth = WindowWidth;
			DestHeight = WindowHeight;
		} else {
			int AspectX, AspectY;

			if ((m_ForcedAspectX > 0) && (m_ForcedAspectY > 0)) {
				// アスペクト比が指定されている
				AspectX = m_ForcedAspectX;
				AspectY = m_ForcedAspectY;
			} else if ((m_VideoInfo.AspectRatioX > 0) && (m_VideoInfo.AspectRatioY > 0)) {
				// 映像のアスペクト比を使用する
				AspectX = m_VideoInfo.AspectRatioX;
				AspectY = m_VideoInfo.AspectRatioY;
				if (m_IgnoreDisplayExtension
						&& (m_VideoInfo.DisplayWidth > 0)
						&& (m_VideoInfo.DisplayHeight > 0)) {
					AspectX = AspectX * 3 * m_VideoInfo.OriginalWidth / m_VideoInfo.DisplayWidth;
					AspectY = AspectY * 3 * m_VideoInfo.OriginalHeight / m_VideoInfo.DisplayHeight;
				}
			} else {
				// アスペクト比不明
				if (m_VideoInfo.DisplayHeight == 1080) {
					AspectX = 16;
					AspectY = 9;
				} else if ((m_VideoInfo.DisplayWidth > 0) && (m_VideoInfo.DisplayHeight > 0)) {
					AspectX = m_VideoInfo.DisplayWidth;
					AspectY = m_VideoInfo.DisplayHeight;
				} else {
					AspectX = WindowWidth;
					AspectY = WindowHeight;
				}
			}

			const double WindowRatio = static_cast<double>(WindowWidth) / static_cast<double>(WindowHeight);
			const double AspectRatio = static_cast<double>(AspectX) / static_cast<double>(AspectY);
			if (((m_ViewStretchMode == ViewStretchMode::KeepAspectRatio) && (AspectRatio > WindowRatio))
					|| ((m_ViewStretchMode == ViewStretchMode::Crop) && (AspectRatio < WindowRatio))) {
				DestWidth = WindowWidth;
				DestHeight = ::MulDiv(DestWidth, AspectY, AspectX);
			} else {
				DestHeight = WindowHeight;
				DestWidth = ::MulDiv(DestHeight, AspectX, AspectY);
			}
		}

		RECT rcSrc,rcDst,rcWindow;
		CalcSourceRect(&rcSrc);
#if 0
		// 座標値がマイナスになるとマルチディスプレイでおかしくなる?
		rcDst.left = (WindowWidth - DestWidth) / 2;
		rcDst.top = (WindowHeight - DestHeight) / 2,
		rcDst.right = rcDst.left + DestWidth;
		rcDst.bottom = rcDst.top + DestHeight;
#else
		if (WindowWidth < DestWidth) {
			rcDst.left = 0;
			rcDst.right = WindowWidth;
			rcSrc.left += ::MulDiv(DestWidth - WindowWidth, rcSrc.right - rcSrc.left, DestWidth) / 2;
			rcSrc.right = m_VideoInfo.OriginalWidth - rcSrc.left;
		} else {
			if (m_NoMaskSideCut
					&& (WindowWidth > DestWidth)
					&& (rcSrc.right - rcSrc.left < m_VideoInfo.OriginalWidth)) {
				int NewDestWidth = ::MulDiv(m_VideoInfo.OriginalWidth, DestWidth, rcSrc.right - rcSrc.left);
				if (NewDestWidth > WindowWidth)
					NewDestWidth = WindowWidth;
				int NewSrcWidth = ::MulDiv(rcSrc.right - rcSrc.left, NewDestWidth, DestWidth);
				rcSrc.left = (m_VideoInfo.OriginalWidth - NewSrcWidth) / 2;
				rcSrc.right = rcSrc.left + NewSrcWidth;
				LIBISDB_TRACE(
					LIBISDB_STR("Adjust %d x %d -> %d x %d [%d - %d (%d)]\n"),
					DestWidth, DestHeight, NewDestWidth, DestHeight,
					rcSrc.left, rcSrc.right, NewSrcWidth);
				DestWidth = NewDestWidth;
			}
			rcDst.left = (WindowWidth - DestWidth) / 2;
			rcDst.right = rcDst.left + DestWidth;
		}
		if (WindowHeight < DestHeight) {
			rcDst.top = 0;
			rcDst.bottom = WindowHeight;
			rcSrc.top += ::MulDiv(DestHeight - WindowHeight, rcSrc.bottom - rcSrc.top, DestHeight) / 2;
			rcSrc.bottom = m_VideoInfo.OriginalHeight - rcSrc.top;
		} else {
			rcDst.top = (WindowHeight - DestHeight) / 2,
			rcDst.bottom = rcDst.top + DestHeight;
		}
#endif

		rcWindow.left = 0;
		rcWindow.top = 0;
		rcWindow.right = WindowWidth;
		rcWindow.bottom = WindowHeight;

#if 0
		LIBISDB_TRACE(
			LIBISDB_STR("SetVideoPosition %d,%d,%d,%d -> %d,%d,%d,%d [%d,%d,%d,%d]\n"),
			rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom,
			rcDst.left, rcDst.top, rcDst.right, rcDst.bottom,
			rcWindow.left, rcWindow.top, rcWindow.right, rcWindow.bottom);
#endif

		return m_VideoRenderer->SetVideoPosition(
			m_VideoInfo.OriginalWidth, m_VideoInfo.OriginalHeight, rcSrc, rcDst, rcWindow);
	}

	return false;
}


// 映像ウィンドウのサイズを設定する
bool ViewerFilter::SetViewSize(int Width, int Height)
{
	BlockLock Lock(m_ResizeLock);

	if ((Width > 0) && (Height > 0)) {
		m_VideoWindowSize.cx = Width;
		m_VideoWindowSize.cy = Height;
		return AdjustVideoPosition();
	}

	return false;
}


// 映像のサイズを取得する
bool ViewerFilter::GetVideoSize(ReturnArg<int> Width, ReturnArg<int> Height) const
{
	if (m_IgnoreDisplayExtension)
		return GetOriginalVideoSize(Width, Height);

	BlockLock Lock(m_ResizeLock);

	if ((m_VideoInfo.DisplayWidth > 0) && (m_VideoInfo.DisplayHeight > 0)) {
		Width = m_VideoInfo.DisplayWidth;
		Height = m_VideoInfo.DisplayHeight;
		return true;
	}

	Width = 0;
	Height = 0;

	return false;
}


bool ViewerFilter::GetOriginalVideoSize(ReturnArg<int> Width, ReturnArg<int> Height) const
{
	BlockLock Lock(m_ResizeLock);

	if ((m_VideoInfo.OriginalWidth > 0) && (m_VideoInfo.OriginalHeight > 0)) {
		Width = m_VideoInfo.OriginalWidth;
		Height = m_VideoInfo.OriginalHeight;
		return true;
	}

	Width = 0;
	Height = 0;

	return false;
}


bool ViewerFilter::GetCroppedVideoSize(ReturnArg<int> Width, ReturnArg<int> Height) const
{
	RECT rc;

	if (!GetSourceRect(&rc)) {
		Width = 0;
		Height = 0;
		return false;
	}

	Width = rc.right - rc.left;
	Height = rc.bottom - rc.top;

	return true;
}


bool ViewerFilter::GetSourceRect(ReturnArg<RECT> Rect) const
{
	if (!Rect)
		return false;

	BlockLock Lock(m_ResizeLock);

	return CalcSourceRect(Rect);
}


bool ViewerFilter::CalcSourceRect(ReturnArg<RECT> Rect) const
{
	if ((m_VideoInfo.OriginalWidth == 0) || (m_VideoInfo.OriginalHeight == 0))
		return false;

	long SrcX, SrcY, SrcWidth, SrcHeight;

	if (m_Clipping.HorzFactor != 0) {
		long ClipLeft, ClipRight;
		ClipLeft = ::MulDiv(m_VideoInfo.OriginalWidth, m_Clipping.Left, m_Clipping.HorzFactor);
		ClipRight = ::MulDiv(m_VideoInfo.OriginalWidth, m_Clipping.Right, m_Clipping.HorzFactor);
		SrcWidth = m_VideoInfo.OriginalWidth - (ClipLeft + ClipRight);
		SrcX = ClipLeft;
	} else if (m_IgnoreDisplayExtension) {
		SrcWidth = m_VideoInfo.OriginalWidth;
		SrcX = 0;
	} else {
		SrcWidth = m_VideoInfo.DisplayWidth;
		SrcX = m_VideoInfo.DisplayPosX;
	}

	if (m_Clipping.VertFactor != 0) {
		long ClipTop, ClipBottom;
		ClipTop = ::MulDiv(m_VideoInfo.OriginalHeight, m_Clipping.Top, m_Clipping.VertFactor);
		ClipBottom = ::MulDiv(m_VideoInfo.OriginalHeight, m_Clipping.Bottom, m_Clipping.VertFactor);
		SrcHeight = m_VideoInfo.OriginalHeight - (ClipTop + ClipBottom);
		SrcY = ClipTop;
	} else if (m_IgnoreDisplayExtension) {
		SrcHeight = m_VideoInfo.OriginalHeight;
		SrcY = 0;
	} else {
		SrcHeight = m_VideoInfo.DisplayHeight;
		SrcY = m_VideoInfo.DisplayPosY;
	}

	Rect->left = SrcX;
	Rect->top = SrcY;
	Rect->right = SrcX + SrcWidth;
	Rect->bottom = SrcY + SrcHeight;

	return true;
}


bool ViewerFilter::GetDestRect(ReturnArg<RECT> Rect) const
{
	if (m_VideoRenderer && Rect) {
		if (m_VideoRenderer->GetDestPosition(Rect))
			return true;
	}

	return false;
}


bool ViewerFilter::GetDestSize(ReturnArg<int> Width, ReturnArg<int> Height) const
{
	RECT rc;

	if (!GetDestRect(&rc)) {
		Width = 0;
		Height = 0;
		return false;
	}

	Width = rc.right-rc.left;
	Height = rc.bottom-rc.top;

	return true;
}


// 映像のアスペクト比を取得する
bool ViewerFilter::GetVideoAspectRatio(ReturnArg<int> AspectRatioX, ReturnArg<int> AspectRatioY) const
{
	BlockLock Lock(m_ResizeLock);

	if ((m_VideoInfo.AspectRatioX > 0) && (m_VideoInfo.AspectRatioY > 0)) {
		AspectRatioX = m_VideoInfo.AspectRatioX;
		AspectRatioY = m_VideoInfo.AspectRatioY;
		return true;
	}

	return false;
}


// 映像のアスペクト比を設定する
bool ViewerFilter::SetForcedAspectRatio(int AspectX, int AspectY)
{
	m_ForcedAspectX = AspectX;
	m_ForcedAspectY = AspectY;
	return true;
}


// 設定されたアスペクト比を取得する
bool ViewerFilter::GetForcedAspectRatio(ReturnArg<int> AspectX, ReturnArg<int> AspectY) const
{
	AspectX = m_ForcedAspectX;
	AspectY = m_ForcedAspectY;
	return true;
}


// 有効なアスペクト比を取得する
bool ViewerFilter::GetEffectiveAspectRatio(ReturnArg<int> AspectX, ReturnArg<int> AspectY) const
{
	if ((m_ForcedAspectX > 0) && (m_ForcedAspectY > 0)) {
		AspectX = m_ForcedAspectX;
		AspectY = m_ForcedAspectY;
		return true;
	}

	int X, Y;
	if (!GetVideoAspectRatio(&X, &Y))
		return false;

	if (m_IgnoreDisplayExtension
			&& ((m_VideoInfo.DisplayWidth != m_VideoInfo.OriginalWidth)
				|| (m_VideoInfo.DisplayHeight != m_VideoInfo.OriginalHeight))) {
		if ((m_VideoInfo.DisplayWidth == 0) || (m_VideoInfo.DisplayHeight == 0))
			return false;
		X = X * 3 * m_VideoInfo.OriginalWidth / m_VideoInfo.DisplayWidth;
		Y = Y * 3 * m_VideoInfo.OriginalHeight / m_VideoInfo.DisplayHeight;
		int d = std::gcd(X, Y);
		if (d != 0) {
			X /= d;
			Y /= d;
		}
	}

	AspectX = X;
	AspectY = Y;

	return true;
}


bool ViewerFilter::SetPanAndScan(int AspectX, int AspectY, const ClippingInfo *pClipping)
{
	if ((m_ForcedAspectX != AspectX) || (m_ForcedAspectY != AspectY) || (pClipping != nullptr)) {
		BlockLock Lock(m_ResizeLock);

		m_ForcedAspectX = AspectX;
		m_ForcedAspectY = AspectY;
		if (pClipping != nullptr)
			m_Clipping = *pClipping;
		else
			m_Clipping.Reset();

		AdjustVideoPosition();
	}

	return true;
}


bool ViewerFilter::GetClippingInfo(ReturnArg<ClippingInfo> Clipping) const
{
	if (!Clipping)
		return false;

	*Clipping = m_Clipping;

	return true;
}


bool ViewerFilter::SetViewStretchMode(ViewStretchMode Mode)
{
	if (m_ViewStretchMode != Mode) {
		BlockLock Lock(m_ResizeLock);

		m_ViewStretchMode = Mode;

		return AdjustVideoPosition();
	}

	return true;
}


bool ViewerFilter::SetNoMaskSideCut(bool NoMask, bool Adjust)
{
	if (m_NoMaskSideCut != NoMask) {
		BlockLock Lock(m_ResizeLock);

		m_NoMaskSideCut = NoMask;

		if (Adjust)
			AdjustVideoPosition();
	}

	return true;
}


bool ViewerFilter::SetIgnoreDisplayExtension(bool Ignore)
{
	if (Ignore != m_IgnoreDisplayExtension) {
		BlockLock Lock(m_ResizeLock);

		m_IgnoreDisplayExtension = Ignore;

		if ((m_VideoInfo.DisplayWidth != m_VideoInfo.OriginalWidth)
				|| (m_VideoInfo.DisplayHeight != m_VideoInfo.OriginalHeight))
			AdjustVideoPosition();
	}

	return true;
}


bool ViewerFilter::SetClipToDevice(bool Clip)
{
	if (Clip != m_ClipToDevice) {
		m_ClipToDevice = Clip;

		if (m_VideoRenderer)
			m_VideoRenderer->SetClipToDevice(m_ClipToDevice);
	}

	return true;
}


COMPointer<IBaseFilter> ViewerFilter::GetVideoDecoderFilter() const
{
	return m_VideoDecoderFilter;
}


void ViewerFilter::SetVideoDecoderSettings(const DirectShow::KnownDecoderManager::VideoDecoderSettings &Settings)
{
	m_KnownDecoderManager.SetVideoDecoderSettings(Settings);
}


bool ViewerFilter::GetVideoDecoderSettings(DirectShow::KnownDecoderManager::VideoDecoderSettings *pSettings) const
{
	return m_KnownDecoderManager.GetVideoDecoderSettings(pSettings);
}


void ViewerFilter::SaveVideoDecoderSettings()
{
	if (m_VideoDecoderFilter)
		m_KnownDecoderManager.SaveVideoDecoderSettings(m_VideoDecoderFilter.Get());
}


uint8_t ViewerFilter::GetAudioChannelCount() const
{
	if (m_AudioDecoder)
		return m_AudioDecoder->GetCurrentChannelCount();
	return AudioChannelCount_Invalid;
}


bool ViewerFilter::SetDualMonoMode(DirectShow::AudioDecoderFilter::DualMonoMode Mode)
{
	if (m_AudioDecoder)
		return m_AudioDecoder->SetDualMonoMode(Mode);
	return false;
}


DirectShow::AudioDecoderFilter::DualMonoMode ViewerFilter::GetDualMonoMode() const
{
	if (m_AudioDecoder)
		return m_AudioDecoder->GetDualMonoMode();
	return DirectShow::AudioDecoderFilter::DualMonoMode::Invalid;
}


bool ViewerFilter::SetStereoMode(DirectShow::AudioDecoderFilter::StereoMode Mode)
{
	// ステレオ出力チャンネルの設定
	if (m_AudioDecoder)
		return m_AudioDecoder->SetStereoMode(Mode);
	return false;
}


DirectShow::AudioDecoderFilter::StereoMode ViewerFilter::GetStereoMode() const
{
	if (m_AudioDecoder)
		return m_AudioDecoder->GetStereoMode();
	return DirectShow::AudioDecoderFilter::StereoMode::Stereo;
}


bool ViewerFilter::SetSPDIFOptions(const DirectShow::AudioDecoderFilter::SPDIFOptions &Options)
{
	if (m_AudioDecoder)
		return m_AudioDecoder->SetSPDIFOptions(Options);
	return false;
}


bool ViewerFilter::GetSPDIFOptions(DirectShow::AudioDecoderFilter::SPDIFOptions *pOptions) const
{
	if (m_AudioDecoder)
		return m_AudioDecoder->GetSPDIFOptions(pOptions);
	return false;
}


bool ViewerFilter::IsSPDIFPassthrough() const
{
	if (m_AudioDecoder)
		return m_AudioDecoder->IsSPDIFPassthrough();
	return false;
}


bool ViewerFilter::SetDownMixSurround(bool DownMix)
{
	if (m_AudioDecoder)
		return m_AudioDecoder->SetDownMixSurround(DownMix);
	return false;
}


bool ViewerFilter::GetDownMixSurround() const
{
	if (m_AudioDecoder)
		return m_AudioDecoder->GetDownMixSurround();
	return false;
}


bool ViewerFilter::SetAudioGainControl(bool EnableGainControl, float Gain, float SurroundGain)
{
	if (m_AudioDecoder)
		return m_AudioDecoder->SetGainControl(EnableGainControl, Gain, SurroundGain);
	return false;
}


bool ViewerFilter::SetAudioDelay(LONGLONG Delay)
{
	if (m_AudioDecoder)
		return m_AudioDecoder->SetDelay(Delay);
	return false;
}


LONGLONG ViewerFilter::GetAudioDelay() const
{
	if (m_AudioDecoder)
		return m_AudioDecoder->GetDelay();
	return 0;
}


COMPointer<DirectShow::AudioDecoderFilter> ViewerFilter::GetAudioDecoderFilter() const
{
	return m_AudioDecoder;
}


bool ViewerFilter::SetAudioStreamType(uint8_t StreamType)
{
	m_AudioStreamType = StreamType;

	if (m_AudioDecoder) {
		SetAudioDecoderType(m_AudioStreamType);
	}

	return true;
}


bool ViewerFilter::GetVideoDecoderName(String *pName) const
{
	if (pName == nullptr)
		return false;

	*pName = m_VideoDecoderName;

	return !pName->empty();
}


bool ViewerFilter::GetVideoRendererName(String *pName) const
{
	if (pName == nullptr)
		return false;

	LPCTSTR pszRenderer = DirectShow::VideoRenderer::EnumRendererName(m_VideoRendererType);
	if (pszRenderer == nullptr) {
		pName->clear();
		return false;
	}

	*pName = pszRenderer;

	return true;
}


bool ViewerFilter::GetAudioRendererName(String *pName) const
{
	if (pName == nullptr)
		return false;

	*pName = m_AudioRendererName;

	return !pName->empty();
}


DirectShow::VideoRenderer::RendererType ViewerFilter::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


BYTE ViewerFilter::GetVideoStreamType() const
{
	return m_VideoStreamType;
}


bool ViewerFilter::DisplayFilterProperty(HWND hwndOwner, PropertyFilterType Filter, int Index)
{
	switch (Filter) {
	case PropertyFilterType::VideoDecoder:
		if (m_VideoDecoderFilter)
			return DirectShow::ShowPropertyPage(m_VideoDecoderFilter.Get(), hwndOwner);
		break;

	case PropertyFilterType::VideoRenderer:
		if (m_VideoRenderer)
			return m_VideoRenderer->ShowProperty(hwndOwner);
		break;

	case PropertyFilterType::MPEG2Demultiplexer:
		if (m_MPEG2DemuxerFilter)
			return DirectShow::ShowPropertyPage(m_MPEG2DemuxerFilter.Get(), hwndOwner);
		break;

	case PropertyFilterType::AudioFilter:
		if (static_cast<size_t>(Index) < m_AudioFilterList.size())
			return DirectShow::ShowPropertyPage(m_AudioFilterList[Index].Get(), hwndOwner);
		break;

	case PropertyFilterType::AudioRenderer:
		if (m_AudioRenderer)
			return DirectShow::ShowPropertyPage(m_AudioRenderer.Get(), hwndOwner);
		break;
	}

	return false;
}

bool ViewerFilter::FilterHasProperty(PropertyFilterType Filter, int Index) const
{
	switch (Filter) {
	case PropertyFilterType::VideoDecoder:
		if (m_VideoDecoderFilter)
			return DirectShow::HasPropertyPage(m_VideoDecoderFilter.Get());
		break;

	case PropertyFilterType::VideoRenderer:
		if (m_VideoRenderer)
			return m_VideoRenderer->HasProperty();
		break;

	case PropertyFilterType::MPEG2Demultiplexer:
		if (m_MPEG2DemuxerFilter)
			return DirectShow::HasPropertyPage(m_MPEG2DemuxerFilter.Get());
		break;

	case PropertyFilterType::AudioFilter:
		if (static_cast<size_t>(Index) < m_AudioFilterList.size())
			return DirectShow::HasPropertyPage(m_AudioFilterList[Index].Get());
		break;

	case PropertyFilterType::AudioRenderer:
		if (m_AudioRenderer)
			return DirectShow::HasPropertyPage(m_AudioRenderer.Get());
		break;
	}

	return false;
}


bool ViewerFilter::SetUseAudioRendererClock(bool Use)
{
	m_UseAudioRendererClock = Use;
	return true;
}


bool ViewerFilter::SetAdjustAudioStreamTime(bool Adjust)
{
	m_AdjustAudioStreamTime = Adjust;
	if (m_AudioDecoder)
		return m_AudioDecoder->SetJitterCorrection(Adjust);
	return true;
}


bool ViewerFilter::SetAudioSampleCallback(DirectShow::AudioDecoderFilter::SampleCallback *pCallback)
{
	m_pAudioSampleCallback = pCallback;
	if (m_AudioDecoder)
		return m_AudioDecoder->SetSampleCallback(pCallback);
	return true;
}


void ViewerFilter::SetVideoStreamCallback(DirectShow::VideoParser::StreamCallback *pCallback)
{
	m_pVideoStreamCallback = pCallback;
	if (m_pVideoParser)
		m_pVideoParser->SetStreamCallback(pCallback);
}


COMMemoryPointer<> ViewerFilter::GetCurrentImage()
{
	if (m_VideoRenderer)
		return m_VideoRenderer->GetCurrentImage();

	return COMMemoryPointer<>();
}


bool ViewerFilter::DrawText(LPCTSTR pszText, int x, int y, HFONT hfont, COLORREF Color, int Opacity)
{
	if (!m_VideoRenderer || !IsDrawTextSupported())
		return false;

	IBaseFilter *pRenderer;
	int Width, Height;

	pRenderer = m_VideoRenderer->GetRendererFilter();
	if (pRenderer == nullptr)
		return false;
	if (!m_ImageMixer) {
		m_ImageMixer.reset(DirectShow::ImageMixer::CreateImageMixer(m_VideoRendererType, pRenderer));
		if (!m_ImageMixer)
			return false;
	}
	if (!m_ImageMixer->GetMapSize(&Width, &Height))
		return false;
	m_ResizeLock.Lock();
	if ((m_VideoInfo.OriginalWidth == 0) || (m_VideoInfo.OriginalHeight == 0))
		return false;
	x = x * Width / m_VideoInfo.OriginalWidth;
	y = y * Height / m_VideoInfo.OriginalHeight;
	m_ResizeLock.Unlock();
	return m_ImageMixer->SetText(pszText, x, y, hfont, Color, Opacity);
}


bool ViewerFilter::IsDrawTextSupported() const
{
	return DirectShow::ImageMixer::IsSupported(m_VideoRendererType);
}


bool ViewerFilter::ClearOSD()
{
	if (!m_VideoRenderer)
		return false;

	if (m_ImageMixer)
		m_ImageMixer->Clear();

	return true;
}


bool ViewerFilter::EnablePTSSync(bool Enable)
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::EnablePTSSync(%") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR(")\n"), Enable ? LIBISDB_STR("true") : LIBISDB_STR("false"));
	if (m_SourceFilter) {
		if (!m_SourceFilter->EnableSync(Enable, m_1SegMode))
			return false;
	}
	m_EnablePTSSync = Enable;
	return true;
}


bool ViewerFilter::IsPTSSyncEnabled() const
{
	/*
	if (m_SourceFilter)
		return m_SourceFilter->IsSyncEnabled();
	*/
	return m_EnablePTSSync;
}


bool ViewerFilter::SetAdjust1SegVideoSample(bool AdjustTime, bool AdjustFrameRate)
{
	LIBISDB_TRACE(
		LIBISDB_STR("ViewerFilter::SetAdjust1SegVideoSample() : Adjust time %d / Adjust frame rate %d\n"),
		AdjustTime, AdjustFrameRate);

	m_Adjust1SegVideoSampleTime = AdjustTime;
	m_Adjust1SegFrameRate = AdjustFrameRate;
	ApplyAdjustVideoSampleOptions();

	return true;
}


void ViewerFilter::ResetBuffer()
{
	if (m_SourceFilter)
		m_SourceFilter->Reset();
}


bool ViewerFilter::SetBufferSize(size_t Size)
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::SetBufferSize(%zu)\n"), Size);

	if (m_SourceFilter) {
		if (!m_SourceFilter->SetBufferSize(Size))
			return false;
	}

	m_BufferSize = Size;

	return true;
}


bool ViewerFilter::SetInitialPoolPercentage(int Percentage)
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::SetInitialPoolPercentage(%d)\n"), Percentage);

	if (m_SourceFilter) {
		if (!m_SourceFilter->SetInitialPoolPercentage(Percentage))
			return false;
	}

	m_InitialPoolPercentage = Percentage;

	return true;
}


int ViewerFilter::GetBufferFillPercentage() const
{
	if (m_SourceFilter)
		return m_SourceFilter->GetBufferFillPercentage();
	return 0;
}


bool ViewerFilter::SetPacketInputWait(DWORD Wait)
{
	LIBISDB_TRACE(LIBISDB_STR("ViewerFilter::SetPacketInputWait(%u)\n"), Wait);

	if (m_SourceFilter) {
		if (!m_SourceFilter->SetInputWait(Wait))
			return false;
	}

	m_PacketInputWait = Wait;

	return true;
}


void ViewerFilter::ConnectVideoDecoder(
	LPCTSTR pszCodecName, const GUID &MediaSubType, LPCTSTR pszDecoderName, COMPointer<IPin> *pOutputPin)
{
	Log(Logger::LogType::Information, LIBISDB_STR("%") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("デコーダの接続中..."), pszCodecName);

	const bool Default = StringIsEmpty(pszDecoderName);
	bool ConnectSuccess = false;
	HRESULT hr;
	String FilterName;

	if (m_KnownDecoderManager.IsMediaSupported(MediaSubType)
			&& ((Default && m_KnownDecoderManager.IsDecoderAvailable(MediaSubType))
			|| (!Default && ::lstrcmpi(m_KnownDecoderManager.GetDecoderName(MediaSubType), pszDecoderName) == 0))) {
		IBaseFilter *pFilter;

		hr = m_KnownDecoderManager.CreateInstance(MediaSubType, &pFilter);
		if (SUCCEEDED(hr)) {
			FilterName = m_KnownDecoderManager.GetDecoderName(MediaSubType);
			hr = DirectShow::AppendFilterAndConnect(
				m_GraphBuilder.Get(), pFilter, FilterName.c_str(), pOutputPin, true);
			if (SUCCEEDED(hr)) {
				m_VideoDecoderFilter.Attach(pFilter);
				ConnectSuccess = true;
			} else {
				pFilter->Release();
			}
		}
	}

	TCHAR szText1[128], szText2[128];

	if (!ConnectSuccess) {
		DirectShow::FilterFinder FilterFinder;

		// 検索
		if (!FilterFinder.FindFilters(&MEDIATYPE_Video, &MediaSubType)) {
			StringPrintf(
				szText1,
				LIBISDB_STR("%") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("デコーダが見付かりません。"),
				pszCodecName);
			StringPrintf(
				szText2,
				LIBISDB_STR("%") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("デコーダがインストールされているか確認してください。"),
				pszCodecName);
			throw ErrorDescription(HRESULTErrorCode(E_FAIL), szText1, szText2);
		}

		if (Default) {
			CLSID id = m_KnownDecoderManager.GetDecoderCLSID(MediaSubType);

			if (id != GUID_NULL)
				FilterFinder.SetPreferredFilter(id);
		}

		for (int i = 0; i < FilterFinder.GetFilterCount(); i++) {
			CLSID clsidFilter;

			if (FilterFinder.GetFilterInfo(i, &clsidFilter, &FilterName)) {
				if (!Default && ::lstrcmpi(FilterName.c_str(), pszDecoderName) != 0)
					continue;
				hr = DirectShow::AppendFilterAndConnect(
					m_GraphBuilder.Get(),
					clsidFilter, FilterName.c_str(), &m_VideoDecoderFilter,
					pOutputPin, true);
				if (SUCCEEDED(hr)) {
					ConnectSuccess = true;
					break;
				}
			}
		}
	}

	// どれかのフィルタで接続できたか
	if (ConnectSuccess) {
		m_VideoDecoderName = FilterName;
	} else {
		StringPrintf(
			szText1,
			LIBISDB_STR("%") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("デコーダフィルタをフィルタグラフに追加できません。"),
			pszCodecName);
		throw ErrorDescription(
			HRESULTErrorCode(hr), szText1,
			LIBISDB_STR("設定で有効なデコーダが選択されているか確認してください。\nまた、レンダラを変えてみてください。"));
	}
}


bool ViewerFilter::MapVideoPID(uint16_t PID)
{
	if (m_MPEG2DemuxerVideoMap) {
		// 新規にPIDをマップ
		if (PID != PID_INVALID) {
			ULONG TempPID = PID;
			if (m_MPEG2DemuxerVideoMap->MapPID(1, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
		}
	}

	if (m_SourceFilter)
		m_SourceFilter->SetVideoPID(PID);

	return true;
}


bool ViewerFilter::MapAudioPID(uint16_t PID)
{
	if (m_MPEG2DemuxerAudioMap) {
		// 新規にPIDをマップ
		if (PID != PID_INVALID) {
			ULONG TempPID = PID;
			if (m_MPEG2DemuxerAudioMap->MapPID(1, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
			m_MapAudioPID = PID;
		}
	}

	if (m_SourceFilter)
		m_SourceFilter->SetAudioPID(PID);

	return true;
}


void ViewerFilter::ApplyAdjustVideoSampleOptions()
{
	if (m_pVideoParser != nullptr) {
		DirectShow::VideoParser::AdjustSampleFlag Flags = DirectShow::VideoParser::AdjustSampleFlag::None;

		if (m_1SegMode) {
			Flags = DirectShow::VideoParser::AdjustSampleFlag::OneSeg;
			// Microsoft DTV-DVD Video Decoder では何故か映像が出なくなってしまうため無効とする
			if (StringCompareI(m_VideoDecoderName.c_str(), LIBISDB_STR("Microsoft DTV-DVD Video Decoder")) != 0) {
				if (m_Adjust1SegVideoSampleTime)
					Flags |= DirectShow::VideoParser::AdjustSampleFlag::Time;
				if (m_Adjust1SegFrameRate)
					Flags |= DirectShow::VideoParser::AdjustSampleFlag::FrameRate;
			}
		}

		m_pVideoParser->SetAdjustSampleOptions(Flags);
	}
}


void ViewerFilter::SetAudioDecoderType(BYTE StreamType)
{
	if (m_AudioDecoder) {
		m_AudioDecoder->SetDecoderType(
			StreamType == STREAM_TYPE_AAC ?
				DirectShow::AudioDecoderFilter::DecoderType::AAC :
			StreamType == STREAM_TYPE_MPEG1_AUDIO ||
			StreamType == STREAM_TYPE_MPEG2_AUDIO ?
				DirectShow::AudioDecoderFilter::DecoderType::MPEGAudio :
			StreamType == STREAM_TYPE_AC3 ?
				DirectShow::AudioDecoderFilter::DecoderType::AC3 :
				DirectShow::AudioDecoderFilter::DecoderType::Invalid);
	}
}


void ViewerFilter::OnSPDIFPassthroughError(HRESULT hr)
{
	m_EventListenerList.CallEventListener(&ViewerFilter::EventListener::OnSPDIFPassthroughError, this, hr);
}


void ViewerFilter::OnVideoInfoUpdated(const DirectShow::VideoParser::VideoInfo &VideoInfo)
{
	/*if (m_VideoInfo != VideoInfo)*/ {
		// ビデオ情報の更新
		BlockLock Lock(m_ResizeLock);

		m_VideoInfo = VideoInfo;
		//AdjustVideoPosition();
	}

	m_EventListenerList.CallEventListener(&ViewerFilter::EventListener::OnVideoSizeChanged, this, VideoInfo);
}


}	// namespace LibISDB
