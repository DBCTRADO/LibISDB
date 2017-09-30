/*
  LibISDB
  Copyright(c) 2017 DBCTRADO

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
 @file   Thread.hpp
 @brief  スレッド
 @author DBCTRADO
*/


#ifndef LIBISDB_THREAD_H
#define LIBISDB_THREAD_H


#include <chrono>
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#else
#include <future>
#endif


namespace LibISDB
{

	/** スレッドクラス */
	class Thread
	{
	public:
		Thread();
		virtual ~Thread();

		Thread(const Thread &) = delete;
		Thread & operator = (const Thread &) = delete;

		bool Start();
		void Stop();
		bool IsStarted() const;
		bool Wait() const;
		bool Wait(const std::chrono::milliseconds &Timeout) const;
		void Terminate();

	protected:
		virtual const CharType * GetThreadName() const noexcept = 0;
		virtual void ThreadMain() = 0;

		void SetThreadName(const CharType *pName);

#ifdef LIBISDB_WINDOWS
		HANDLE m_hThread = nullptr;
		unsigned int m_ThreadID = 0;
#else
		std::future<void> m_Future;
#endif
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_THREAD_H
