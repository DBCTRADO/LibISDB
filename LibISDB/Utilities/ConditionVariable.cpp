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
 @file   ConditionVariable.cpp
 @brief  条件変数
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "ConditionVariable.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


#ifdef LIBISDB_WINDOWS


ConditionVariable::ConditionVariable()
{
	::InitializeConditionVariable(&m_ConditionVariable);
}


void ConditionVariable::NotifyOne()
{
	::WakeConditionVariable(&m_ConditionVariable);
}


void ConditionVariable::NotifyAll()
{
	::WakeAllConditionVariable(&m_ConditionVariable);
}


void ConditionVariable::Wait(MutexLock &Lock)
{
	::SleepConditionVariableCS(&m_ConditionVariable, &Lock.Native(), INFINITE);
}


bool ConditionVariable::WaitFor(MutexLock &Lock, const std::chrono::milliseconds &Timeout)
{
	LIBISDB_ASSERT(Timeout.count() < 0xFFFFFFFFLL);
	return ::SleepConditionVariableCS(&m_ConditionVariable, &Lock.Native(), static_cast<DWORD>(Timeout.count())) != FALSE;
}


#else	// LIBISDB_WINDOWS


ConditionVariable::ConditionVariable()
{
}


void ConditionVariable::NotifyOne()
{
	m_ConditionVariable.notify_one();
}


void ConditionVariable::NotifyAll()
{
	m_ConditionVariable.notify_all();
}


void ConditionVariable::Wait(MutexLock &Lock)
{
	m_ConditionVariable.wait(Lock.Native());
}


bool ConditionVariable::WaitFor(MutexLock &Lock, const std::chrono::milliseconds &Timeout)
{
	std::unique_lock<std::recursive_timed_mutex> UniqueLock(Lock.Native(), std::adopt_lock);

	return m_ConditionVariable.wait_for(UniqueLock, Timeout) == std::cv_status::no_timeout;
}


#endif	// !def LIBISDB_WINDOWS


}	// namespace LibISDB
