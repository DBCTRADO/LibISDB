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
 @file   TSPacketParserFilter.hpp
 @brief  TSパケット解析フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_PACKET_PARSER_FILTER_H
#define LIBISDB_TS_PACKET_PARSER_FILTER_H


#include "FilterBase.hpp"
#include "../TS/TSPacket.hpp"
#include "../TS/OneSegPATGenerator.hpp"
#include <array>


namespace LibISDB
{

	/** TSパケット解析フィルタクラス */
	class TSPacketParserFilter
		: public SingleIOFilter
	{
	public:
		struct PacketCountInfo {
			unsigned long long Input = 0;
			unsigned long long Output = 0;
			unsigned long long FormatError = 0;
			unsigned long long TransportError = 0;
			unsigned long long ContinuityError = 0;
			unsigned long long Scrambled = 0;

			PacketCountInfo & operator += (const PacketCountInfo &rhs) noexcept
			{
				Input           += rhs.Input;
				Output          += rhs.Output;
				FormatError     += rhs.FormatError;
				TransportError  += rhs.TransportError;
				ContinuityError += rhs.ContinuityError;
				Scrambled       += rhs.Scrambled;
				return *this;
			}

			PacketCountInfo operator + (const PacketCountInfo &rhs) const noexcept
			{
				return PacketCountInfo(*this) += rhs;
			}

			void Reset() noexcept
			{
				*this = PacketCountInfo();
			}
		};

		TSPacketParserFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TSPacketParserFilter"); }

	// FilterBase
		void Reset() override;
		bool StartStreaming() override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	// TSPacketParserFilter
		void SetOutputSequence(bool Enable);
		bool GetOutputSequence() const noexcept { return m_OutputSequence; }
		bool SetMaxSequencePacketCount(size_t Count);
		size_t GetMaxSequencePacketCount() const noexcept { return m_MaxSequencePacketCount; }
		void SetOutputNullPacket(bool Enable);
		bool GetOutputNullPacket() const noexcept { return m_OutputNullPacket; }
		void SetOutputErrorPacket(bool Enable);
		bool GetOutputErrorPacket() const noexcept { return m_OutputErrorPacket; }

		PacketCountInfo GetPacketCount() const;
		PacketCountInfo GetPacketCount(uint16_t PID) const;
		PacketCountInfo GetTotalPacketCount() const;
		PacketCountInfo GetTotalPacketCount(uint16_t PID) const;
		void ResetErrorPacketCount();
		unsigned long long GetInputBytes() const;
		unsigned long long GetTotalInputBytes() const;

		void SetGenerate1SegPAT(bool Enable);
		bool GetGenerate1SegPAT() const noexcept { return m_Generate1SegPAT; }
		bool SetTransportStreamID(uint16_t TransportStreamID);

	private:
		void SyncPacket(const uint8_t *pData, size_t Size);
		void ProcessPacket(TSPacket::ParseResult Result);
		void OutputPacket(TSPacket &Packet);

		TSPacket m_Packet;
		DataStreamSequence<TSPacket> m_PacketSequence;
		size_t m_OutOfSyncCount;

		bool m_OutputSequence;
		size_t m_MaxSequencePacketCount;
		bool m_OutputNullPacket;
		bool m_OutputErrorPacket;

		PacketCountInfo m_PacketCount;
		PacketCountInfo m_TotalPacketCount;
		std::array<PacketCountInfo, PID_MAX + 1> m_PIDPacketCount;
		std::array<PacketCountInfo, PID_MAX + 1> m_PIDTotalPacketCount;
		std::array<uint8_t, PID_MAX> m_ContinuityCounter;
		unsigned long long m_InputBytes;
		unsigned long long m_TotalInputBytes;

		OneSegPATGenerator m_PATGenerator;
		bool m_Generate1SegPAT;
		TSPacket m_PATPacket;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TS_PACKET_PARSER_FILTER_H
