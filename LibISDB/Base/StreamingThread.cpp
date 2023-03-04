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
 @file   StreamingThread.cpp
 @brief  ストリーミングスレッド
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamingThread.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


StreamingThread::StreamingThread() noexcept
	: m_StreamingThreadEndSignal(false)
	, m_StreamingThreadTimeout(10 * 1000)
	, m_StreamingThreadIdleWait(10)
{
}


StreamingThread::~StreamingThread()
{
	StopStreamingThread();
}


bool StreamingThread::StartStreamingThread()
{
	if (IsStarted())
		return false;

	m_StreamingThreadEndSignal.store(false, std::memory_order_release);

	if (!Start())
		return false;

	return true;
}


void StreamingThread::StopStreamingThread()
{
	if (IsStarted()) {
		m_StreamingThreadEndSignal.store(true, std::memory_order_release);
		m_StreamingThreadCondition.NotifyOne();

		if (!((m_StreamingThreadTimeout.count() > 0) ? Wait(m_StreamingThreadTimeout) : Wait())) {
			// TODO: ログとして出力
			//Log(Logger::LogType::Warning, LIBISDB_STR("Thread {}[{}] not responding, trying to termiante\n"), GetThreadName(), static_cast<void *>(this));
			LIBISDB_TRACE_WARNING(LIBISDB_STR("Thread {}[{}] not responding, trying to terminate\n"), GetThreadName(), static_cast<void *>(this));
			Terminate();
		} else {
			Stop();
		}
	}
}


void StreamingThread::ThreadMain()
{
	LIBISDB_TRACE(LIBISDB_STR("Start thread {}[{}]\n"), GetThreadName(), static_cast<void *>(this));

	try {
		StreamingLoop();
	} catch (...) {
		// TODO: ログとして出力
		//Log(Logger::LogType::Error, LIBISDB_STR("Exception in thread {}[{}]"), GetThreadName(), static_cast<void *>(this));
		LIBISDB_TRACE_ERROR(LIBISDB_STR("Exception in thread {}[{}]\n"), GetThreadName(), static_cast<void *>(this));
	}

	LIBISDB_TRACE(LIBISDB_STR("End thread {}[{}]\n"), GetThreadName(), static_cast<void *>(this));
}


void StreamingThread::StreamingLoop()
{
	LockGuard Lock(m_StreamingThreadLock);
	std::chrono::milliseconds Wait(0);

	for (;;) {
		m_StreamingThreadCondition.WaitFor(m_StreamingThreadLock, Wait);
		if (m_StreamingThreadEndSignal.load(std::memory_order_acquire))
			break;
		Lock.Unlock();

		if (ProcessStream()) {
			Wait = std::chrono::milliseconds(0);
		} else {
			Wait = m_StreamingThreadIdleWait;
		}

		Lock.Lock();
	}
}


}	// namespace LibISDB
