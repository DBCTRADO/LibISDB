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
 @file   StreamSourceFilter.hpp
 @brief  ストリームソースフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_SOURCE_FILTER_H
#define LIBISDB_STREAM_SOURCE_FILTER_H


#include "SourceFilter.hpp"
#include "../Utilities/Thread.hpp"
#include "../Utilities/ConditionVariable.hpp"
#include "../Base/Stream.hpp"
#include <deque>
#include <memory>
#include <atomic>


namespace LibISDB
{

	/** ストリームソースフィルタクラス */
	class StreamSourceFilter
		: public SourceFilter
		, protected Thread
	{
	public:
		StreamSourceFilter();
		~StreamSourceFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("StreamSourceFilter"); }

	// FilterBase
		void Reset() override;
		void ResetGraph() override;
		bool StartStreaming() override;
		bool StopStreaming() override;

	// SourceFilter
		bool OpenSource(const CStringView &Name) override;
		bool CloseSource() override;
		bool IsSourceOpen() const override;
		bool FetchSource(size_t RequestSize) override;
		SourceMode GetAvailableSourceModes() const noexcept override { return SourceMode::Push | SourceMode::Pull; }
		bool SetSourceMode(SourceMode Mode) override;

	// StreamSourceFilter
		bool OpenSource(Stream *pStream);
		bool SetOutputBufferSize(size_t Size);
		size_t GetOutputBufferSize() const noexcept { return m_OutputBufferSize; }
		unsigned long long GetInputBytes() const noexcept { return m_InputBytes; }

	protected:
		enum class RequestType {
			End,
			Reset,
			Start,
			Stop,
		};

		struct StreamingRequest {
			RequestType Type;
			bool IsProcessing;
		};

	// Thread
		const CharType * GetThreadName() const noexcept override { return LIBISDB_STR("StreamSource"); }
		void ThreadMain() override;

		void StreamingMain();
		void AddRequest(RequestType Type);
		bool WaitAllRequests(const std::chrono::milliseconds &Timeout);
		bool HasPendingRequest();

		std::unique_ptr<Stream> m_Stream;
		DataBuffer m_OutputBuffer;
		size_t m_OutputBufferSize;

		std::deque<StreamingRequest> m_RequestQueue;
		MutexLock m_RequestLock;
		ConditionVariable m_RequestQueued;
		ConditionVariable m_RequestProcessed;
		std::chrono::milliseconds m_RequestTimeout;

		std::atomic<unsigned long long> m_InputBytes;

		volatile bool m_IsStreaming;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_SOURCE_FILTER_H
