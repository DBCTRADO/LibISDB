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
 @file   ConditionVariable.hpp
 @brief  条件変数
 @author DBCTRADO
*/


#ifndef LIBISDB_CONDITION_VARIABLE_H
#define LIBISDB_CONDITION_VARIABLE_H


#include "Lock.hpp"
#ifndef LIBISDB_WINDOWS
#include <condition_variable>
#endif


namespace LibISDB
{

	class ConditionVariable
	{
	public:
		ConditionVariable();
		ConditionVariable(const ConditionVariable &) = delete;
		ConditionVariable & operator = (const ConditionVariable &) = delete;

		void NotifyOne();
		void NotifyAll();

		void Wait(MutexLock &Lock);
		template<typename TPred> void Wait(MutexLock &Lock, TPred Pred)
		{
			while (!Pred())
				Wait(Lock);
		}
		bool WaitFor(MutexLock &Lock, const std::chrono::milliseconds &Timeout);
		template<typename TPred> bool WaitFor(MutexLock &Lock, const std::chrono::milliseconds &Timeout, TPred Pred)
		{
			while (!Pred()) {
				if (!WaitFor(Lock, Timeout))
					return Pred();
			}
			return true;
		}

	private:
#ifdef LIBISDB_WINDOWS
		::CONDITION_VARIABLE m_ConditionVariable;
#else
		std::condition_variable_any m_ConditionVariable;
#endif
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_CONDITION_VARIABLE_H
