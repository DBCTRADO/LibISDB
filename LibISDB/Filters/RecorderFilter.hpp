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
 @file   RecorderFilter.hpp
 @brief  録画フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_RECORDER_FILTER_H
#define LIBISDB_RECORDER_FILTER_H


#include "FilterBase.hpp"
#include "../Base/EventListener.hpp"
#include "../Base/FileStream.hpp"
#include "../Base/StreamBufferDataStreamer.hpp"
#include "../Base/StreamWriter.hpp"
#include "../TS/StreamSelector.hpp"
#include "../Utilities/Clock.hpp"
#include <vector>
#include <deque>
#include <memory>
#include <atomic>


namespace LibISDB
{

	/** 録画フィルタクラス */
	class RecorderFilter
		: public SingleIOFilter
	{
	public:
		/** 録画設定 */
		struct RecordingOptions {
			uint16_t ServiceID = SERVICE_ID_INVALID;
			bool FollowActiveService = false;
			StreamSelector::StreamFlag StreamFlags = StreamSelector::StreamFlag::All;
			size_t WriteCacheSize = 0;
			size_t MaxPendingSize = 0;
			bool ClearPendingBufferOnServiceChanged = true;
		};

		/** 録画統計情報 */
		struct RecordingStatistics {
			static constexpr unsigned long long INVALID_SIZE = std::numeric_limits<unsigned long long>::max();

			unsigned long long InputBytes = 0;
			unsigned long long OutputBytes = 0;
			unsigned long long OutputCount = 0;
			unsigned long long WriteBytes = INVALID_SIZE;
			unsigned long WriteErrorCount = 0;
		};

		/** 録画タスク */
		class RecordingTask
			: public ObjectBase
		{
		public:
		// ObjectBase
			const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("RecordingTask"); }

		// RecordingTask
			virtual bool SetWriter(StreamWriter *pWriter) = 0;
			virtual bool Reopen(
				const CStringView &FileName,
				StreamWriter::OpenFlag Flags = StreamWriter::OpenFlag::None) = 0;

			virtual bool Start() = 0;
			virtual void Stop() = 0;
			virtual bool Pause() = 0;
			virtual bool Resume() = 0;
			virtual bool IsPaused() const = 0;

			virtual void ClearBuffer() = 0;

			virtual bool SetOptions(const RecordingOptions &Options) = 0;
			virtual const RecordingOptions & GetOptions() const = 0;

			virtual bool GetFileName(String *pFileName) const = 0;
			virtual bool GetStatistics(RecordingStatistics *pStatistics) const = 0;
		};

		/** イベントリスナ */
		class EventListener
			: public LibISDB::EventListener
		{
		public:
			virtual void OnWriteError(RecorderFilter *pRecorder, RecordingTask *pTask) {}
		};

		RecorderFilter();
		~RecorderFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("RecorderFilter"); }

	// FilterBase
		void Finalize() override;
		void SetActiveServiceID(uint16_t ServiceID) override;

	// SingleIOFilter
		bool ProcessData(DataStream *pData) override;

	// ObjectBase
		void SetLogger(Logger *pLogger) override;

	// RecorderFilter
		std::shared_ptr<RecordingTask> CreateTask(StreamWriter *pWriter, const RecordingOptions *pOptions = nullptr);
		bool DeleteTask(std::shared_ptr<RecordingTask> &Task);
		void DeleteAllTasks();
		bool IsTaskValid(const std::shared_ptr<RecordingTask> &Task) const;

		int GetTaskCount() const;
		std::shared_ptr<RecordingTask> GetTaskByIndex(int Index) const;

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);

	protected:
		class RecordingDataStreamer
			: public StreamBufferDataStreamer
		{
		public:
			RecordingDataStreamer(StreamWriter *pWriter);

			bool SetWriter(StreamWriter *pWriter);
			bool ReopenWriter(const CStringView &FileName, StreamWriter::OpenFlag Flags);
			void CloseWriter();
			bool GetFileName(String *pFileName) const;
			bool GetRecordingStatistics(RecordingStatistics *pStatistics) const;

			bool IsOutputValid() const override;

		private:
			size_t OutputData(const uint8_t *pData, size_t DataSize) override;

			std::unique_ptr<StreamWriter> m_Writer;
		};

		class RecordingTaskImpl
			: public RecordingTask
		{
		public:
			class EventListener
				: public LibISDB::EventListener
			{
			public:
				virtual void OnWriteError(RecordingTaskImpl *pTask) {}
			};

			RecordingTaskImpl(StreamWriter *pWriter, const RecordingOptions *pOptions);
			~RecordingTaskImpl();

		// RecordingTask
			bool SetWriter(StreamWriter *pWriter) override;
			bool Reopen(const CStringView &FileName, StreamWriter::OpenFlag Flags) override;

			bool Start() override;
			void Stop() override;
			bool Pause() override;
			bool Resume() override;
			bool IsPaused() const override { return m_Paused.load(std::memory_order_acquire); }

			void ClearBuffer() override;

			bool SetOptions(const RecordingOptions &Options) override;
			const RecordingOptions & GetOptions() const override { return m_Options; }

			bool GetFileName(String *pFileName) const override;
			bool GetStatistics(RecordingStatistics *pStatistics) const override;

		// RecordingTaskImpl
			void InputPacket(TSPacket *pPacket);
			void InputData(const DataBuffer *pData);
			void OnActiveServiceChanged(uint16_t ServiceID);

			bool AllocateWriteCacheBuffer(size_t Size);

			bool AddEventListener(EventListener *pEventListener);
			bool RemoveEventListener(EventListener *pEventListener);

		private:
			class StreamerEventListener
				: public DataStreamer::EventListener
			{
			public:
				StreamerEventListener(RecordingTaskImpl *pTask);

				void OnOutputError(DataStreamer *pDataStreamer) override;

			private:
				RecordingTaskImpl *m_pTask;
			};

			bool SetPendingBufferSize(size_t Size);

			RecordingOptions m_Options;
			std::atomic<bool> m_Paused;

			StreamSelector m_StreamSelector;
			RecordingDataStreamer m_DataStreamer;
			StreamerEventListener m_StreamerEventListener;

			mutable MutexLock m_Lock;

			EventListenerList<EventListener> m_EventListenerList;

			static const size_t m_BufferBlockSize = 1024 * 1024;
		};

		typedef std::vector<std::shared_ptr<RecordingTaskImpl>> TaskList;

		TaskList::iterator FindTask(const RecordingTask *pTask);
		TaskList::const_iterator FindTask(const RecordingTask *pTask) const;

		class TaskEventListener
			: public RecordingTaskImpl::EventListener
		{
		public:
			TaskEventListener(RecorderFilter *pRecorder);

		private:
			void OnWriteError(RecordingTaskImpl *pTask) override;

			RecorderFilter *m_pRecorder;
		};

		TaskList m_TaskList;

		EventListenerList<EventListener> m_EventListenerList;
		TaskEventListener m_TaskEventListener;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_RECORDER_FILTER_H
