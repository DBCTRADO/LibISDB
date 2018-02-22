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
 @file   StreamSourceEngine.hpp
 @brief  ストリームソースエンジン
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_SOURCE_ENGINE_H
#define LIBISDB_STREAM_SOURCE_ENGINE_H


#include "TSEngine.hpp"
#include "../Utilities/ConditionVariable.hpp"
#include <atomic>


namespace LibISDB
{

	/**< ストリームソースエンジンクラス */
	class StreamSourceEngine
		: public TSEngine
	{
	public:
		void WaitForEndOfStream();
		bool WaitForEndOfStream(const std::chrono::milliseconds &Timeout);

	private:
		void OnSourceClosed(SourceFilter *pSource) override;
		void OnSourceEnd(SourceFilter *pSource) override;
		void OnStreamingStart(SourceFilter *pSource) override;

		ConditionVariable m_EndCondition;
		MutexLock m_EndLock;
		std::atomic<bool> m_IsSourceEnd {false};
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_SOURCE_ENGINE_H
