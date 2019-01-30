/*
  LibISDB
  Copyright(c) 2017-2019 DBCTRADO

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
 @file   Lock.cpp
 @brief  排他制御
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "Lock.hpp"
#ifdef LIBISDB_WINDOWS
#include "Clock.hpp"
#endif
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


#ifdef LIBISDB_WINDOWS


namespace
{


class TryLockWait
{
public:
	TryLockWait(const std::chrono::milliseconds &Timeout)
		: m_StartTime(m_Clock.Get())
		, m_Timeout(Timeout)
	{
	}

	bool Wait()
	{
		if (m_Clock.Get() - m_StartTime >= static_cast<TickClock::ClockType>(m_Timeout.count()))
			return true;
		::Sleep(1);
		return false;
	}

private:
	TickClock m_Clock;
	const TickClock::ClockType m_StartTime;
	const TickClock::DurationType m_Timeout;
};


}




MutexLock::MutexLock()
{
	::InitializeCriticalSection(&m_CriticalSection);
}


MutexLock::~MutexLock()
{
	::DeleteCriticalSection(&m_CriticalSection);
}


void MutexLock::Lock()
{
	::EnterCriticalSection(&m_CriticalSection);
}


void MutexLock::Unlock()
{
	::LeaveCriticalSection(&m_CriticalSection);
}


bool MutexLock::TryLock()
{
	return ::TryEnterCriticalSection(&m_CriticalSection) != FALSE;
}


bool MutexLock::TryLock(const std::chrono::milliseconds &Timeout)
{
	TryLockWait Wait(Timeout);

	while (!TryLock()) {
		if (Wait.Wait())
			return false;
	}

	return true;
}




SharedLock::SharedLock()
{
	::InitializeSRWLock(&m_Lock);
}


SharedLock::~SharedLock()
{
}


void SharedLock::Lock()
{
	::AcquireSRWLockExclusive(&m_Lock);
}


void SharedLock::Unlock()
{
	::ReleaseSRWLockExclusive(&m_Lock);
}


bool SharedLock::TryLock()
{
	return ::TryAcquireSRWLockExclusive(&m_Lock) != FALSE;
}


bool SharedLock::TryLock(const std::chrono::milliseconds &Timeout)
{
	TryLockWait Wait(Timeout);

	while (!TryLock()) {
		if (Wait.Wait())
			return false;
	}

	return true;
}


void SharedLock::LockShared()
{
	::AcquireSRWLockShared(&m_Lock);
}


void SharedLock::UnlockShared()
{
	::ReleaseSRWLockShared(&m_Lock);
}


bool SharedLock::TryLockShared()
{
	return ::TryAcquireSRWLockShared(&m_Lock) != FALSE;
}


bool SharedLock::TryLockShared(const std::chrono::milliseconds &Timeout)
{
	TryLockWait Wait(Timeout);

	while (!TryLockShared()) {
		if (Wait.Wait())
			return false;
	}

	return true;
}


#else	// LIBISDB_WINDOWS


MutexLock::MutexLock()
{
}


MutexLock::~MutexLock()
{
}


void MutexLock::Lock()
{
	m_Mutex.lock();
}


void MutexLock::Unlock()
{
	m_Mutex.unlock();
}


bool MutexLock::TryLock()
{
	m_Mutex.try_lock();
}


bool MutexLock::TryLock(const std::chrono::milliseconds &Timeout)
{
	return m_Mutex.try_lock_for(Timeout);
}




SharedLock::SharedLock()
{
}


SharedLock::~SharedLock()
{
}


void SharedLock::Lock()
{
	m_Mutex.lock();
}


void SharedLock::Unlock()
{
	m_Mutex.unlock();
}


bool SharedLock::TryLock()
{
	return m_Mutex.try_lock();
}


bool SharedLock::TryLock(const std::chrono::milliseconds &Timeout)
{
	return m_Mutex.try_lock_for(Timeout);
}


void SharedLock::LockShared()
{
	m_Mutex.lock_shared();
}


void SharedLock::UnlockShared()
{
	m_Mutex.unlock_shared();
}


bool SharedLock::TryLockShared()
{
	return m_Mutex.try_lock_shared();
}


bool SharedLock::TryLockShared(const std::chrono::milliseconds &Timeout)
{
	return m_Mutex.try_lock_shared_for(Timeout);
}


#endif	// !def LIBISDB_WINDOWS




const LockGuard::DeferLockT LockGuard::DeferLock;
const LockGuard::AdoptLockT LockGuard::AdoptLock;


}	// namespace LibISDB
