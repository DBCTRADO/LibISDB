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
 @file   BonDriverSourceFilter.cpp
 @brief  BonDriver ソースフィルタ
 @author DBCTRADO
*/


#include "../../LibISDBPrivate.hpp"
#include "../../LibISDBWindows.hpp"
#include "BonDriverSourceFilter.hpp"
#include "../../Utilities/StringFormat.hpp"
#include <thread>
#include <objbase.h>
#include "../../Base/DebugDef.hpp"


namespace LibISDB
{


BonDriverSourceFilter::ErrorCategory BonDriverSourceFilter::m_ErrorCategory;


BonDriverSourceFilter::BonDriverSourceFilter()
	: SourceFilter(SourceMode::Push)
	, m_RequestTimeout(10 * 1000)
	, m_IsStreaming(false)
	, m_StreamRemain(0)
	, m_SignalLevel(0.0f)
	, m_StreamingThreadPriority(THREAD_PRIORITY_NORMAL)
	, m_PurgeStreamOnChannelChange(true)
	, m_FirstChannelSetDelay(0)
	, m_MinChannelChangeInterval(0)
	, m_SetChannelCount(0)
{
}


BonDriverSourceFilter::~BonDriverSourceFilter()
{
	CloseSource();
}


void BonDriverSourceFilter::Reset()
{
	if (!m_BonDriver.IsIBonDriverCreated())
		return;

	if (HasPendingRequest()) {
		Log(Logger::LogType::Error, LIBISDB_STR("前回の要求が完了しないため新しい要求を行えません。"));
		return;
	}

	// 未処理のストリームを破棄する
	StreamingRequest Request;
	Request.Type = StreamingRequest::RequestType::PurgeStream;
	AddRequest(Request);

	if (!WaitAllRequests(m_RequestTimeout)) {
		Log(Logger::LogType::Error, LIBISDB_STR("ストリーム受信スレッドが応答しません。"));
	}
}


void BonDriverSourceFilter::ResetGraph()
{
	BlockLock Lock(m_FilterLock);

	if (!m_BonDriver.IsIBonDriverCreated()) {
		ResetDownstreamFilters();
		m_EventListenerList.CallEventListener(&SourceFilter::EventListener::OnGraphReset, this);
	} else {
		if (HasPendingRequest()) {
			Log(Logger::LogType::Error, LIBISDB_STR("前回の要求が完了しないため新しい要求を行えません。"));
			return;
		}

		StreamingRequest Request[2];
		Request[0].Type = StreamingRequest::RequestType::PurgeStream;
		Request[1].Type = StreamingRequest::RequestType::Reset;
		AddRequests(Request, 2);

		if (!WaitAllRequests(m_RequestTimeout)) {
			Log(Logger::LogType::Error, LIBISDB_STR("ストリーム受信スレッドが応答しません。"));
		}
	}
}


bool BonDriverSourceFilter::StartStreaming()
{
	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::StartStreaming()\n"));

	FilterBase::StartStreaming();

	if (!m_BonDriver.IsIBonDriverCreated()) {
		// チューナが開かれていない
		SetError(ErrorCode::TunerNotOpened);
		return false;
	}

	if (m_IsStreaming.load(std::memory_order_acquire)) {
		ResetError();
		return true;
	}

	if (HasPendingRequest()) {
		SetError(ErrorCode::Pending, LIBISDB_STR("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request[2];

	// 未処理のストリームを破棄する
	Request[0].Type = StreamingRequest::RequestType::PurgeStream;
	// 下流フィルタをリセットする
	Request[1].Type = StreamingRequest::RequestType::Reset;

	AddRequests(Request, 2);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	// ストリームを再生状態にする
	m_IsStreaming.store(true, std::memory_order_relaxed);

	ResetError();

	m_EventListenerList.CallEventListener(&EventListener::OnStreamingStart, this);

	return true;
}


bool BonDriverSourceFilter::StopStreaming()
{
	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::StopStreaming()\n"));

	if (!m_IsStreaming.exchange(false, std::memory_order_relaxed)) {
		return true;
	}

	ResetError();

	m_EventListenerList.CallEventListener(&EventListener::OnStreamingStop, this);

	return FilterBase::StopStreaming();
}


bool BonDriverSourceFilter::OpenSource(const CStringView &Name)
{
	if (!LoadBonDriver(Name))
		return false;

	if (!OpenTuner()) {
		UnloadBonDriver();
		return false;
	}

	m_EventListenerList.CallEventListener(&EventListener::OnSourceOpened, this);

	return true;
}


bool BonDriverSourceFilter::CloseSource()
{
	if (!UnloadBonDriver())
		return false;

	m_EventListenerList.CallEventListener(&EventListener::OnSourceClosed, this);

	return true;
}


bool BonDriverSourceFilter::IsSourceOpen() const
{
	return IsTunerOpen();
}


bool BonDriverSourceFilter::LoadBonDriver(const CStringView &FileName)
{
	if (m_BonDriver.IsLoaded()) {
		SetError(ErrorCode::AlreadyLoaded, LIBISDB_STR("既に読み込まれています。"));
		return false;
	}

	if (FileName.empty()) {
		SetError(std::errc::invalid_argument, LIBISDB_STR("ファイルが指定されていません。"));
		return false;
	}

	Log(Logger::LogType::Information, LIBISDB_STR("BonDriver \"{}\" を読み込みます..."), FileName);

	if (!m_BonDriver.Load(FileName)) {
		const DWORD ErrorCode = ::GetLastError();
		CharType Text[MAX_PATH + 64];

		StringFormat(Text, LIBISDB_STR("\"{}\" が読み込めません。"), FileName);
		SetError(ErrorCode, std::system_category(), Text);

		switch (ErrorCode) {
		case ERROR_MOD_NOT_FOUND:
			SetErrorAdvise(LIBISDB_STR("ファイルが見つかりません。"));
			break;

		case ERROR_BAD_EXE_FORMAT:
			SetErrorAdvise(
#ifndef _WIN64
				LIBISDB_STR("32")
#else
				LIBISDB_STR("64")
#endif
				LIBISDB_STR("ビット用の BonDriver ではないか、ファイルが破損している可能性があります。"));
			break;

		case ERROR_SXS_CANT_GEN_ACTCTX:
			SetErrorAdvise(LIBISDB_STR("この BonDriver に必要なランタイムがインストールされていない可能性があります。"));
			break;

		default:
			StringFormat(Text, LIBISDB_STR("エラーコード: {:#x}"), ErrorCode);
			SetErrorAdvise(Text);
		}

		SetErrorSystemMessageByWin32ErrorCode(ErrorCode);

		return false;
	}

	Log(Logger::LogType::Information, LIBISDB_STR("BonDriver を読み込みました。"));

	ResetError();

	return true;
}


bool BonDriverSourceFilter::UnloadBonDriver()
{
	if (m_BonDriver.IsLoaded()) {
		CloseTuner();

		Log(Logger::LogType::Information, LIBISDB_STR("BonDriver を解放します..."));
		m_BonDriver.Unload();
		Log(Logger::LogType::Information, LIBISDB_STR("BonDriver を解放しました。"));
	}

	return true;
}


bool BonDriverSourceFilter::IsBonDriverLoaded() const
{
	return m_BonDriver.IsLoaded();
}


bool BonDriverSourceFilter::OpenTuner()
{
	if (!m_BonDriver.IsLoaded()) {
		SetError(ErrorCode::NotLoaded, LIBISDB_STR("BonDriverが読み込まれていません。"));
		return false;
	}

	// オープンチェック
	if (m_BonDriver.IsIBonDriverCreated()) {
		SetError(ErrorCode::TunerAlreadyOpened, LIBISDB_STR("チューナは既に開かれています。"));
		return false;
	}

	Log(Logger::LogType::Information, LIBISDB_STR("チューナを開いています..."));

	if (!m_BonDriver.CreateIBonDriver()) {
		SetError(ErrorCode::CreateBonDriverFailed, LIBISDB_STR("IBonDriver を作成できません。"));
		return false;
	}

	const HANDLE hThread = ::GetCurrentThread();
	const int ThreadPriority = ::GetThreadPriority(hThread);
	bool TunerOpened = false;

	try {
		// チューナを開く
		TunerOpened = m_BonDriver.OpenTuner();

		// なぜかスレッドの優先度を変えるBonDriverがあるので元に戻す
		::SetThreadPriority(hThread, ThreadPriority);

		if (!TunerOpened) {
			SetError(
				ErrorCode::TunerOpenFailed,
				LIBISDB_STR("チューナを開けません。"),
				LIBISDB_STR("BonDriver にチューナを開くよう要求しましたがエラーが返されました。"));
			throw ErrorCode::TunerOpenFailed;
		}

		m_SetChannelCount = 0;
		m_TunerOpenTime = m_Clock.Get();

		// ストリーム受信スレッド起動
		m_IsStreaming.store(false, std::memory_order_release);
		if (!Start()) {
			SetError(std::errc::resource_unavailable_try_again, LIBISDB_STR("ストリーム受信スレッドを作成できません。"));
			throw std::errc::resource_unavailable_try_again;
		}
	} catch (...) {
		if (TunerOpened)
			m_BonDriver.CloseTuner();
		m_BonDriver.ReleaseIBonDriver();
		return false;
	}

	Log(Logger::LogType::Information, LIBISDB_STR("チューナを開きました。"));

	ResetError();

	return true;
}


bool BonDriverSourceFilter::CloseTuner()
{
	// ストリーム停止
	m_IsStreaming.store(false, std::memory_order_release);

	if (IsStarted()) {
		// ストリーム受信スレッド停止
		Log(Logger::LogType::Information, LIBISDB_STR("ストリーム受信スレッドを停止しています..."));
		StreamingRequest Request;
		Request.Type = StreamingRequest::RequestType::End;
		AddRequest(Request);
		if (!Wait(std::chrono::milliseconds(5 * 1000))) {
			// スレッド強制終了
			Log(Logger::LogType::Warning, LIBISDB_STR("ストリーム受信スレッドが応答しないため強制終了します。"));
			Terminate();
		} else {
			Stop();
		}
	}

	m_RequestQueue.clear();

	if (m_BonDriver.IsIBonDriverCreated()) {
		// チューナを閉じる
		Log(Logger::LogType::Information, LIBISDB_STR("チューナを閉じています..."));
		m_BonDriver.CloseTuner();

		// ドライバインスタンス開放
		Log(Logger::LogType::Information, LIBISDB_STR("BonDriver インターフェースを解放しています..."));
		m_BonDriver.ReleaseIBonDriver();
		Log(Logger::LogType::Information, LIBISDB_STR("BonDriver インターフェースを解放しました。"));
	}

	ResetStatus();

	m_SetChannelCount = 0;

	ResetError();

	return true;
}


bool BonDriverSourceFilter::IsTunerOpen() const
{
	return m_BonDriver.IsTunerOpen();
}


bool BonDriverSourceFilter::SetChannel(uint8_t Channel)
{
	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::SetChannel({})\n"), Channel);

	if (!IsTunerOpen()) {
		// チューナが開かれていない
		SetError(ErrorCode::TunerNotOpened);
		return false;
	}

	if (HasPendingRequest()) {
		SetError(ErrorCode::Pending, LIBISDB_STR("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request[3];
	int RequestCount = 0;

	// 未処理のストリームを破棄する
	if (m_PurgeStreamOnChannelChange) {
		Request[RequestCount].Type = StreamingRequest::RequestType::PurgeStream;
		RequestCount++;
	}

	// チャンネルを変更する
	Request[RequestCount].Type = StreamingRequest::RequestType::SetChannel;
	Request[RequestCount].SetChannel.Channel = Channel;
	RequestCount++;

	// 下流フィルタをリセットする
	Request[RequestCount].Type = StreamingRequest::RequestType::Reset;
	RequestCount++;

	AddRequests(Request, RequestCount);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	if (!m_RequestResult) {
		SetError(
			ErrorCode::SetChannelFailed,
			LIBISDB_STR("チャンネルの変更が BonDriver に受け付けられません。"),
			LIBISDB_STR("IBonDriver::SetChannel() の呼び出しでエラーが返されました。"));
		return false;
	}

	ResetError();

	return true;
}


bool BonDriverSourceFilter::SetChannel(uint32_t Space, uint32_t Channel)
{
	LIBISDB_TRACE(
		LIBISDB_STR("BonDriverSourceFilter::SetChannel({}, {})\n"),
		Space, Channel);

	if (!m_BonDriver.IsIBonDriver2()) {
		// チューナが開かれていない
		SetError(ErrorCode::TunerNotOpened);
		return false;
	}

	if (HasPendingRequest()) {
		SetError(ErrorCode::Pending, LIBISDB_STR("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request[3];
	int RequestCount = 0;

	// 未処理のストリームを破棄する
	if (m_PurgeStreamOnChannelChange) {
		Request[RequestCount].Type = StreamingRequest::RequestType::PurgeStream;
		RequestCount++;
	}

	// チャンネルを変更する
	Request[RequestCount].Type = StreamingRequest::RequestType::SetChannel2;
	Request[RequestCount].SetChannel2.Space = Space;
	Request[RequestCount].SetChannel2.Channel = Channel;
	RequestCount++;

	// 下流フィルタをリセットする
	Request[RequestCount].Type = StreamingRequest::RequestType::Reset;
	RequestCount++;

	AddRequests(Request, RequestCount);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	if (!m_RequestResult) {
		SetError(
			ErrorCode::SetChannelFailed,
			LIBISDB_STR("チャンネルの変更が BonDriver に受け付けられません。"),
			LIBISDB_STR("IBonDriver2::SetChannel() の呼び出しでエラーが返されました。"));
		return false;
	}

	ResetError();

	return true;
}


bool BonDriverSourceFilter::SetChannelAndPlay(uint32_t Space, uint32_t Channel)
{
	LIBISDB_TRACE(
		LIBISDB_STR("BonDriverSourceFilter::SetChannelAndPlay({}, {})\n"),
		Space, Channel);

	if (!SetChannel(Space, Channel))
		return false;

	m_IsStreaming.store(true, std::memory_order_relaxed);

	return true;
}


uint32_t BonDriverSourceFilter::GetCurSpace() const
{
	if (!m_BonDriver.IsIBonDriver2()) {
		//SetError(ErrorCode::TunerNotOpened);
		return SPACE_INVALID;
	}
	return m_BonDriver.GetCurSpace();
}


uint32_t BonDriverSourceFilter::GetCurChannel() const
{
	if (!m_BonDriver.IsIBonDriver2()) {
		//SetError(ErrorCode::TunerNotOpened);
		return CHANNEL_INVALID;
	}
	return m_BonDriver.GetCurChannel();
}


bool BonDriverSourceFilter::IsIBonDriver2() const
{
	return m_BonDriver.IsIBonDriver2();
}


const IBonDriver2::CharType * BonDriverSourceFilter::GetSpaceName(uint32_t Space) const
{
	// チューニング空間名を返す
	if (!m_BonDriver.IsIBonDriver2()) {
		//SetError(ErrorCode::TunerNotOpened);
		return nullptr;
	}
	return m_BonDriver.EnumTuningSpace(Space);
}


const IBonDriver2::CharType * BonDriverSourceFilter::GetChannelName(uint32_t Space, uint32_t Channel) const
{
	// チャンネル名を返す
	if (!m_BonDriver.IsIBonDriver2()) {
		//SetError(ErrorCode::TunerNotOpened);
		return nullptr;
	}
	return m_BonDriver.EnumChannelName(Space, Channel);
}


int BonDriverSourceFilter::GetSpaceCount() const
{
	if (!m_BonDriver.IsIBonDriver2())
		return 0;

	uint32_t i;
	for (i = 0; m_BonDriver.EnumTuningSpace(i) != nullptr; i++);
	return i;
}


const IBonDriver2::CharType * BonDriverSourceFilter::GetTunerName() const
{
	if (!m_BonDriver.IsIBonDriver2()) {
		//SetError(ErrorCode::TunerNotOpened);
		return nullptr;
	}
	return m_BonDriver.GetTunerName();
}


bool BonDriverSourceFilter::PurgeStream()
{
	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::PurgeStream()\n"));

	if (!IsTunerOpen()) {
		SetError(ErrorCode::TunerNotOpened);
		return false;
	}

	if (HasPendingRequest()) {
		SetError(ErrorCode::Pending, LIBISDB_STR("前回の要求が完了しないため新しい要求を行えません。"));
		return false;
	}

	StreamingRequest Request;
	Request.Type = StreamingRequest::RequestType::PurgeStream;
	AddRequest(Request);

	if (!WaitAllRequests(m_RequestTimeout)) {
		SetRequestTimeoutError();
		return false;
	}

	ResetError();

	return true;
}


float BonDriverSourceFilter::GetSignalLevel() const
{
	return m_SignalLevel;
}


unsigned long BonDriverSourceFilter::GetBitRate() const
{
	return m_BitRateCalculator.GetBitRate();
}


uint32_t BonDriverSourceFilter::GetStreamRemain() const
{
	return m_StreamRemain;
}


bool BonDriverSourceFilter::SetStreamingThreadPriority(int Priority)
{
	if (m_StreamingThreadPriority != Priority) {
		LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::SetStreamingThreadPriority({})\n"), Priority);

		if (m_hThread != nullptr) {
			if (!::SetThreadPriority(m_hThread, Priority))
				return false;
		}

		m_StreamingThreadPriority = Priority;
	}

	return true;
}


void BonDriverSourceFilter::SetPurgeStreamOnChannelChange(bool Purge)
{
	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::SetPurgeStreamOnChannelChange({})\n"), Purge);

	m_PurgeStreamOnChannelChange = Purge;
}


bool BonDriverSourceFilter::SetFirstChannelSetDelay(unsigned long Delay)
{
	if (Delay > FIRST_CHANNEL_SET_DELAY_MAX)
		return false;

	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::SetFirstChannelSetDelay({})\n"), Delay);

	m_FirstChannelSetDelay = Delay;

	return true;
}


bool BonDriverSourceFilter::SetMinChannelChangeInterval(unsigned long Interval)
{
	if (Interval > CHANNEL_CHANGE_INTERVAL_MAX)
		return false;

	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::SetMinChannelChangeInterval({})\n"), Interval);

	m_MinChannelChangeInterval = Interval;

	return true;
}


void BonDriverSourceFilter::ThreadMain()
{
	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::ThreadMain() begin\n"));

	::CoInitialize(nullptr);

	::SetThreadPriority(::GetCurrentThread(), m_StreamingThreadPriority);

	try {
		StreamingMain();
	} catch (...) {
		Log(Logger::LogType::Error, LIBISDB_STR("ストリーム処理で例外が発生しました。"));
	}

	::CoUninitialize();

	LIBISDB_TRACE(LIBISDB_STR("BonDriverSourceFilter::ThreadMain() end\n"));
}


void BonDriverSourceFilter::StreamingMain()
{
	m_BitRateCalculator.Initialize();

	DataBuffer StreamBuffer(0x10000_z);
	LockGuard Lock(m_RequestLock);
	std::chrono::milliseconds Wait(0);

	for (;;) {
		m_RequestQueued.WaitFor(m_RequestLock, Wait);

		if (!m_RequestQueue.empty()) {
			StreamingRequest &Req = m_RequestQueue.front();
			Req.IsProcessing = true;
			const StreamingRequest Request = Req;
			Lock.Unlock();

			switch (Request.Type) {
			case StreamingRequest::RequestType::End:
				LIBISDB_TRACE(LIBISDB_STR("End request received\n"));
				break;

			case StreamingRequest::RequestType::Reset:
				LIBISDB_TRACE(LIBISDB_STR("Reset request received\n"));
				ResetStatus();
				ResetDownstreamFilters();
				m_EventListenerList.CallEventListener(&SourceFilter::EventListener::OnGraphReset, this);
				break;

			case StreamingRequest::RequestType::SetChannel:
				SetChannelWait();
				LIBISDB_TRACE(LIBISDB_STR("IBonDriver::SetChannel({})\n"), Request.SetChannel.Channel);
				m_RequestResult = m_BonDriver.SetChannel(Request.SetChannel.Channel);
				m_SetChannelTime = m_Clock.Get();
				if (m_RequestResult)
					m_EventListenerList.CallEventListener(&SourceFilter::EventListener::OnSourceChanged, this);
				else
					m_EventListenerList.CallEventListener(&SourceFilter::EventListener::OnSourceChangeFailed, this);
				break;

			case StreamingRequest::RequestType::SetChannel2:
				SetChannelWait();
				LIBISDB_TRACE(
					LIBISDB_STR("IBonDriver2::SetChannel({}, {})\n"),
					Request.SetChannel2.Space,
					Request.SetChannel2.Channel);
				m_RequestResult = m_BonDriver.SetChannel(
						Request.SetChannel2.Space, Request.SetChannel2.Channel);
				m_SetChannelTime = m_Clock.Get();
				if (m_RequestResult)
					m_EventListenerList.CallEventListener(&SourceFilter::EventListener::OnSourceChanged, this);
				else
					m_EventListenerList.CallEventListener(&SourceFilter::EventListener::OnSourceChangeFailed, this);
				break;

			case StreamingRequest::RequestType::PurgeStream:
				LIBISDB_TRACE(LIBISDB_STR("IBonDriver::PurgeStream()\n"));
				m_BonDriver.PurgeTsStream();
				break;
			}

			Lock.Lock();
			m_RequestQueue.pop_front();
			m_RequestProcessed.NotifyOne();

			if (Request.Type == StreamingRequest::RequestType::End)
				break;

			Wait = std::chrono::milliseconds(0);
		} else {
			Lock.Unlock();

			uint8_t *pStreamData = nullptr;
			uint32_t StreamSize = 0;
			uint32_t StreamRemain = 0;

			if (m_BonDriver.GetTsStream(&pStreamData, &StreamSize, &StreamRemain)
					&& (pStreamData != nullptr) && (StreamSize > 0)) {
				if (m_IsStreaming.load(std::memory_order_acquire)) {
					// 次のフィルタにデータを渡す
					StreamBuffer.SetData(pStreamData, StreamSize);
					OutputData(&StreamBuffer);
				}
			} else {
				StreamSize = 0;
				StreamRemain = 0;
			}

			if (m_BitRateCalculator.Update(StreamSize))
				m_SignalLevel = m_BonDriver.GetSignalLevel();
			m_StreamRemain = StreamRemain;

			if (StreamRemain != 0)
				Wait = std::chrono::milliseconds(0);
			else
				Wait = std::chrono::milliseconds(10);

			Lock.Lock();
		}
	}
}


void BonDriverSourceFilter::ResetStatus()
{
	m_SignalLevel = 0.0f;
	m_BitRateCalculator.Reset();
	m_StreamRemain = 0;
}


void BonDriverSourceFilter::AddRequest(const StreamingRequest &Request)
{
	StreamingRequest Req = Request;

	Req.IsProcessing = false;

	m_RequestLock.Lock();
	m_RequestQueue.push_back(Req);
	m_RequestLock.Unlock();
	m_RequestQueued.NotifyOne();
}


void BonDriverSourceFilter::AddRequests(const StreamingRequest *pRequestList, int RequestCount)
{
	m_RequestLock.Lock();

	for (int i = 0; i < RequestCount; i++) {
		StreamingRequest Request = pRequestList[i];
		Request.IsProcessing = false;
		m_RequestQueue.push_back(Request);
	}

	m_RequestLock.Unlock();
	m_RequestQueued.NotifyOne();
}


bool BonDriverSourceFilter::WaitAllRequests(const std::chrono::milliseconds &Timeout)
{
	BlockLock Lock(m_RequestLock);

	return m_RequestProcessed.WaitFor(m_RequestLock, Timeout, [this]() -> bool { return m_RequestQueue.empty(); });
}


bool BonDriverSourceFilter::HasPendingRequest()
{
	m_RequestLock.Lock();
	const bool Pending = !m_RequestQueue.empty();
	m_RequestLock.Unlock();
	return Pending;
}


void BonDriverSourceFilter::SetRequestTimeoutError()
{
	StreamingRequest Request;
	bool HasRequest = false;

	m_RequestLock.Lock();
	if (!m_RequestQueue.empty()) {
		Request = m_RequestQueue.front();
		HasRequest = true;
	}
	m_RequestLock.Unlock();

	if (HasRequest && Request.IsProcessing) {
		const CharType *pText;

		switch (Request.Type) {
		case StreamingRequest::RequestType::SetChannel:
		case StreamingRequest::RequestType::SetChannel2:
			pText = LIBISDB_STR("BonDriver にチャンネル変更を要求しましたが応答がありません。");
			break;

		case StreamingRequest::RequestType::PurgeStream:
			pText = LIBISDB_STR("BonDriver に残りデータの破棄を要求しましたが応答がありません。");
			break;

		case StreamingRequest::RequestType::Reset:
			pText = LIBISDB_STR("リセット処理が完了しません。");
			break;

		default:
			pText = LIBISDB_STR("Internal error (Invalid request type)");
			break;
		}

		SetError(std::errc::timed_out, pText);
	} else {
		SetError(std::errc::timed_out, LIBISDB_STR("ストリーム受信スレッドが応答しません。"));
	}
}


void BonDriverSourceFilter::SetChannelWait()
{
	TickClock::ClockType Time, Wait;

	if (m_SetChannelCount == 0) {
		Time = m_TunerOpenTime;
		Wait = m_FirstChannelSetDelay;
	} else {
		Time = m_SetChannelTime;
		Wait = m_MinChannelChangeInterval;
	}

	if (Wait > 0) {
		const TickClock::ClockType Interval = m_Clock.Get() - Time;
		if (Interval < Wait) {
			LIBISDB_TRACE(LIBISDB_STR("SetChannel wait {}\n"), Wait - Interval);
			std::this_thread::sleep_for(
				std::chrono::milliseconds((Wait - Interval) * 1000 / TickClock::ClocksPerSec));
		}
	}

	m_SetChannelCount++;
}


std::string BonDriverSourceFilter::ErrorCategory::message(int ev) const
{
	switch (static_cast<ErrorCode>(ev)) {
	case ErrorCode::NotLoaded:
		return std::string("BonDriver not loaded");
	case ErrorCode::AlreadyLoaded:
		return std::string("BonDriver already loaded");
	case ErrorCode::CreateBonDriverFailed:
		return std::string("CreateBonDriver failed");
	case ErrorCode::TunerOpenFailed:
		return std::string("OpenTuner failed");
	case ErrorCode::TunerNotOpened:
		return std::string("Tuner not opened");
	case ErrorCode::TunerAlreadyOpened:
		return std::string("Tuner already opened");
	case ErrorCode::SetChannelFailed:
		return std::string("SetChannel failed");
	case ErrorCode::Pending:
		return std::string("Previous requests not completed");
	}

	return std::string();
}


}	// namespace LibISDB
