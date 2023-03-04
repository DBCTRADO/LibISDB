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
 @file   DataStreamer.hpp
 @brief  データストリーマ
 @author DBCTRADO
*/


#ifndef LIBISDB_DATA_STREAMER_H
#define LIBISDB_DATA_STREAMER_H


#include "ObjectBase.hpp"
#include "StreamBuffer.hpp"
#include "EventListener.hpp"
#include "StreamingThread.hpp"


namespace LibISDB
{

	/** データストリーマクラス */
	class DataStreamer
		: public ObjectBase
		, protected StreamingThread
	{
	public:
		/** イベントリスナ */
		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnOutputError(DataStreamer *pDataStreamer) {}
		};

		/** 統計情報 */
		struct Statistics {
			unsigned long long InputBytes = 0;
			unsigned long long OutputBytes = 0;
			unsigned long long OutputCount = 0;
			unsigned long OutputErrorCount = 0;

			void Reset() noexcept { *this = Statistics(); }
		};

		DataStreamer() noexcept;
		~DataStreamer();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("DataStreamer"); }

	// DataStreamer
		bool Initialize();
		void Close();

		bool Start();
		bool Stop(const std::chrono::milliseconds &Timeout = std::chrono::milliseconds(0));
		using Thread::IsStarted;
		bool Pause();
		bool Resume();

		bool InputData(const uint8_t *pData, size_t DataSize);
		bool InputData(const DataBuffer *pData);
		bool FlushBuffer(const std::chrono::milliseconds &Timeout = std::chrono::milliseconds(0));
		void ClearBuffer();

		bool CreateInputBuffer(size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount);
		bool FreeInputBuffer();
		bool SetInputBuffer(const std::shared_ptr<StreamBuffer> &Buffer);
		std::shared_ptr<StreamBuffer> GetInputBuffer() const;
		std::shared_ptr<StreamBuffer> DetachInputBuffer();
		bool HasInputBuffer() const noexcept;
		bool SetInputStartPos(StreamBuffer::PosType Pos);

		bool AllocateOutputCacheBuffer(size_t Size);

		bool GetStatistics(Statistics *pStats) const;

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

	protected:
		virtual size_t OutputData(const uint8_t *pData, size_t DataSize) = 0;
		virtual bool IsOutputValid() const = 0;
		virtual void ClearOutput() {}

		bool FillOutputCache();
		bool OutputCachedData();
		bool OutputDataWithCache(const uint8_t *pData, size_t DataSize);

	// Thread
		const CharType * GetThreadName() const noexcept override { return LIBISDB_STR("DataStreamer"); }

	// StreamingThread
		bool ProcessStream() override;

		std::shared_ptr<StreamBuffer> m_InputBuffer;
		StreamBuffer::SequentialReader m_StreamReader;
		StreamBuffer::PosType m_InputStartPos;
		DataBuffer m_OutputCacheBuffer;
		mutable MutexLock m_Lock;

		Statistics m_Statistics;
		bool m_OutputErrorNotified;

		EventListenerList<EventListener> m_EventListenerList;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DATA_STREAMER_H
