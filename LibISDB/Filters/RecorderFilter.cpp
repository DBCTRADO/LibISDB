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
 @file   RecorderFilter.cpp
 @brief  録画フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "RecorderFilter.hpp"
#include "../Utilities/Utilities.hpp"
#include <algorithm>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


RecorderFilter::RecorderFilter()
	: m_TaskEventListener(this)
{
}


RecorderFilter::~RecorderFilter()
{
	DeleteAllTasks();
}


void RecorderFilter::Finalize()
{
	DeleteAllTasks();
}


void RecorderFilter::SetActiveServiceID(uint16_t ServiceID)
{
	BlockLock Lock(m_FilterLock);

	for (auto &Task : m_TaskList)
		Task->OnActiveServiceChanged(ServiceID);
}


bool RecorderFilter::ProcessData(DataStream *pData)
{
	if (pData->Is<TSPacket>()) {
		do {
			TSPacket *pPacket = pData->Get<TSPacket>();
			for (auto &Task : m_TaskList)
				Task->InputPacket(pPacket);
		} while (pData->Next());
	} else {
		do {
			const DataBuffer *pBuffer = pData->GetData();
			for (auto &Task : m_TaskList)
				Task->InputData(pBuffer);
		} while (pData->Next());
	}

	return true;
}


void RecorderFilter::SetLogger(Logger *pLogger)
{
	BlockLock Lock(m_FilterLock);

	ObjectBase::SetLogger(pLogger);

	for (auto &Task : m_TaskList)
		Task->SetLogger(pLogger);
}


std::shared_ptr<RecorderFilter::RecordingTask> RecorderFilter::CreateTask(
	StreamWriter *pWriter, const RecordingOptions *pOptions)
{
	std::shared_ptr<RecordingTaskImpl> Task(new RecordingTaskImpl(pWriter, pOptions));

	Task->AddEventListener(&m_TaskEventListener);
	Task->SetLogger(m_pLogger);

	static const size_t MinCacheSize = 1024;
	size_t CacheSize;
	if (pOptions != nullptr)
		CacheSize = std::max(pOptions->WriteCacheSize, MinCacheSize);
	else
		CacheSize = MinCacheSize;
	if (!Task->AllocateWriteCacheBuffer(CacheSize)) {
		Log(Logger::LogType::Error,
			LIBISDB_STR("Failed to allocate write cache memory. (%zu bytes)"),
			CacheSize);
		if ((CacheSize <= MinCacheSize)
				|| !Task->AllocateWriteCacheBuffer(MinCacheSize)) {
			SetError(std::errc::not_enough_memory);
			return std::shared_ptr<RecordingTask>();
		}
	}

	if (!Task->Start()) {
		SetError(std::errc::resource_unavailable_try_again);
		return std::shared_ptr<RecordingTask>();
	}

	BlockLock Lock(m_FilterLock);

	m_TaskList.emplace_back(Task);

	ResetError();

	return std::shared_ptr<RecordingTask>(std::move(Task));
}


bool RecorderFilter::DeleteTask(std::shared_ptr<RecordingTask> &Task)
{
	auto it = FindTask(Task.get());

	if (it == m_TaskList.end())
		return false;

	(*it)->Stop();

	Task.reset();

	BlockLock Lock(m_FilterLock);

	m_TaskList.erase(it);

	return true;
}


void RecorderFilter::DeleteAllTasks()
{
	BlockLock Lock(m_FilterLock);

	m_TaskList.clear();
}


bool RecorderFilter::IsTaskValid(const std::shared_ptr<RecordingTask> &Task) const
{
	return FindTask(Task.get()) != m_TaskList.end();
}


int RecorderFilter::GetTaskCount() const
{
	return static_cast<int>(m_TaskList.size());
}


std::shared_ptr<RecorderFilter::RecordingTask> RecorderFilter::GetTaskByIndex(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_TaskList.size())
		return std::shared_ptr<RecordingTask>();

	return m_TaskList[Index];
}


RecorderFilter::TaskList::iterator RecorderFilter::FindTask(const RecordingTask *pTask)
{
	return std::find_if(
		m_TaskList.begin(), m_TaskList.end(),
		[pTask](const TaskList::value_type &Task) -> bool { return Task.get() == pTask; });
}


RecorderFilter::TaskList::const_iterator RecorderFilter::FindTask(const RecordingTask *pTask) const
{
	return std::find_if(
		m_TaskList.cbegin(), m_TaskList.cend(),
		[pTask](const TaskList::value_type &Task) -> bool { return Task.get() == pTask; });
}


bool RecorderFilter::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool RecorderFilter::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}




RecorderFilter::RecordingDataStreamer::RecordingDataStreamer(StreamWriter *pWriter)
	: m_Writer(pWriter)
{
}


bool RecorderFilter::RecordingDataStreamer::SetWriter(StreamWriter *pWriter)
{
	BlockLock Lock(m_Lock);

	m_Writer.reset(pWriter);

	return true;
}


bool RecorderFilter::RecordingDataStreamer::ReopenWriter(
	const CStringView &FileName, StreamWriter::OpenFlag Flags)
{
	BlockLock Lock(m_Lock);

	if (!m_Writer) {
		SetError(std::errc::no_stream_resources);
		return false;
	}

	if (!m_Writer->Reopen(FileName, Flags)) {
		SetError(m_Writer->GetLastErrorDescription());
		if (!m_Writer->IsOpen()) {
			m_Writer.reset();
		}
		return false;
	}

	m_OutputErrorNotified = false;

	ResetError();

	return true;
}


void RecorderFilter::RecordingDataStreamer::CloseWriter()
{
	BlockLock Lock(m_Lock);

	if (m_Writer) {
		m_Writer->SetPreallocationUnit(0);
		FlushBuffer(std::chrono::seconds(10));
		m_Writer->Close();
		m_Writer.reset();
	}
}


bool RecorderFilter::RecordingDataStreamer::GetFileName(String *pFileName) const
{
	if (LIBISDB_TRACE_ERROR_IF(pFileName == nullptr))
		return false;

	pFileName->clear();

	if (!m_Writer)
		return false;

	return m_Writer->GetFileName(pFileName);
}


bool RecorderFilter::RecordingDataStreamer::GetRecordingStatistics(RecordingStatistics *pStatistics) const
{
	if (LIBISDB_TRACE_ERROR_IF(pStatistics == nullptr))
		return false;

	Statistics Stats;

	GetStatistics(&Stats);

	pStatistics->InputBytes = Stats.InputBytes;
	pStatistics->OutputBytes = Stats.OutputBytes;
	pStatistics->OutputCount = Stats.OutputCount;
	if (m_Writer && m_Writer->IsWriteSizeAvailable())
		pStatistics->WriteBytes = m_Writer->GetWriteSize();
	else
		pStatistics->WriteBytes = RecordingStatistics::INVALID_SIZE;
	pStatistics->WriteErrorCount = Stats.OutputErrorCount;

	return true;
}


size_t RecorderFilter::RecordingDataStreamer::OutputData(const uint8_t *pData, size_t DataSize)
{
	if (!m_Writer)
		return 0;

	return m_Writer->Write(pData, DataSize);
}


bool RecorderFilter::RecordingDataStreamer::IsOutputValid() const
{
	return static_cast<bool>(m_Writer);
}




RecorderFilter::RecordingTaskImpl::RecordingTaskImpl(
	StreamWriter *pWriter, const RecordingOptions *pOptions)
	: m_Paused(false)

	, m_DataStreamer(pWriter)
	, m_StreamerEventListener(this)
{
	if (pOptions != nullptr) {
		m_Options = *pOptions;
		m_StreamSelector.SetTarget(m_Options.ServiceID, m_Options.StreamFlags);
	}

	m_DataStreamer.AddEventListener(&m_StreamerEventListener);
}


RecorderFilter::RecordingTaskImpl::~RecordingTaskImpl()
{
	Stop();
}


bool RecorderFilter::RecordingTaskImpl::SetWriter(StreamWriter *pWriter)
{
	m_DataStreamer.SetWriter(pWriter);

	if (!m_DataStreamer.IsStarted()
			&& m_DataStreamer.HasInputBuffer() && m_DataStreamer.IsOutputValid()) {
		if (!m_DataStreamer.Start())
			return false;
	}

	return true;
}


bool RecorderFilter::RecordingTaskImpl::Reopen(
	const CStringView &FileName, StreamWriter::OpenFlag Flags)
{
	LIBISDB_TRACE(LIBISDB_STR("RecorderFilter::RecordingTaskImpl::Reopen() : %p\n"), this);

	return m_DataStreamer.ReopenWriter(FileName, Flags);
}


bool RecorderFilter::RecordingTaskImpl::Start()
{
	LIBISDB_TRACE(LIBISDB_STR("RecorderFilter::RecordingTaskImpl::Start() : %p\n"), this);

	BlockLock Lock(m_Lock);

	if (!SetPendingBufferSize(m_Options.MaxPendingSize))
		return false;

	if (m_DataStreamer.HasInputBuffer() && m_DataStreamer.IsOutputValid()) {
		if (!m_DataStreamer.Start()) {
			m_DataStreamer.FreeInputBuffer();
			return false;
		}
	}

	return true;
}


void RecorderFilter::RecordingTaskImpl::Stop()
{
	LIBISDB_TRACE(LIBISDB_STR("RecorderFilter::RecordingTaskImpl::Stop() : %p\n"), this);

	BlockLock Lock(m_Lock);

	m_DataStreamer.Stop();
	m_DataStreamer.CloseWriter();
	m_DataStreamer.Close();
}


bool RecorderFilter::RecordingTaskImpl::Pause()
{
	LIBISDB_TRACE(LIBISDB_STR("RecorderFilter::RecordingTaskImpl::Pause() : %p\n"), this);

	BlockLock Lock(m_Lock);

	m_Paused.store(true, std::memory_order_release);

	m_DataStreamer.Pause();

	return true;
}


bool RecorderFilter::RecordingTaskImpl::Resume()
{
	LIBISDB_TRACE(LIBISDB_STR("RecorderFilter::RecordingTaskImpl::Resume() : %p\n"), this);

	BlockLock Lock(m_Lock);

	m_Paused.store(false, std::memory_order_release);

	m_DataStreamer.Resume();

	return true;
}


void RecorderFilter::RecordingTaskImpl::ClearBuffer()
{
	LIBISDB_TRACE(LIBISDB_STR("RecorderFilter::RecordingTaskImpl::ClearBuffer() : %p\n"), this);

	m_DataStreamer.ClearBuffer();
}


bool RecorderFilter::RecordingTaskImpl::SetOptions(const RecordingOptions &Options)
{
	BlockLock Lock(m_Lock);

	if ((Options.ServiceID != m_Options.ServiceID) || (Options.StreamFlags != m_Options.StreamFlags)) {
		m_Options.ServiceID = Options.ServiceID;
		m_Options.StreamFlags = Options.StreamFlags;
		m_StreamSelector.SetTarget(m_Options.ServiceID, m_Options.StreamFlags);
	}

	m_Options.FollowActiveService = Options.FollowActiveService;

	if (Options.MaxPendingSize != m_Options.MaxPendingSize) {
		if (!SetPendingBufferSize(Options.MaxPendingSize))
			return false;
		m_Options.MaxPendingSize = Options.MaxPendingSize;
	}

	m_Options.ClearPendingBufferOnServiceChanged = Options.ClearPendingBufferOnServiceChanged;

	return true;
}


bool RecorderFilter::RecordingTaskImpl::GetFileName(String *pFileName) const
{
	return m_DataStreamer.GetFileName(pFileName);
}


bool RecorderFilter::RecordingTaskImpl::GetStatistics(RecordingStatistics *pStatistics) const
{
	return m_DataStreamer.GetRecordingStatistics(pStatistics);
}


void RecorderFilter::RecordingTaskImpl::InputPacket(TSPacket *pPacket)
{
	BlockLock Lock(m_Lock);

	if (!m_Paused.load(std::memory_order_acquire)) {
		TSPacket *pDstPacket = m_StreamSelector.InputPacket(pPacket);

		if (pDstPacket != nullptr)
			m_DataStreamer.InputData(pDstPacket->GetData(), pDstPacket->GetSize());
	}
}


void RecorderFilter::RecordingTaskImpl::InputData(const DataBuffer *pData)
{
	BlockLock Lock(m_Lock);

	if (!m_Paused.load(std::memory_order_acquire)) {
		m_DataStreamer.InputData(pData->GetData(), pData->GetSize());
	}
}


void RecorderFilter::RecordingTaskImpl::OnActiveServiceChanged(uint16_t ServiceID)
{
	BlockLock Lock(m_Lock);

	if (m_Options.FollowActiveService) {
		m_Options.ServiceID = ServiceID;
		m_StreamSelector.SetTarget(ServiceID, m_Options.StreamFlags);
	}

	if (m_Options.ClearPendingBufferOnServiceChanged) {
		if (!m_DataStreamer.IsOutputValid())
			m_DataStreamer.ClearBuffer();
	}
}


bool RecorderFilter::RecordingTaskImpl::AllocateWriteCacheBuffer(size_t Size)
{
	return m_DataStreamer.AllocateOutputCacheBuffer(Size);
}


bool RecorderFilter::RecordingTaskImpl::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool RecorderFilter::RecordingTaskImpl::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


bool RecorderFilter::RecordingTaskImpl::SetPendingBufferSize(size_t Size)
{
	if (Size == 0) {
		m_DataStreamer.FreeInputBuffer();
	} else {
		const size_t MaxBlockCount =
			std::max((Size + m_BufferBlockSize - 1) / m_BufferBlockSize, 2_z);
		std::shared_ptr<StreamBuffer> Buffer(m_DataStreamer.GetInputBuffer());

		if (Buffer) {
			if (!Buffer->SetSize(m_BufferBlockSize, 1, MaxBlockCount, false))
				return false;
		} else {
			if (!m_DataStreamer.CreateInputBuffer(m_BufferBlockSize, 1, MaxBlockCount))
				return false;
		}
	}

	return true;
}




RecorderFilter::RecordingTaskImpl::StreamerEventListener::StreamerEventListener(RecordingTaskImpl *pTask)
	: m_pTask(pTask)
{
}


void RecorderFilter::RecordingTaskImpl::StreamerEventListener::OnOutputError(DataStreamer *pDataStreamer)
{
	m_pTask->m_EventListenerList.CallEventListener(
		&RecordingTaskImpl::EventListener::OnWriteError, m_pTask);
}




RecorderFilter::TaskEventListener::TaskEventListener(RecorderFilter *pRecorder)
	: m_pRecorder(pRecorder)
{
}


void RecorderFilter::TaskEventListener::OnWriteError(RecordingTaskImpl *pTask)
{
	m_pRecorder->m_EventListenerList.CallEventListener(
		&RecorderFilter::EventListener::OnWriteError, m_pRecorder, pTask);
}


}	// namespace LibISDB
