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
 @file   PESPacket.hpp
 @brief  PES パケット
 @author DBCTRADO
*/


#ifndef LIBISDB_PES_PACKET_H
#define LIBISDB_PES_PACKET_H


#include "TSPacket.hpp"


namespace LibISDB
{

	constexpr int64_t PTS_CLOCK = 90000_i64;	// 90kHz

	inline int64_t GetPTS(const uint8_t *p)
	{
		return
			(static_cast<int64_t>(
				(static_cast<uint32_t>(p[0] & 0x0E) << 14) |
				(static_cast<uint32_t>(p[1]) << 7) |
				(static_cast<uint32_t>(p[2]) >> 1)) << 15) |
			(static_cast<int64_t>(
				(static_cast<uint32_t>(p[3]) << 7) |
				(static_cast<uint32_t>(p[4]) >> 1)));
	}

	/** PES パケットクラス */
	class PESPacket
		: public DataBuffer
	{
	public:
		PESPacket();
		PESPacket(size_t BufferSize);

		bool ParseHeader();
		void Reset();

		uint8_t GetStreamID() const noexcept { return m_Header.StreamID; }
		uint16_t GetPacketLength() const noexcept { return m_Header.PacketLength; }
		uint8_t GetScramblingControl() const noexcept { return m_Header.ScramblingControl; }
		bool GetPriority() const noexcept { return m_Header.Priority; }
		bool GetDataAlignmentIndicator() const noexcept { return m_Header.DataAlignmentIndicator; }
		bool GetCopyright() const noexcept { return m_Header.Copyright; }
		bool GetOriginalOrCopy() const noexcept { return m_Header.OriginalOrCopy; }
		uint8_t GetPTSDTSFlags() const noexcept { return m_Header.PTSDTSFlags; }
		bool GetESCRFlag() const noexcept { return m_Header.ESCRFlag; }
		bool GetESRateFlag() const noexcept { return m_Header.ESRateFlag; }
		bool GetDSMTrickModeFlag() const noexcept { return m_Header.DSMTrickModeFlag; }
		bool GetAdditionalCopyInfoFlag() const noexcept { return m_Header.AdditionalCopyInfoFlag; }
		bool GetCRCFlag() const noexcept { return m_Header.CRCFlag; }
		bool GetExtensionFlag() const noexcept { return m_Header.ExtensionFlag; }
		uint8_t GetHeaderDataLength() const noexcept { return m_Header.HeaderDataLength; }

		int64_t GetPTSCount()const;

		uint16_t GetPacketCRC() const;

		const uint8_t * GetPayloadData() const;
		size_t GetPayloadSize() const;

	protected:
		struct PESHeader {
			uint8_t StreamID;				// Stream ID
			uint16_t PacketLength;			// PES Packet Length
			uint8_t ScramblingControl;		// PES Scrambling Control
			bool Priority;					// PES Priority
			bool DataAlignmentIndicator;	// Data Alignment Indicator
			bool Copyright;					// Copyright
			bool OriginalOrCopy;			// Original or Copy
			uint8_t PTSDTSFlags;			// PTS DTS Flags
			bool ESCRFlag;					// ESCR Flag
			bool ESRateFlag;				// ES Rate Flag
			bool DSMTrickModeFlag;			// DSM Trick Mode Flag
			bool AdditionalCopyInfoFlag;	// Additional Copy Info Flag
			bool CRCFlag;					// PES CRC Flag
			bool ExtensionFlag;				// PES Extension Flag
			uint8_t HeaderDataLength;		// PES Header Data Length
		};

		PESHeader m_Header;
	};

	/** PES 解析クラス */
	class PESParser
	{
	public:
		class PacketHandler
		{
		public:
			virtual void OnPESPacket(const PESParser *pParser, const PESPacket *pPacket) = 0;
		};

		PESParser(PacketHandler *pPacketHandler);

		bool StorePacket(const TSPacket *pPacket);
		void Reset();

	protected:
		virtual void OnPESPacket(const PESPacket *pPacket) const;

		PacketHandler *m_pPacketHandler;
		PESPacket m_PESPacket;

	private:
		uint8_t StoreHeader(const uint8_t *pPayload, uint8_t Remain);
		uint8_t StorePayload(const uint8_t *pPayload, uint8_t Remain);

		bool m_IsStoring;
		size_t m_StoreSize;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_PES_PACKET_H
