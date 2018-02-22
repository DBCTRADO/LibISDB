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
 @file   StreamBufferDataStreamer.cpp
 @brief  ストリームバッファデータストリーマ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "StreamBufferDataStreamer.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


StreamBufferDataStreamer::~StreamBufferDataStreamer()
{
	Close();
}


bool StreamBufferDataStreamer::SetOutputBuffer(const std::shared_ptr<StreamBuffer> &Buffer)
{
	if (!Buffer)
		return false;

	BlockLock Lock(m_Lock);

	m_OutputBuffer = Buffer;

	return true;
}


std::shared_ptr<StreamBuffer> StreamBufferDataStreamer::GetOutputBuffer() const
{
	return m_OutputBuffer;
}


std::shared_ptr<StreamBuffer> StreamBufferDataStreamer::DetachOutputBuffer()
{
	BlockLock Lock(m_Lock);

	return std::move(m_OutputBuffer);
}


void StreamBufferDataStreamer::FreeOutputBuffer()
{
	BlockLock Lock(m_Lock);

	m_OutputBuffer.reset();
}


bool StreamBufferDataStreamer::ClearOutputBuffer()
{
	if (!m_OutputBuffer)
		return false;

	m_OutputBuffer->Clear();

	return true;
}


bool StreamBufferDataStreamer::HasOutputBuffer() const
{
	return static_cast<bool>(m_OutputBuffer);
}


size_t StreamBufferDataStreamer::OutputData(const uint8_t *pData, size_t DataSize)
{
	if (!m_OutputBuffer)
		return 0;

	return m_OutputBuffer->PushBack(pData, DataSize);
}


bool StreamBufferDataStreamer::IsOutputValid() const
{
	return static_cast<bool>(m_OutputBuffer);
}


void StreamBufferDataStreamer::ClearOutput()
{
	if (m_OutputBuffer)
		m_OutputBuffer->Clear();
}


}	// namespace LibISDB
