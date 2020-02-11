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

	class LockBase
	{
	public:
		virtual void Lock() = 0;
		virtual void Unlock() = 0;
		virtual bool TryLock() = 0;
		virtual bool TryLock(const std::chrono::milliseconds &Timeout) = 0;
	};

	class SharedLockBase
	{
	public:
		virtual void LockShared() = 0;
		virtual void UnlockShared() = 0;
		virtual bool TryLockShared() = 0;
		virtual bool TryLockShared(const std::chrono::milliseconds &Timeout) = 0;
	};

	class MutexLock
		: public LockBase
	{
	public:
		MutexLock();
		~MutexLock();
		MutexLock(const MutexLock &) = delete;
		MutexLock & operator = (const MutexLock &) = delete;

	// LockBase
		void Lock() override;
		void Unlock() override;
		bool TryLock() override;
		bool TryLock(const std::chrono::milliseconds &Timeout) override;

	// MutexLock
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
		: public LockBase
		, public SharedLockBase
	{
	public:
		SharedLock();
		~SharedLock();
		SharedLock(const SharedLock &) = delete;
		SharedLock & operator = (const SharedLock &) = delete;

	// LockBase
		void Lock() override;
		void Unlock() override;
		bool TryLock() override;
		bool TryLock(const std::chrono::milliseconds &Timeout) override;

	// SharedLockBase
		void LockShared() override;
		void UnlockShared() override;
		bool TryLockShared() override;
		bool TryLockShared(const std::chrono::milliseconds &Timeout) override;

	// SharedLock
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

	class LockGuard
	{
	public:
		struct DeferLockT {};
		static const DeferLockT DeferLock;
		struct AdoptLockT {};
		static const AdoptLockT AdoptLock;

		LockGuard(LockBase &Lock)
			: m_Lock(Lock)
			, m_IsLocked(true)
		{
			m_Lock.Lock();
		}

		LockGuard(LockBase &Lock, DeferLockT)
			: m_Lock(Lock)
			, m_IsLocked(false)
		{
		}

		LockGuard(LockBase &Lock, AdoptLockT)
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
		LockBase &m_Lock;
		bool m_IsLocked;
	};

	class BlockLock
	{
	public:
		BlockLock(LockBase &Lock)
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
		LockBase &m_Lock;
	};

	class TryBlockLock
	{
	public:
		TryBlockLock(LockBase &Lock)
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
		LockBase &m_Lock;
		bool m_IsLocked;
	};

	class SharedBlockLock
	{
	public:
		SharedBlockLock(SharedLockBase &Lock)
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
		SharedLockBase &m_Lock;
	};

	class SharedTryBlockLock
	{
	public:
		SharedTryBlockLock(SharedLockBase &Lock)
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
		SharedLockBase &m_Lock;
		bool m_IsLocked;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_LOCK_H
