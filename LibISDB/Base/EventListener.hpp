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
 @file   EventListener.hpp
 @brief  イベントリスナ
 @author DBCTRADO
*/


#ifndef LIBISDB_EVENT_LISTENER_H
#define LIBISDB_EVENT_LISTENER_H


#include "../Utilities/Lock.hpp"
#include <vector>
#include <utility>


namespace LibISDB
{

	/** イベントリスナ基底クラス */
	class EventListener
	{
	public:
		EventListener() = default;
		virtual ~EventListener() = default;
	};

	/** イベントリスナのリスト */
	template<typename T> class EventListenerList
	{
	public:
		bool AddEventListener(T *pListener)
		{
			if (pListener == nullptr)
				return false;

			BlockLock Lock(m_Lock);

			if (std::find(m_EventListenerList.begin(), m_EventListenerList.end(), pListener) != m_EventListenerList.end())
				return false;

			m_EventListenerList.push_back(pListener);

			return true;
		}

		bool RemoveEventListener(T *pListener)
		{
			BlockLock Lock(m_Lock);

			auto it = std::find(m_EventListenerList.begin(), m_EventListenerList.end(), pListener);

			if (it == m_EventListenerList.end())
				return false;

			m_EventListenerList.erase(it);

			return true;
		}

		void RemoveAllEventListeners()
		{
			BlockLock Lock(m_Lock);

			m_EventListenerList.clear();
		}

		size_t GetEventListenerCount() const
		{
			BlockLock Lock(m_Lock);

			return m_EventListenerList.size();
		}

		template<typename TMember, typename... TArgs> void CallEventListener(TMember Member, TArgs... Args) const
		{
			BlockLock Lock(m_Lock);

			for (T *p : m_EventListenerList)
				(p->*Member)(Args...);
		}

	protected:
		std::vector<T *> m_EventListenerList;
		mutable MutexLock m_Lock;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_EVENT_LISTENER_H
