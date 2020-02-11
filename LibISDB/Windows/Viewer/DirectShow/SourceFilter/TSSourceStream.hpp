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
 @file   TSSourceStream.hpp
 @brief  TS ソースストリーム
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_SOURCE_STREAM_H
#define LIBISDB_TS_SOURCE_STREAM_H


#include "RingBuffer.hpp"
#include "../../../../Base/DataBuffer.hpp"
#include "../../../../Utilities/Lock.hpp"


namespace LibISDB::DirectShow
{

	/** TS ソースストリームクラス */
	class TSSourceStream
	{
	public:
		static constexpr size_t DEFAULT_QUEUE_SIZE = 0x1000;
		static constexpr size_t DEFAULT_POOL_SIZE  = 0x0800;

		TSSourceStream();

		bool Initialize();
		bool InputData(DataBuffer *pData);
		size_t GetData(uint8_t *pData, size_t Size);
		void Reset();
		bool IsDataAvailable();
		bool IsBufferFull();
		bool IsBufferActuallyFull();
		int GetFillPercentage();
		bool SetQueueSize(size_t Size);
		size_t GetQueueSize() const noexcept { return m_QueueSize; }
		bool SetPoolSize(size_t Size);
		size_t GetPoolSize() const noexcept { return m_PoolSize; }
		bool EnableSync(bool Enable, bool OneSeg = false);
		bool IsSyncEnabled() const noexcept { return m_EnableSync; }
		bool IsSyncFor1Seg() const noexcept { return m_SyncFor1Seg; }
		void SetVideoPID(uint16_t PID);
		void SetAudioPID(uint16_t PID);
		void MapAudioPID(uint16_t AudioPID, uint16_t MapPID);
		long long GetPTSDuration() const noexcept { return m_PTSDuration; }

	private:
		struct PacketPTSData {
			uint8_t Data[TS_PACKET_SIZE];
			long long PTS;
		};

		void ResetSync();
		void AddData(const uint8_t *pData);
		void AddData(const DataBuffer *pData) { AddData(pData->GetData()); }
		void AddPacket(const PacketPTSData *pPacket) { AddData(pPacket->Data); }
		void AddPoolPacket();
		void AddPoolPackets();
		bool ResizeQueue(size_t QueueSize, size_t PoolSize);

		MutexLock m_Lock;
		ChunkedRingBuffer<uint8_t, TS_PACKET_SIZE, 1024> m_PacketQueue;
		RingBuffer<PacketPTSData> m_PacketPool;

		size_t m_QueueSize;
		size_t m_PoolSize;
		bool m_EnableSync;
		bool m_SyncFor1Seg;
		long long m_VideoPTS;
		long long m_VideoPTSPrev;
		long long m_AudioPTS;
		long long m_AudioPTSPrev;
		long long m_PTSDuration;
		uint16_t m_VideoPID;
		uint16_t m_AudioPID;
		uint16_t m_MapAudioPID;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_TS_SOURCE_STREAM_H
