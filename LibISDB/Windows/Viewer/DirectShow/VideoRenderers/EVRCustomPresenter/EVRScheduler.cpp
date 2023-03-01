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
 @file   EVRScheduler.cpp
 @brief  EVR スケジューラ
 @author DBCTRADO
*/


#include "../../../../../LibISDBPrivate.hpp"
#include "../../../../../LibISDBWindows.hpp"
#include "EVRPresenterBase.hpp"
#include "EVRScheduler.hpp"
#include <process.h>
#include "../../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{

namespace
{

enum
{
	MESSAGE_TERMINATE = WM_USER,
	MESSAGE_SCHEDULE,
	MESSAGE_FLUSH
};

}


EVRScheduler::EVRScheduler()
	: m_pCallback(nullptr)
	, m_ThreadID(0)
	, m_hSchedulerThread(nullptr)
	, m_hThreadReadyEvent(nullptr)
	, m_hFlushEvent(nullptr)
	, m_Rate(1.0f)
	, m_LastSampleTime(0)
	, m_PerFrameInterval(0)
	, m_PerFrame_1_4th(0)
{
}


EVRScheduler::~EVRScheduler()
{
}


void EVRScheduler::SetFrameRate(const MFRatio &fps)
{
	UINT64 AvgTimePerFrame = 0;

	::MFFrameRateToAverageTimePerFrame(fps.Numerator, fps.Denominator, &AvgTimePerFrame);

	m_PerFrameInterval = static_cast<MFTIME>(AvgTimePerFrame);
	m_PerFrame_1_4th = m_PerFrameInterval / 4;
}


HRESULT EVRScheduler::StartScheduler(IMFClock *pClock)
{
	if (m_hSchedulerThread != nullptr) {
		return MF_E_UNEXPECTED;
	}

	HRESULT hr = S_OK;

	m_Clock = pClock;

	m_hThreadReadyEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_hThreadReadyEvent == nullptr) {
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto Done;
	}

	m_hFlushEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_hFlushEvent == nullptr) {
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto Done;
	}

	unsigned int ID;
	m_hSchedulerThread = reinterpret_cast<HANDLE>(
		::_beginthreadex(nullptr, 0, SchedulerThreadProc, this, 0, &ID));
	if (m_hSchedulerThread == nullptr) {
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto Done;
	}

	{
		HANDLE Objects[] = {m_hThreadReadyEvent, m_hSchedulerThread};
		const DWORD Wait = ::WaitForMultipleObjects(_countof(Objects), Objects, FALSE, INFINITE);
		if (Wait != WAIT_OBJECT_0) {
			hr = E_UNEXPECTED;
			goto Done;
		}
	}

	m_ThreadID = ID;

Done:
	if (FAILED(hr)) {
		if (m_hSchedulerThread != nullptr) {
			::CloseHandle(m_hSchedulerThread);
			m_hSchedulerThread = nullptr;
		}

		if (m_hFlushEvent != nullptr) {
			::CloseHandle(m_hFlushEvent);
			m_hFlushEvent = nullptr;
		}
	}

	if (m_hThreadReadyEvent != nullptr) {
		::CloseHandle(m_hThreadReadyEvent);
		m_hThreadReadyEvent = nullptr;
	}

	return hr;
}


HRESULT EVRScheduler::StopScheduler()
{
	if (m_hSchedulerThread != nullptr) {
		::PostThreadMessage(m_ThreadID, MESSAGE_TERMINATE, 0, 0);

		::WaitForSingleObject(m_hSchedulerThread, INFINITE);

		::CloseHandle(m_hSchedulerThread);
		m_hSchedulerThread = nullptr;
	}

	if (m_hFlushEvent != nullptr) {
		::CloseHandle(m_hFlushEvent);
		m_hFlushEvent = nullptr;
	}

	m_ScheduledSamples.Clear();

	return S_OK;
}


HRESULT EVRScheduler::Flush()
{
	if (m_hSchedulerThread == nullptr) {
		return S_OK;
	}

	LIBISDB_TRACE(LIBISDB_STR("EVRScheduler::Flush\n"));

	::PostThreadMessage(m_ThreadID, MESSAGE_FLUSH, 0 , 0);

	HANDLE Objects[] = {m_hFlushEvent, m_hSchedulerThread};

	::WaitForMultipleObjects(_countof(Objects), Objects, FALSE, 5000);

	LIBISDB_TRACE(LIBISDB_STR("EVRScheduler::Flush completed.\n"));

	return S_OK;
}


HRESULT EVRScheduler::ScheduleSample(IMFSample *pSample, bool bPresentNow)
{
	if (m_pCallback == nullptr) {
		return MF_E_NOT_INITIALIZED;
	}
	if (m_hSchedulerThread == nullptr) {
		return MF_E_NOT_INITIALIZED;
	}

	DWORD ExitCode;

	::GetExitCodeThread(m_hSchedulerThread, &ExitCode);
	if (ExitCode != STILL_ACTIVE) {
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	if (bPresentNow || !m_Clock) {
		m_pCallback->PresentSample(pSample, 0);
	} else {
		hr = m_ScheduledSamples.Queue(pSample);

		if (SUCCEEDED(hr)) {
			::PostThreadMessage(m_ThreadID, MESSAGE_SCHEDULE, 0, 0);
		}
	}

	return hr;
}


HRESULT EVRScheduler::ProcessSamplesInQueue(LONG *pNextSleep)
{
	HRESULT hr = S_OK;
	LONG Wait = 0;
	IMFSample *pSample = nullptr;

	while (m_ScheduledSamples.Dequeue(&pSample) == S_OK) {
		hr = ProcessSample(pSample, &Wait);
		pSample->Release();

		if (FAILED(hr)) {
			break;
		}
		if (Wait > 0) {
			break;
		}
	}

	if (Wait == 0) {
		Wait = INFINITE;
	}

	*pNextSleep = Wait;

	return hr;
}


HRESULT EVRScheduler::ProcessSample(IMFSample *pSample, LONG *pNextSleep)
{
	HRESULT hr = S_OK;

	LONGLONG hnsPresentationTime = 0;
	LONGLONG hnsTimeNow = 0;
	MFTIME hnsSystemTime = 0;

	bool bPresentNow = true;
	bool bDrop = false;
	LONG NextSleep = 0;

	if (m_Clock) {
		hr = pSample->GetSampleTime(&hnsPresentationTime);

		if (SUCCEEDED(hr)) {
			hr = m_Clock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);
		}

		LONGLONG hnsDelta = hnsPresentationTime - hnsTimeNow;
		if (m_Rate < 0) {
			hnsDelta = -hnsDelta;
		}

		if (hnsDelta < -m_PerFrameInterval * 6) {
			bDrop = true;
		} else if (hnsDelta > 3 * m_PerFrame_1_4th) {
			NextSleep = MFTimeToMsec(hnsDelta - (3 * m_PerFrame_1_4th));

			NextSleep = static_cast<LONG>(static_cast<float>(NextSleep) / std::fabs(m_Rate));

			bPresentNow = false;
		}
	}

	if (!bDrop) {
		if (bPresentNow) {
			hr = m_pCallback->PresentSample(pSample, hnsPresentationTime);
		} else {
			hr = m_ScheduledSamples.PutBack(pSample);
		}
	}
#ifdef LIBISDB_DEBUG
	else {
		LIBISDB_TRACE(LIBISDB_STR("Frame dropped.\n"));
	}
#endif

	*pNextSleep = NextSleep;

	return hr;
}


unsigned int __stdcall EVRScheduler::SchedulerThreadProc(void *pParameter)
{
	EVRScheduler *pScheduler = static_cast<EVRScheduler*>(pParameter);

	return pScheduler->SchedulerThread();
}


unsigned int EVRScheduler::SchedulerThread()
{
	LIBISDB_TRACE(LIBISDB_STR("Start scheduler thread.\n"));

	HRESULT hr = S_OK;
	MSG msg;
	LONG Wait = INFINITE;
	bool ExitSignal = false;

	// メッセージキュー作成
	::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

	::SetEvent(m_hThreadReadyEvent);

	do {
		const DWORD Result = ::MsgWaitForMultipleObjects(0, nullptr, FALSE, Wait, QS_POSTMESSAGE);

		if (Result == WAIT_TIMEOUT) {
			hr = ProcessSamplesInQueue(&Wait);
			if (FAILED(hr)) {
				ExitSignal = true;
			}
		}

		while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			switch (msg.message) {
			case MESSAGE_TERMINATE:
				LIBISDB_TRACE(LIBISDB_STR("MESSAGE_TERMINATE\n"));
				ExitSignal = true;
				break;

			case MESSAGE_FLUSH:
				LIBISDB_TRACE(LIBISDB_STR("MESSAGE_FLUSH\n"));
				m_ScheduledSamples.Clear();
				Wait = INFINITE;
				::SetEvent(m_hFlushEvent);
				break;

			case MESSAGE_SCHEDULE:
				hr = ProcessSamplesInQueue(&Wait);
				if (FAILED(hr)) {
					ExitSignal = true;
				}
				break;
			}
		}
	} while (!ExitSignal);

	LIBISDB_TRACE(LIBISDB_STR("Exit scheduler thread.\n"));

	return SUCCEEDED(hr) ? 0 : 1;
}


}	// namespace LibISDB::DirectShow
