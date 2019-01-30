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
 @file   AsyncStreamingFilter.cpp
 @brief  非同期ストリーミングフィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "AsyncStreamingFilter.hpp"
#include <thread>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


AsyncStreamingFilter::AsyncStreamingFilter()
	: m_BufferingEnabled(true)
	, m_ClearOnReset(true)
	, m_OutputBufferSize(256 * TS_PACKET_SIZE)
	, m_pSourceFilter(nullptr)
{
}


AsyncStreamingFilter::~AsyncStreamingFilter()
{
	StopStreamingThread();
}


void AsyncStreamingFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	if (m_ClearOnReset && m_StreamBuffer) {
		m_StreamBuffer->Clear();
	}
}


bool AsyncStreamingFilter::StartStreaming()
{
	FilterBase::StartStreaming();

	BlockLock Lock(m_FilterLock);

	if (m_StreamBuffer)
		m_StreamReader.Open(m_StreamBuffer);

	if (!IsStarted()) {
		if (m_OutputBuffer.AllocateBuffer(m_OutputBufferSize) < m_OutputBufferSize)
			return false;

		if (!StartStreamingThread())
			return false;
	}

	return true;
}


bool AsyncStreamingFilter::StopStreaming()
{
	BlockLock Lock(m_FilterLock);

	StopStreamingThread();

	m_StreamReader.Close();
	m_OutputBuffer.FreeBuffer();

	return FilterBase::StopStreaming();
}


bool AsyncStreamingFilter::ReceiveData(DataStream *pData)
{
	BlockLock Lock(m_FilterLock);

	if (m_BufferingEnabled && m_StreamBuffer) {
		do {
			m_StreamBuffer->PushBack(pData->GetData());
		} while (pData->Next());
	}

	return true;
}


bool AsyncStreamingFilter::CreateBuffer(
	size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount)
{
	if (LIBISDB_TRACE_ERROR_IF((BlockSize == 0) || (MaxBlockCount == 0) || (MinBlockCount > MaxBlockCount)))
		return false;

	std::shared_ptr<StreamBuffer> Buffer = std::make_shared<StreamBuffer>();

	if (!Buffer->Create(BlockSize, MinBlockCount, MaxBlockCount))
		return false;

	return SetBuffer(Buffer);
}


void AsyncStreamingFilter::DeleteBuffer()
{
	BlockLock Lock(m_FilterLock);

	m_StreamReader.Close();
	m_StreamBuffer.reset();
}


bool AsyncStreamingFilter::IsBufferCreated() const
{
	return static_cast<bool>(m_StreamBuffer);
}


void AsyncStreamingFilter::ClearBuffer()
{
	BlockLock Lock(m_FilterLock);

	if (m_StreamBuffer)
		m_StreamBuffer->Clear();
}


bool AsyncStreamingFilter::SetBuffer(const std::shared_ptr<StreamBuffer> &Buffer)
{
	if (!Buffer)
		return false;

	BlockLock Lock(m_FilterLock);

	if (m_StreamBuffer != Buffer) {
		const bool IsReaderOpen = m_StreamReader.IsOpen();
		if (IsReaderOpen)
			m_StreamReader.Close();

		m_StreamBuffer = Buffer;

		if (IsReaderOpen)
			m_StreamReader.Open(m_StreamBuffer);
	}

	return true;
}


std::shared_ptr<StreamBuffer> AsyncStreamingFilter::GetBuffer() const
{
	return m_StreamBuffer;
}


std::shared_ptr<StreamBuffer> AsyncStreamingFilter::DetachBuffer()
{
	BlockLock Lock(m_FilterLock);

	m_StreamReader.Close();

	return std::move(m_StreamBuffer);
}


bool AsyncStreamingFilter::SetBufferingEnabled(bool Enabled)
{
	BlockLock Lock(m_FilterLock);

	m_BufferingEnabled = Enabled;

	return true;
}


void AsyncStreamingFilter::SetClearOnReset(bool Clear)
{
	BlockLock Lock(m_FilterLock);

	m_ClearOnReset = Clear;
}


bool AsyncStreamingFilter::SetOutputBufferSize(size_t Size)
{
	if (Size < TS_PACKET_SIZE)
		return false;

	BlockLock Lock(m_FilterLock);

	m_OutputBufferSize = Size;

	return true;
}


bool AsyncStreamingFilter::SetSourceFilter(SourceFilter *pSourceFilter)
{
	if (IsStarted())
		return false;

	m_pSourceFilter = pSourceFilter;

	return true;
}


bool AsyncStreamingFilter::WaitForEndOfStream()
{
	if (!IsStarted())
		return true;

	while (m_StreamReader.IsDataAvailable())
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

	return true;
}


bool AsyncStreamingFilter::WaitForEndOfStream(const std::chrono::milliseconds &Timeout)
{
	if (!IsStarted())
		return true;

	std::chrono::milliseconds SleepTime(0);

	while (m_StreamReader.IsDataAvailable()) {
		if (SleepTime >= Timeout)
			return false;

		const std::chrono::milliseconds Wait = std::min(std::chrono::milliseconds(100), Timeout - SleepTime);

		std::this_thread::sleep_for(Wait);
		SleepTime += Wait;
	}

	return true;
}


void AsyncStreamingFilter::StreamingLoop()
{
	PullSourceThread PullThread(this);

	if ((m_pSourceFilter != nullptr) && !!(m_pSourceFilter->GetSourceMode() & SourceFilter::SourceMode::Pull))
		PullThread.StartStreamingThread();

	StreamingThread::StreamingLoop();
}


bool AsyncStreamingFilter::ProcessStream()
{
	if (m_StreamReader.IsDataAvailable()) {
		const size_t ReadSize = m_StreamReader.Read(
			m_OutputBuffer.GetBuffer(), m_OutputBuffer.GetBufferSize());

		if (ReadSize > 0) {
			m_OutputBuffer.SetSize(ReadSize);
			OutputData(&m_OutputBuffer);
			return true;
		}
	}

	return false;
}




AsyncStreamingFilter::PullSourceThread::PullSourceThread(AsyncStreamingFilter *pFilter)
	: m_pFilter(pFilter)
{
}


AsyncStreamingFilter::PullSourceThread::~PullSourceThread()
{
	StopStreamingThread();
}


bool AsyncStreamingFilter::PullSourceThread::ProcessStream()
{
	const size_t FreeSpace = m_pFilter->m_StreamBuffer->GetFreeSpace();
	if (FreeSpace == 0)
		return false;

	m_pFilter->m_pSourceFilter->FetchSource(FreeSpace);

	return true;
}


}	// namespace LibISDB
