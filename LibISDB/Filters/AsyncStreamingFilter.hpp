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
 @file   AsyncStreamingFilter.hpp
 @brief  非同期ストリーミングフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_ASYNC_STREAMING_FILTER_H
#define LIBISDB_ASYNC_STREAMING_FILTER_H


#include "FilterBase.hpp"
#include "SourceFilter.hpp"
#include "../Base/StreamBuffer.hpp"
#include "../Base/StreamingThread.hpp"


namespace LibISDB
{

	/** 非同期ストリーミングフィルタクラス */
	class AsyncStreamingFilter
		: public SingleIOFilter
		, protected StreamingThread
	{
	public:
		AsyncStreamingFilter();
		~AsyncStreamingFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("AsyncStreamingFilter"); }

	// FilterBase
		void Reset() override;
		bool StartStreaming() override;
		bool StopStreaming() override;

	// SingleIOFilter
		bool ReceiveData(DataStream *pData) override;

	// AsyncStreamingFilter
		bool CreateBuffer(
			size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount);
		void DeleteBuffer();
		bool IsBufferCreated() const;
		void ClearBuffer();
		bool SetBuffer(const std::shared_ptr<StreamBuffer> &Buffer);
		std::shared_ptr<StreamBuffer> GetBuffer() const;
		std::shared_ptr<StreamBuffer> DetachBuffer();

		bool SetBufferingEnabled(bool Enabled);
		bool GetBufferingEnabled() const noexcept { return m_BufferingEnabled; }
		void SetClearOnReset(bool Clear);
		bool GetClearOnReset() const noexcept { return m_ClearOnReset; }
		bool SetOutputBufferSize(size_t Size);
		size_t GetOutputBufferSize() const noexcept { return m_OutputBufferSize; }

		bool SetSourceFilter(SourceFilter *pSourceFilter);

		bool WaitForEndOfStream();
		bool WaitForEndOfStream(const std::chrono::milliseconds &Timeout);

	protected:
		class PullSourceThread
			: public StreamingThread
		{
		public:
			PullSourceThread(AsyncStreamingFilter *pFilter);
			~PullSourceThread();

		private:
		// Thread
			const CharType * GetThreadName() const noexcept override { return LIBISDB_STR("PullSource"); }

		// StreamingThread
			bool ProcessStream() override;

			AsyncStreamingFilter *m_pFilter;
		};

	// Thread
		const CharType * GetThreadName() const noexcept override { return LIBISDB_STR("AsyncStreaming"); }

	// StreamingThread
		void StreamingLoop() override;
		bool ProcessStream() override;

		bool m_BufferingEnabled;
		bool m_ClearOnReset;

		std::shared_ptr<StreamBuffer> m_StreamBuffer;
		StreamBuffer::SequentialReader m_StreamReader;
		DataBuffer m_OutputBuffer;
		size_t m_OutputBufferSize;

		SourceFilter *m_pSourceFilter;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ASYNC_STREAMING_FILTER_H
