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
 @file   StreamSourceEngine.cpp
 @brief  ストリームソースエンジン
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamSourceEngine.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


void StreamSourceEngine::WaitForEndOfStream()
{
	BlockLock Lock(m_EndLock);

	m_EndCondition.Wait(m_EndLock, [this]() -> bool { return m_IsSourceEnd.load(std::memory_order_acquire); });
}


bool StreamSourceEngine::WaitForEndOfStream(const std::chrono::milliseconds &Timeout)
{
	BlockLock Lock(m_EndLock);

	return m_EndCondition.WaitFor(m_EndLock, Timeout, [this]() -> bool { return m_IsSourceEnd.load(std::memory_order_acquire); });
}


void StreamSourceEngine::OnSourceClosed(SourceFilter *pSource)
{
	m_IsSourceEnd.store(true, std::memory_order_release);
	m_EndCondition.NotifyOne();
}


void StreamSourceEngine::OnSourceEnd(SourceFilter *pSource)
{
	m_IsSourceEnd.store(true, std::memory_order_release);
	m_EndCondition.NotifyOne();
}


void StreamSourceEngine::OnStreamingStart(SourceFilter *pSource)
{
	m_IsSourceEnd.store(false, std::memory_order_release);
}


}	// namespace LibISDB
