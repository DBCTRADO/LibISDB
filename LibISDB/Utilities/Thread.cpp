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
 @file   Thread.cpp
 @brief  スレッド
 @author DBCTRADO
*/


#include "../LibISDBBase.hpp"
#include "Thread.hpp"
#ifdef LIBISDB_WINDOWS
#include <process.h>
#include <ProcessThreadsApi.h>
#else
#include <chrono>
#endif
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


Thread::Thread()
{
}


Thread::~Thread()
{
	Stop();
}


#ifdef LIBISDB_WINDOWS


bool Thread::Start()
{
	if (m_hThread != nullptr)
		return false;

	m_hThread = reinterpret_cast<HANDLE>(
		::_beginthreadex(
			nullptr, 0,
			[](void *pParam) -> unsigned int {
				Thread *pThis = static_cast<Thread *>(pParam);
				pThis->SetThreadName(pThis->GetThreadName());
				pThis->ThreadMain();
				return 0U;
			},
			this, 0, &m_ThreadID));
	if (m_hThread == nullptr)
		return false;

	return true;
}


void Thread::Stop()
{
	if (m_hThread != nullptr) {
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
		m_ThreadID = 0;
	}
}


bool Thread::IsStarted() const
{
	return m_hThread != nullptr;
}


bool Thread::Wait() const
{
	if (m_hThread == nullptr)
		return false;

	return ::WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
}


bool Thread::Wait(const std::chrono::milliseconds &Timeout) const
{
	if (m_hThread == nullptr)
		return false;

	DWORD Wait = static_cast<DWORD>(std::min(Timeout.count(), static_cast<std::chrono::milliseconds::rep>(0xFFFFFFFEUL)));

	return ::WaitForSingleObject(m_hThread, Wait) == WAIT_OBJECT_0;
}


void Thread::Terminate()
{
	if (m_hThread != nullptr) {
		::TerminateThread(m_hThread, -1);
		::CloseHandle(m_hThread);
		m_hThread = nullptr;
		m_ThreadID = 0;
	}
}


void Thread::SetThreadName(const CharType *pName)
{
	auto pSetThreadDescription =
		reinterpret_cast<decltype(SetThreadDescription) *>(
			::GetProcAddress(::GetModuleHandle(TEXT("kernel32.dll")), "SetThreadDescription"));
	if (pSetThreadDescription != nullptr) {
#ifdef LIBISDB_WCHAR
		pSetThreadDescription(::GetCurrentThread(), pName);
#else
		WCHAR Name[256];
		::MultiByteToWideChar(CP_ACP, 0, pName, -1, Name, (int)CountOf(Name));
		pSetThreadDescription(::GetCurrentThread(), Name);
#endif
		return;
	}

#if defined(LIBISDB_DEBUG) && defined(LIBISDB_ENABLE_SEH)
	// How to: Set a Thread Name in Native Code
	// https://msdn.microsoft.com/en-us/library/xcb2z8hs(VS.90).aspx

	if (::IsDebuggerPresent()) {
		static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
		typedef struct tagTHREADNAME_INFO
		{
			DWORD dwType;     // Must be 0x1000
			LPCSTR szName;    // Pointer to name (in user addr space)
			DWORD dwThreadID; // Thread ID (-1=caller thread)
			DWORD dwFlags;    // Reserved for future use.  Must be zero
		} THREADNAME_INFO;
#pragma pack(pop)

		THREADNAME_INFO Info;

		Info.dwType = 0x1000;
#ifdef LIBISDB_WCHAR
		char Name[256];
		::WideCharToMultiByte(CP_ACP, 0, pName, -1, Name, sizeof(Name), nullptr, nullptr);
		Info.szName = Name;
#else
		Info.szName = pName;
#endif
		Info.dwThreadID = static_cast<DWORD>(-1);
		Info.dwFlags = 0;

		__try {
			::RaiseException(
				MS_VC_EXCEPTION, 0, sizeof(Info) / sizeof(ULONG_PTR),
				reinterpret_cast<ULONG_PTR *>(&Info));
		} __except(EXCEPTION_CONTINUE_EXECUTION) {
		}
	}
#endif
}


#else	// LIBISDB_WINDOWS


bool Thread::Start()
{
	if (m_Future.valid())
		return false;

	try {
		m_Future = std::async(std::launch::async, [this]() { ThreadMain(); });
	} catch (...) {
		return false;
	}

	return true;
}


void Thread::Stop()
{
	if (m_Future.valid()) {
		m_Future.wait();
	}
}


bool Thread::IsStarted() const
{
	return m_Future.valid();
}


bool Thread::Wait() const
{
	if (!m_Future.valid())
		return false;

	m_Future.wait();

	return true;
}


bool Thread::Wait(const std::chrono::milliseconds &Timeout) const
{
	if (!m_Future.valid())
		return false;

	return m_Future.wait_for(Timeout) == std::future_status::ready;
}


void Thread::Terminate()
{
	Stop();
}


void Thread::SetThreadName(const CharType *pName)
{
}


#endif	// ndef LIBISDB_WINDOWS


}	// namespace LibISDB
