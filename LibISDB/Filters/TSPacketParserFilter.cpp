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
 @file   TSPacketParserFilter.cpp
 @brief  TSパケット解析フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "TSPacketParserFilter.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


TSPacketParserFilter::TSPacketParserFilter()
	: m_OutOfSyncCount(0)

	, m_OutputSequence(true)
	, m_MaxSequencePacketCount(64)
	, m_OutputNullPacket(false)
	, m_OutputErrorPacket(false)
	, m_Generate1SegPAT(true)

	, m_InputBytes(0)
	, m_TotalInputBytes(0)
{
	m_ContinuityCounter.fill(0x10);
}


void TSPacketParserFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_TotalPacketCount += m_PacketCount;
	m_PacketCount.Reset();

	for (size_t i = 0; i < m_PIDPacketCount.size(); i++) {
		m_PIDTotalPacketCount[i] += m_PIDPacketCount[i];
		m_PIDPacketCount[i].Reset();
	}

	m_TotalInputBytes += m_InputBytes;
	m_InputBytes = 0;

	m_ContinuityCounter.fill(0x10);

	m_Packet.ClearSize();
	m_PacketSequence.SetDataCount(0);
	m_OutOfSyncCount = 0;

	m_PATGenerator.Reset();
}


bool TSPacketParserFilter::StartStreaming()
{
	if (!FilterBase::StartStreaming())
		return false;

	BlockLock Lock(m_FilterLock);

	if (m_OutputSequence)
		m_PacketSequence.Allocate(m_MaxSequencePacketCount);

	return true;
}


bool TSPacketParserFilter::ReceiveData(DataStream *pData)
{
	BlockLock Lock(m_FilterLock);

	do {
		DataBuffer *pBuffer = pData->GetData();
		SyncPacket(pBuffer->GetData(), pBuffer->GetSize());
	} while (pData->Next());

	if (m_PacketSequence.GetDataCount() > 0) {
		OutputData(m_PacketSequence);
		m_PacketSequence.SetDataCount(0);
	}

	return true;
}


void TSPacketParserFilter::SetOutputSequence(bool Enable)
{
	BlockLock Lock(m_FilterLock);

	m_OutputSequence = Enable;
}


bool TSPacketParserFilter::SetMaxSequencePacketCount(size_t Count)
{
	if (LIBISDB_TRACE_ERROR_IF(Count < 1))
		return false;

	BlockLock Lock(m_FilterLock);

	m_MaxSequencePacketCount = Count;

	return true;
}


void TSPacketParserFilter::SetOutputNullPacket(bool Enable)
{
	BlockLock Lock(m_FilterLock);

	m_OutputNullPacket = Enable;
}


void TSPacketParserFilter::SetOutputErrorPacket(bool Enable)
{
	BlockLock Lock(m_FilterLock);

	m_OutputErrorPacket = Enable;
}


TSPacketParserFilter::PacketCountInfo TSPacketParserFilter::GetPacketCount() const
{
	BlockLock Lock(m_FilterLock);

	return m_PacketCount;
}


TSPacketParserFilter::PacketCountInfo TSPacketParserFilter::GetPacketCount(uint16_t PID) const
{
	if (LIBISDB_TRACE_ERROR_IF(PID > PID_MAX))
		return PacketCountInfo();

	BlockLock Lock(m_FilterLock);

	return m_PIDPacketCount[PID];
}


TSPacketParserFilter::PacketCountInfo TSPacketParserFilter::GetTotalPacketCount() const
{
	BlockLock Lock(m_FilterLock);

	return m_TotalPacketCount + m_PacketCount;
}


TSPacketParserFilter::PacketCountInfo TSPacketParserFilter::GetTotalPacketCount(uint16_t PID) const
{
	if (LIBISDB_TRACE_ERROR_IF(PID > PID_MAX))
		return PacketCountInfo();

	BlockLock Lock(m_FilterLock);

	return m_PIDTotalPacketCount[PID] + m_PIDPacketCount[PID];
}


void TSPacketParserFilter::ResetErrorPacketCount()
{
	BlockLock Lock(m_FilterLock);

	m_PacketCount.FormatError = 0;
	m_PacketCount.TransportError = 0;
	m_PacketCount.ContinuityError = 0;
	m_PacketCount.Scrambled = 0;
}


unsigned long long TSPacketParserFilter::GetInputBytes() const
{
	BlockLock Lock(m_FilterLock);

	return m_InputBytes;
}


unsigned long long TSPacketParserFilter::GetTotalInputBytes() const
{
	BlockLock Lock(m_FilterLock);

	return m_TotalInputBytes + m_InputBytes;
}


void TSPacketParserFilter::SetGenerate1SegPAT(bool Enable)
{
	BlockLock Lock(m_FilterLock);

	m_Generate1SegPAT = Enable;
}


bool TSPacketParserFilter::SetTransportStreamID(uint16_t TransportStreamID)
{
	BlockLock Lock(m_FilterLock);

	return m_PATGenerator.SetTransportStreamID(TransportStreamID);
}


void TSPacketParserFilter::SyncPacket(const uint8_t *pData, size_t Size)
{
	m_InputBytes += Size;

	static const uint8_t SYNC_BYTE = 0x47_u8;
	size_t CurPos = 0;

	while (CurPos < Size) {
		size_t CurSize = m_Packet.GetSize();

		if (CurSize == 0) {
			// 同期バイト待ち中
			do {
				if (pData[CurPos++] == SYNC_BYTE) {
					// 同期バイト発見
					m_Packet.AddByte(SYNC_BYTE);
					break;
				}
				m_OutOfSyncCount++;
			} while (CurPos < Size);
		} else {
			if (CurSize < TS_PACKET_SIZE) {
				// データ待ち中
				const size_t Remain = std::min(TS_PACKET_SIZE - CurSize, Size - CurPos);

				m_Packet.AddData(&pData[CurPos], Remain);
				CurPos += Remain;
				CurSize += Remain;
			}

			if (CurSize == TS_PACKET_SIZE) {
				// パケットサイズ分データが揃った
				const TSPacket::ParseResult Result = m_Packet.ParsePacket(m_ContinuityCounter.data());

				if ((m_OutOfSyncCount > TS_PACKET_SIZE_MAX - TS_PACKET_SIZE)
						&& ((Result == TSPacket::ParseResult::FormatError)
							|| (Result == TSPacket::ParseResult::TransportError))) {
					// 同期バイトが他にもある場合、そこから再同期する
					bool Resync = false;
					for (size_t i = 1; i < TS_PACKET_SIZE; i++) {
						if (m_Packet.GetAt(i) == SYNC_BYTE) {
							m_Packet.TrimHead(i);
							Resync = true;
							break;
						}
					}
					if (Resync)
						continue;
				}

				ProcessPacket(Result);

				m_OutOfSyncCount = 0;
			}
		}
	}
}


void TSPacketParserFilter::ProcessPacket(TSPacket::ParseResult Result)
{
	++m_PacketCount.Input;

	const uint16_t PID = m_Packet.GetPID();
	bool Output = false;

	switch (Result) {
	case TSPacket::ParseResult::ContinuityError:
		++m_PacketCount.ContinuityError;
		++m_PIDPacketCount[PID].ContinuityError;
		[[fallthrough]];
	case TSPacket::ParseResult::OK:
		{
			++m_PIDPacketCount[PID].Input;

			if (m_Packet.IsScrambled()) {
				++m_PacketCount.Scrambled;
				++m_PIDPacketCount[PID].Scrambled;
			}

#ifdef LIBISDB_ONESEG_PAT_SIMULATE
			// PAT の無い状態をシミュレート(PAT 生成のデバッグ用)
			if (PID == PID_PAT) {
				OK = true;
				break;
			}
#endif
			if (m_PATGenerator.StorePacket(&m_Packet) && m_Generate1SegPAT) {
				if (m_PATGenerator.GetPATPacket(&m_PATPacket))
					OutputPacket(m_PATPacket);
			}

			if (m_OutputNullPacket || (PID != PID_NULL))
				Output = true;
		}
		break;

	case TSPacket::ParseResult::FormatError:
		++m_PacketCount.FormatError;

		if (m_OutputErrorPacket)
			Output = true;
		break;

	case TSPacket::ParseResult::TransportError:
		++m_PacketCount.TransportError;

		if (m_OutputErrorPacket)
			Output = true;
		break;
	}

	if (Output)
		OutputPacket(m_Packet);

	m_Packet.ClearSize();
}


void TSPacketParserFilter::OutputPacket(TSPacket &Packet)
{
	const uint16_t PID = Packet.GetPID();

	++m_PacketCount.Output;
	++m_PIDPacketCount[PID].Output;

	if (m_OutputSequence) {
		if ((m_PacketSequence.GetDataCount() >= m_MaxSequencePacketCount)
				|| ((m_PacketSequence.GetDataCount() > 0)
					&& (m_PacketSequence[0].GetPID() != Packet.GetPID()))) {
			OutputData(m_PacketSequence);
			m_PacketSequence.SetDataCount(0);
		}

		m_PacketSequence.AddData(Packet);
	} else {
		OutputData(&Packet);
	}
}


}	// namespace LibISDB
