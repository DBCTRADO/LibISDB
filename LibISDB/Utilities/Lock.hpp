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
 @file   Lock.hpp
 @brief  排他制御
 @author DBCTRADO
*/


#ifndef LIBISDB_LOCK_H
#define LIBISDB_LOCK_H


#include <chrono>
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#else
#include <mutex>
#include <shared_mutex>
#endif


namespace LibISDB
{

	class MutexLock
	{
	public:
		MutexLock();
		~MutexLock();
		MutexLock(const MutexLock &) = delete;
		MutexLock & operator = (const MutexLock &) = delete;

		void Lock();
		void Unlock();
		bool TryLock();
		bool TryLock(const std::chrono::milliseconds &Timeout);

#ifdef LIBISDB_WINDOWS
		::CRITICAL_SECTION & Native() { return m_CriticalSection; }
#else
		std::recursive_timed_mutex & Native() { return m_Mutex; }
#endif

	private:
#ifdef LIBISDB_WINDOWS
		::CRITICAL_SECTION m_CriticalSection;
#else
		std::recursive_timed_mutex m_Mutex;
#endif
	};

	class SharedLock
	{
	public:
		SharedLock();
		~SharedLock();
		SharedLock(const SharedLock &) = delete;
		SharedLock & operator = (const SharedLock &) = delete;

		void Lock();
		void Unlock();
		bool TryLock();
		bool TryLock(const std::chrono::milliseconds &Timeout);

		void LockShared();
		void UnlockShared();
		bool TryLockShared();
		bool TryLockShared(const std::chrono::milliseconds &Timeout);

#ifdef LIBISDB_WINDOWS
		::SRWLOCK & Native() { return m_Lock; }
#else
		std::shared_timed_mutex & Native() { return m_Mutex; }
#endif

	private:
#ifdef LIBISDB_WINDOWS
		::SRWLOCK m_Lock;
#else
		std::shared_timed_mutex m_Mutex;
#endif
	};

	template<typename TLock> class LockGuard
	{
	public:
		struct DeferLockT {};
		static constexpr DeferLockT DeferLock;
		struct AdoptLockT {};
		static constexpr AdoptLockT AdoptLock;

		LockGuard(TLock &Lock)
			: m_Lock(Lock)
			, m_IsLocked(true)
		{
			m_Lock.Lock();
		}

		LockGuard(TLock &Lock, DeferLockT)
			: m_Lock(Lock)
			, m_IsLocked(false)
		{
		}

		LockGuard(TLock &Lock, AdoptLockT)
			: m_Lock(Lock)
			, m_IsLocked(true)
		{
		}

		~LockGuard()
		{
			if (m_IsLocked)
				m_Lock.Unlock();
		}

		LockGuard(const LockGuard &) = delete;
		LockGuard & operator = (const LockGuard &) = delete;

		void Lock()
		{
			m_Lock.Lock();
			m_IsLocked = true;
		}

		void Unlock()
		{
			m_Lock.Unlock();
			m_IsLocked = false;
		}

		bool TryLock()
		{
			return m_IsLocked = m_Lock.TryLock();
		}

		bool TryLock(const std::chrono::milliseconds &Timeout)
		{
			return m_IsLocked = m_Lock.TryLock(Timeout);
		}

		bool IsLocked() const noexcept
		{
			return m_IsLocked;
		}

	private:
		TLock &m_Lock;
		bool m_IsLocked;
	};

	template<typename TLock> class BlockLock
	{
	public:
		BlockLock(TLock &Lock)
			: m_Lock(Lock)
		{
			m_Lock.Lock();
		}

		~BlockLock()
		{
			m_Lock.Unlock();
		}

		BlockLock(const BlockLock &) = delete;
		BlockLock & operator = (const BlockLock &) = delete;

	private:
		TLock &m_Lock;
	};

	template<typename TLock> class TryBlockLock
	{
	public:
		TryBlockLock(TLock &Lock)
			: m_Lock(Lock)
			, m_IsLocked(false)
		{
		}

		~TryBlockLock()
		{
			if (m_IsLocked)
				m_Lock.Unlock();
		}

		TryBlockLock(const TryBlockLock &) = delete;
		TryBlockLock & operator = (const TryBlockLock &) = delete;

		bool TryLock()
		{
			return m_IsLocked = m_Lock.TryLock();
		}

		bool TryLock(const std::chrono::milliseconds &Timeout)
		{
			return m_IsLocked = m_Lock.TryLock(Timeout);
		}

		bool IsLocked() const noexcept
		{
			return m_IsLocked;
		}

	private:
		TLock &m_Lock;
		bool m_IsLocked;
	};

	template<typename TLock> class SharedBlockLock
	{
	public:
		SharedBlockLock(TLock &Lock)
			: m_Lock(Lock)
		{
			m_Lock.LockShared();
		}

		~SharedBlockLock()
		{
			m_Lock.UnlockShared();
		}

		SharedBlockLock(const SharedBlockLock &) = delete;
		SharedBlockLock & operator = (const SharedBlockLock &) = delete;

	private:
		TLock &m_Lock;
	};

	template<typename TLock> class SharedTryBlockLock
	{
	public:
		SharedTryBlockLock(TLock &Lock)
			: m_Lock(Lock)
			, m_IsLocked(false)
		{
		}

		~SharedTryBlockLock()
		{
			if (m_IsLocked)
				m_Lock.UnlockShared();
		}

		SharedTryBlockLock(const SharedTryBlockLock &) = delete;
		SharedTryBlockLock & operator = (const SharedTryBlockLock &) = delete;

		bool TryLock()
		{
			return m_IsLocked = m_Lock.TryLockShared();
		}

		bool TryLock(const std::chrono::milliseconds &Timeout)
		{
			return m_IsLocked = m_Lock.TryLockShared(Timeout);
		}

		bool IsLocked() const noexcept
		{
			return m_IsLocked;
		}

	private:
		TLock &m_Lock;
		bool m_IsLocked;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_LOCK_H
