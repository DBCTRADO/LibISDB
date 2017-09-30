/*
  LibISDB
  Copyright(c) 2017 DBCTRADO

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
 @file   StreamBufferFilter.cpp
 @brief  ストリームバッファフィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamBufferFilter.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


StreamBufferFilter::StreamBufferFilter()
	: m_BufferingEnabled(false)
	, m_ClearOnReset(true)
{
}


void StreamBufferFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	if (m_ClearOnReset) {
		m_DataStreamer.ClearBuffer();
	}
}


bool StreamBufferFilter::ProcessData(DataStream *pData)
{
	if (m_BufferingEnabled) {
		do {
			m_DataStreamer.InputData(pData->GetData());
		} while (pData->Next());
	}

	return true;
}


bool StreamBufferFilter::CreateMemoryBuffer(
	size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount)
{
	if (LIBISDB_TRACE_ERROR_IF((BlockSize == 0) || (MaxBlockCount == 0) || (MinBlockCount > MaxBlockCount)))
		return false;

	std::shared_ptr<StreamBuffer> Buffer = std::make_shared<StreamBuffer>();

	if (!Buffer->Create(BlockSize, MinBlockCount, MaxBlockCount))
		return false;

	BlockLock Lock(m_FilterLock);

	if (!m_DataStreamer.SetOutputBuffer(Buffer))
		return false;

	return true;
}


void StreamBufferFilter::DeleteBuffer()
{
	BlockLock Lock(m_FilterLock);

	m_DataStreamer.FreeOutputBuffer();
}


bool StreamBufferFilter::IsBufferCreated() const
{
	return m_DataStreamer.HasOutputBuffer();
}


void StreamBufferFilter::ClearBuffer()
{
	m_DataStreamer.ClearOutputBuffer();
}


bool StreamBufferFilter::SetBuffer(const std::shared_ptr<StreamBuffer> &Buffer)
{
	return m_DataStreamer.SetOutputBuffer(Buffer);
}


std::shared_ptr<StreamBuffer> StreamBufferFilter::GetBuffer() const
{
	return m_DataStreamer.GetOutputBuffer();
}


std::shared_ptr<StreamBuffer> StreamBufferFilter::DetachBuffer()
{
	BlockLock Lock(m_FilterLock);

	return m_DataStreamer.DetachOutputBuffer();
}


bool StreamBufferFilter::SetPendingBufferSize(size_t BlockSize, size_t MaxBlockCount)
{
	BlockLock Lock(m_FilterLock);

	if (m_DataStreamer.HasInputBuffer()) {
		if (!m_DataStreamer.GetInputBuffer()->SetSize(BlockSize, 0, MaxBlockCount, true))
			return false;
	} else {
		if (!m_DataStreamer.CreateInputBuffer(BlockSize, 0, MaxBlockCount))
			return false;
	}

	return true;
}


bool StreamBufferFilter::SetBufferingEnabled(bool Enabled)
{
	BlockLock Lock(m_FilterLock);

	if (m_BufferingEnabled != Enabled) {
		if (Enabled) {
			m_DataStreamer.Initialize();
			if (!m_DataStreamer.Start())
				return false;
		} else {
			m_DataStreamer.Stop();
			m_DataStreamer.FlushBuffer(std::chrono::seconds(10));
			m_DataStreamer.Close();
		}

		m_BufferingEnabled = Enabled;
	}

	return true;
}


void StreamBufferFilter::SetClearOnReset(bool Clear)
{
	BlockLock Lock(m_FilterLock);

	m_ClearOnReset = Clear;
}


}	// namespace LibISDB
