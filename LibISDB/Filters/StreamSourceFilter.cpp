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
 @file   StreamSourceFilter.cpp
 @brief  ストリームソースフィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamSourceFilter.hpp"
#include "../Base/StandardStream.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


StreamSourceFilter::StreamSourceFilter()
	: SourceFilter(SourceMode::Push)
	, m_OutputBufferSize(256 * TS_PACKET_SIZE)
	, m_RequestTimeout(5 * 1000)
	, m_InputBytes(0)
	, m_IsStreaming(false)
{
}


StreamSourceFilter::~StreamSourceFilter()
{
	CloseSource();
}


void StreamSourceFilter::Reset()
{
}


void StreamSourceFilter::ResetGraph()
{
	BlockLock Lock(m_FilterLock);

	if (IsStarted()) {
		AddRequest(RequestType::Reset);
		if (!WaitAllRequests(m_RequestTimeout)) {
			Log(Logger::LogType::Error, LIBISDB_STR("ストリーム読み込みスレッドが応答しません。"));
		}
	} else {
		ResetDownstreamFilters();
		m_EventListenerList.CallEventListener(&EventListener::OnGraphReset, this);
	}
}


bool StreamSourceFilter::StartStreaming()
{
	LIBISDB_TRACE(LIBISDB_STR("StreamSourceFilter::StartStreaming()\n"));

	FilterBase::StartStreaming();

	if (!m_Stream)
		return false;

	if (m_IsStreaming)
		return true;

	m_OutputBuffer.AllocateBuffer(m_OutputBufferSize);

	if (IsStarted()) {
		AddRequest(RequestType::Reset);
		if (!WaitAllRequests(m_RequestTimeout))
			return false;

		m_IsStreaming = true;

		AddRequest(RequestType::Start);
		if (!WaitAllRequests(m_RequestTimeout))
			return false;
	} else {
		m_IsStreaming = true;
	}

	ResetError();

	m_EventListenerList.CallEventListener(&EventListener::OnStreamingStart, this);

	return true;
}


bool StreamSourceFilter::StopStreaming()
{
	LIBISDB_TRACE(LIBISDB_STR("StreamSourceFilter::StopStreaming()\n"));

	if (m_IsStreaming) {
		if (IsStarted()) {
			AddRequest(RequestType::Stop);
			if (!WaitAllRequests(m_RequestTimeout))
				return false;
		}

		m_IsStreaming = false;
	}

	m_OutputBuffer.FreeBuffer();

	ResetError();

	m_EventListenerList.CallEventListener(&EventListener::OnStreamingStop, this);

	return FilterBase::StopStreaming();
}


bool StreamSourceFilter::OpenSource(const CStringView &Name)
{
	if (m_Stream) {
		SetError(std::errc::operation_in_progress);
		return false;
	}

	FileStreamBase *pStream = OpenFileStream(
		Name,
		FileStreamBase::OpenFlag::Read |
		FileStreamBase::OpenFlag::ShareRead |
		FileStreamBase::OpenFlag::ShareWrite |
		FileStreamBase::OpenFlag::ShareDelete |
		FileStreamBase::OpenFlag::SequentialRead);

	if (pStream == nullptr) {
		SetError(std::errc::invalid_argument);
		return false;
	}

	if (!pStream->IsOpen()) {
		SetError(pStream->GetLastErrorDescription());
		delete pStream;
		return false;
	}

	if (!OpenSource(pStream)) {
		delete pStream;
		return false;
	}

	return true;
}


bool StreamSourceFilter::OpenSource(Stream *pStream)
{
	if (LIBISDB_TRACE_ERROR_IF(pStream == nullptr)) {
		SetError(std::errc::invalid_argument);
		return false;
	}

	if (m_Stream) {
		SetError(std::errc::operation_in_progress);
		return false;
	}

	m_Stream.reset(pStream);

	m_IsStreaming = false;

	if (!(m_SourceMode & SourceMode::Pull)) {
		if (!Start()) {
			SetError(std::errc::resource_unavailable_try_again);
			m_Stream.release();
			return false;
		}
	}

	ResetError();

	m_EventListenerList.CallEventListener(&EventListener::OnSourceOpened, this);

	return true;
}


bool StreamSourceFilter::CloseSource()
{
	m_IsStreaming = false;

	if (IsStarted()) {
		// ストリーム読み込みスレッド停止
		Log(Logger::LogType::Information, LIBISDB_STR("ストリーム読み込みスレッドを停止しています..."));
		AddRequest(RequestType::End);
		if (!Wait(std::chrono::milliseconds(5 * 1000))) {
			// スレッド強制終了
			Log(Logger::LogType::Warning, LIBISDB_STR("ストリーム読み込みスレッドが応答しないため強制終了します。"));
			Terminate();
		} else {
			Stop();
		}
	}

	m_Stream.reset();

	m_EventListenerList.CallEventListener(&EventListener::OnSourceClosed, this);

	return true;
}


bool StreamSourceFilter::IsSourceOpen() const
{
	return static_cast<bool>(m_Stream);
}


bool StreamSourceFilter::FetchSource(size_t RequestSize)
{
	if (!m_IsStreaming || !m_Stream || !(m_SourceMode & SourceMode::Pull))
		return false;

	if (RequestSize > m_OutputBuffer.GetBufferSize())
		RequestSize = m_OutputBuffer.GetBufferSize();

	const size_t ReadSize = m_Stream->Read(m_OutputBuffer.GetBuffer(), RequestSize);
	if (ReadSize > 0) {
		m_OutputBuffer.SetSize(ReadSize);
		OutputData(&m_OutputBuffer);
	}

	if ((ReadSize < RequestSize) && m_Stream->IsEnd())
		m_EventListenerList.CallEventListener(&EventListener::OnSourceEnd, this);

	return ReadSize > 0;
}


bool StreamSourceFilter::SetSourceMode(SourceMode Mode)
{
	if (m_Stream)
		return false;

	return SourceFilter::SetSourceMode(Mode);
}


bool StreamSourceFilter::SetOutputBufferSize(size_t Size)
{
	if (Size < 1)
		return false;

	m_OutputBufferSize = Size;

	return true;
}


void StreamSourceFilter::ThreadMain()
{
	LIBISDB_TRACE(LIBISDB_STR("StreamSourceFilter::ThreadMain() begin\n"));

	try {
		StreamingMain();
	} catch (...) {
		Log(Logger::LogType::Error, LIBISDB_STR("ストリーム処理で例外が発生しました。"));
	}

	LIBISDB_TRACE(LIBISDB_STR("StreamSourceFilter::ThreadMain() end\n"));
}


void StreamSourceFilter::StreamingMain()
{
	LockGuard Lock(m_RequestLock);
	bool IsStarted = false;
	std::chrono::milliseconds Wait(0);

	for (;;) {
		m_RequestQueued.WaitFor(m_RequestLock, Wait);

		if (!m_RequestQueue.empty()) {
			StreamingRequest &Req = m_RequestQueue.front();
			Req.IsProcessing = true;
			StreamingRequest Request = Req;
			Lock.Unlock();

			switch (Request.Type) {
			case RequestType::End:
				LIBISDB_TRACE(LIBISDB_STR("End request received\n"));
				break;

			case RequestType::Reset:
				LIBISDB_TRACE(LIBISDB_STR("Reset request received\n"));
				ResetDownstreamFilters();
				m_EventListenerList.CallEventListener(&EventListener::OnGraphReset, this);
				break;

			case RequestType::Start:
				LIBISDB_TRACE(LIBISDB_STR("Start request received\n"));
				IsStarted = true;
				break;

			case RequestType::Stop:
				LIBISDB_TRACE(LIBISDB_STR("Stop request received\n"));
				IsStarted = false;
				break;
			}

			Lock.Lock();
			m_RequestQueue.pop_front();
			m_RequestProcessed.NotifyOne();

			if (Request.Type == RequestType::End)
				break;

			Wait = std::chrono::milliseconds(0);
		} else {
			if (!IsStarted) {
				Wait = std::chrono::milliseconds(1000);
				continue;
			}

			Lock.Unlock();

			const size_t ReadSize = m_Stream->Read(m_OutputBuffer.GetBuffer(), m_OutputBuffer.GetBufferSize());
			if (ReadSize > 0) {
				m_InputBytes += ReadSize;

				if (m_IsStreaming) {
					m_OutputBuffer.SetSize(ReadSize);
					OutputData(&m_OutputBuffer);
				}

				Wait = std::chrono::milliseconds(0);
			} else {
				Wait = std::chrono::milliseconds(10);
			}

			if ((ReadSize < m_OutputBuffer.GetBufferSize()) && m_Stream->IsEnd()) {
				m_EventListenerList.CallEventListener(&EventListener::OnSourceEnd, this);
				Wait = std::chrono::milliseconds(100);
			}

			Lock.Lock();
		}
	}
}


void StreamSourceFilter::AddRequest(RequestType Type)
{
	StreamingRequest Request;

	Request.Type = Type;
	Request.IsProcessing = false;

	m_RequestLock.Lock();
	m_RequestQueue.push_back(Request);
	m_RequestLock.Unlock();
	m_RequestQueued.NotifyOne();
}


bool StreamSourceFilter::WaitAllRequests(const std::chrono::milliseconds &Timeout)
{
	BlockLock Lock(m_RequestLock);

	return m_RequestProcessed.WaitFor(m_RequestLock, Timeout, [this]() -> bool { return m_RequestQueue.empty(); });
}


bool StreamSourceFilter::HasPendingRequest()
{
	m_RequestLock.Lock();
	bool Pending = !m_RequestQueue.empty();
	m_RequestLock.Unlock();
	return Pending;
}


}	// namespace LibISDB
