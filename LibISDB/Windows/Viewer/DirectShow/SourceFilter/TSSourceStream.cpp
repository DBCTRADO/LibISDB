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
 @file   TSSourceStream.cpp
 @brief  TS ソースストリーム
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "TSSourceStream.hpp"
#include "../../../../TS/TSPacket.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{

namespace
{


constexpr long long PTS_CLOCK = 90000LL;	// 90kHz

constexpr long long ERR_PTS_DIFF    = PTS_CLOCK * 5;
constexpr long long MAX_AUDIO_DELAY = PTS_CLOCK;
constexpr long long MIN_AUDIO_DELAY = PTS_CLOCK / 5;




inline long long GetPTS(const uint8_t *p)
{
	return (static_cast<long long>(
	           (static_cast<unsigned long>(p[0] & 0x0E) << 14) |
	           (static_cast<unsigned long>(p[1]) << 7) |
	           (static_cast<unsigned long>(p[2]) >> 1)) << 15) |
	        static_cast<long long>(
	           (static_cast<unsigned long>(p[3]) << 7) |
	           (static_cast<unsigned long>(p[4]) >> 1));
}


inline long long GetPacketPTS(const TSPacket *pPacket)
{
	if (pPacket->GetPayloadSize() >= 14) {
		const uint8_t *pData = pPacket->GetPayloadData();
		if (pData != nullptr) {
			if ((pData[0] == 0) && (pData[1] == 0) && (pData[2] == 1)) {	// PES
				if ((pData[7] & 0x80) != 0) {	// pts_flag
					return GetPTS(&pData[9]);
				}
			}
		}
	}

	return -1;
}


}	// namespace




TSSourceStream::TSSourceStream()
	: m_QueueSize(DEFAULT_QUEUE_SIZE)
	, m_PoolSize(DEFAULT_POOL_SIZE)
	, m_EnableSync(false)
	, m_SyncFor1Seg(false)
	, m_VideoPID(PID_INVALID)
	, m_AudioPID(PID_INVALID)
	, m_MapAudioPID(PID_INVALID)
{
	Reset();
}


bool TSSourceStream::Initialize()
{
	Reset();

	if (!ResizeQueue(m_QueueSize, m_EnableSync ? m_PoolSize : 0))
		return false;

	if (m_EnableSync) {
		if (!m_PacketPool.Allocate(m_PoolSize))
			return false;
	}

	return true;
}


bool TSSourceStream::InputData(DataBuffer *pData)
{
	BlockLock Lock(m_Lock);

	TSPacket *pPacket = static_cast<TSPacket *>(pData);
	const uint16_t PID = pPacket->GetPID();
	if (PID != m_VideoPID && PID != m_AudioPID) {
		if (PID != m_MapAudioPID)
			AddData(pData);
		return true;
	}

	const bool bVideoPacket = (PID == m_VideoPID);

	if (!bVideoPacket && m_MapAudioPID != PID_INVALID) {
		pPacket->SetPID(m_MapAudioPID);
	}

	if (pPacket->GetPayloadUnitStartIndicator()) {
		const long long PTS = GetPacketPTS(pPacket);
		if (PTS >= 0) {
			if (bVideoPacket) {
				m_VideoPTSPrev = m_VideoPTS;
				m_VideoPTS = PTS;
			} else {
				if (m_AudioPTSPrev >= 0) {
					if (m_AudioPTSPrev < PTS)
						m_PTSDuration += PTS - m_AudioPTSPrev;
				}
				m_AudioPTSPrev = m_AudioPTS;
				m_AudioPTS = PTS;
			}
		}
	}

	if (!m_EnableSync || m_PacketPool.GetCapacity() == 0) {
		AddData(pData);
		return true;
	}

	if (m_SyncFor1Seg) {
		long long AudioPTS = m_AudioPTS;
		if (m_AudioPTSPrev >= 0) {
			if (AudioPTS < m_AudioPTSPrev) {
				LIBISDB_TRACE(LIBISDB_STR("Audio PTS wrap-around\n"));
				AudioPTS += 0x200000000LL;
			}
			if (AudioPTS >= m_AudioPTSPrev + ERR_PTS_DIFF) {
				LIBISDB_TRACE(
					LIBISDB_STR("Reset Audio PTS : Adj=%llX Cur=%llX Prev=%llX\n"),
					AudioPTS, m_AudioPTS, m_AudioPTSPrev);
				AddPoolPackets();
				ResetSync();
				AudioPTS = -1;
			}
		}

		if (bVideoPacket && m_VideoPTS >= 0) {
			if (m_PacketPool.IsFull())
				AddPoolPacket();
			PacketPTSData *pPacketData = m_PacketPool.Push();
			std::memcpy(pPacketData->Data, pData->GetData(), TS_PACKET_SIZE);
			pPacketData->PTS = m_VideoPTS;
		} else {
			AddData(pData);
		}

		while (!m_PacketPool.IsEmpty() && !m_PacketQueue.IsFull()) {
			PacketPTSData *pPacketData = m_PacketPool.Front();
			if (AudioPTS < 0
					|| pPacketData->PTS <= AudioPTS + MAX_AUDIO_DELAY
					|| pPacketData->PTS >= AudioPTS + ERR_PTS_DIFF) {
				AddPacket(pPacketData);
				m_PacketPool.Pop();
			} else {
				break;
			}
		}
	} else {
		long long VideoPTS = m_VideoPTS;
		if (m_VideoPTSPrev >= 0 && _abs64(VideoPTS - m_VideoPTSPrev) >= ERR_PTS_DIFF) {
			if (VideoPTS < m_VideoPTSPrev) {
				LIBISDB_TRACE(LIBISDB_STR("Video PTS wrap-around\n"));
				VideoPTS += 0x200000000LL;
			}
			if (VideoPTS >= m_VideoPTSPrev + ERR_PTS_DIFF) {
				LIBISDB_TRACE(
					LIBISDB_STR("Reset Video PTS : Adj=%llX Cur=%llX Prev=%llX\n"),
					VideoPTS, m_VideoPTS, m_VideoPTSPrev);
				AddPoolPackets();
				ResetSync();
				VideoPTS = -1;
			}
		}

		if (!bVideoPacket && m_AudioPTS >= 0) {
			if (m_PacketPool.IsFull())
				AddPoolPacket();
			PacketPTSData *pPacketData = m_PacketPool.Push();
			std::memcpy(pPacketData->Data, pData->GetData(), TS_PACKET_SIZE);
			pPacketData->PTS = m_AudioPTS;
		} else {
			AddData(pData);
		}

		while (!m_PacketPool.IsEmpty() && !m_PacketQueue.IsFull()) {
			PacketPTSData *pPacketData = m_PacketPool.Front();
			if (VideoPTS < 0
					|| pPacketData->PTS + MIN_AUDIO_DELAY <= VideoPTS
					|| pPacketData->PTS >= VideoPTS + ERR_PTS_DIFF) {
				AddPacket(pPacketData);
				m_PacketPool.Pop();
			} else {
				break;
			}
		}
	}

	return true;
}


void TSSourceStream::AddData(const uint8_t *pData)
{
	if (m_PacketQueue.IsFull())
		m_PacketQueue.Pop(m_QueueSize / 2);
	m_PacketQueue.Write(pData);
}


void TSSourceStream::AddPoolPacket()
{
	if (!m_PacketPool.IsEmpty()) {
		AddPacket(m_PacketPool.Front());
		m_PacketPool.Pop();
	}
}


void TSSourceStream::AddPoolPackets()
{
	while (!m_PacketPool.IsEmpty()) {
		AddPacket(m_PacketPool.Front());
		m_PacketPool.Pop();
	}
}


size_t TSSourceStream::GetData(uint8_t *pData, size_t Size)
{
	if (pData == nullptr || Size == 0)
		return 0;

	BlockLock Lock(m_Lock);

	if (m_PacketQueue.IsEmpty())
		return 0;

	const size_t ActualSize = m_PacketQueue.Read(pData, Size);

	// 不要そうなメモリを解放
	if (m_PacketQueue.GetAllocatedChunkCount() >= 8
			&& m_PacketQueue.GetUsed() + m_PacketQueue.GetChunkSize() < m_PacketQueue.GetAllocatedSize() / 2) {
		LIBISDB_TRACE(LIBISDB_STR("TSSourceStream::GetData() : Shrink to fit\n"));
		m_PacketQueue.ShrinkToFit();
	}

	return ActualSize;
}


void TSSourceStream::Reset()
{
	BlockLock Lock(m_Lock);

	ResetSync();
	//m_PacketQueue.Clear();
	m_PacketQueue.Free();
}


void TSSourceStream::ResetSync()
{
	m_VideoPTS = -1;
	m_VideoPTSPrev = -1;
	m_AudioPTS = -1;
	m_AudioPTSPrev = -1;
	m_PTSDuration = 0;
	m_PacketPool.Clear();
}


bool TSSourceStream::IsDataAvailable()
{
	BlockLock Lock(m_Lock);

	return !m_PacketQueue.IsEmpty();
}


bool TSSourceStream::IsBufferFull()
{
	BlockLock Lock(m_Lock);

	return m_PacketQueue.GetUsed() >= m_QueueSize;
}


bool TSSourceStream::IsBufferActuallyFull()
{
	BlockLock Lock(m_Lock);

	return m_PacketQueue.IsFull();
}


int TSSourceStream::GetFillPercentage()
{
	BlockLock Lock(m_Lock);

	if (m_PacketQueue.GetCapacity() == 0)
		return 0;

	return static_cast<int>(m_PacketQueue.GetUsed() * 100 / m_PacketQueue.GetCapacity());
}


bool TSSourceStream::SetQueueSize(size_t Size)
{
	if (Size == 0)
		return false;

	BlockLock Lock(m_Lock);

	if (m_QueueSize != Size) {
		if (!ResizeQueue(Size, m_EnableSync ? m_PoolSize : 0))
			return false;
		m_QueueSize = Size;
	}

	return true;
}


bool TSSourceStream::SetPoolSize(size_t Size)
{
	if (Size == 0)
		return false;

	BlockLock Lock(m_Lock);

	if (m_PoolSize != Size) {
		if (m_PacketPool.IsAllocated()) {
			if (!m_PacketPool.Resize(Size))
				return false;
		}
		m_PoolSize = Size;
	}

	return true;
}


bool TSSourceStream::EnableSync(bool Enable, bool OneSeg)
{
	BlockLock Lock(m_Lock);

	if (m_EnableSync != Enable || m_SyncFor1Seg != OneSeg) {
		LIBISDB_TRACE(LIBISDB_STR("TSSourceStream::EnableSync(%d, %d)\n"), Enable, OneSeg);

		ResetSync();

		if (!m_EnableSync && Enable) {
			if (!m_PacketPool.Allocate(m_PoolSize))
				return false;
		}

		m_EnableSync = Enable;
		m_SyncFor1Seg = OneSeg;
	}

	return true;
}


void TSSourceStream::SetVideoPID(uint16_t PID)
{
	BlockLock Lock(m_Lock);

	m_VideoPID = PID;
}


void TSSourceStream::SetAudioPID(uint16_t PID)
{
	BlockLock Lock(m_Lock);

	m_AudioPID = PID;
	m_MapAudioPID = PID_INVALID;
}


void TSSourceStream::MapAudioPID(uint16_t AudioPID, uint16_t MapPID)
{
	LIBISDB_TRACE(LIBISDB_STR("TSSourceStream::MapAudioPID() : %04x -> %04x\n"), AudioPID, MapPID);

	BlockLock Lock(m_Lock);

	m_AudioPID = AudioPID;
	if (AudioPID == MapPID)
		m_MapAudioPID = PID_INVALID;
	else
		m_MapAudioPID = MapPID;
}


bool TSSourceStream::ResizeQueue(size_t QueueSize, size_t PoolSize)
{
	const size_t ChunkSize = m_PacketQueue.GetChunkSize();
	return m_PacketQueue.Resize((QueueSize + PoolSize + ChunkSize - 1) / ChunkSize);
}


}	// namespace LibISDB::DirectShow
