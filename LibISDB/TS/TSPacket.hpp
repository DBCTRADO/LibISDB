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
 @file   TSPacket.hpp
 @brief  TS パケット
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_PACKET_H
#define LIBISDB_TS_PACKET_H


#include "../Base/DataBuffer.hpp"
#include "../Utilities/Utilities.hpp"


namespace LibISDB
{

	/** TS パケットクラス */
	class TSPacket
		: public DataBuffer
	{
		/** パケットヘッダ */
		struct TSPacketHeader {
			uint8_t SyncByte;                   /**< Sync Byte */
			bool TransportErrorIndicator;       /**< Transport Error Indicator */
			bool PayloadUnitStartIndicator;     /**< Payload Unit Start Indicator */
			bool TransportPriority;             /**< Transport Priority */
			uint16_t PID;                       /**< PID */
			uint8_t TransportScramblingControl; /**< Transport Scrambling Control */
			uint8_t AdaptationFieldControl;     /**< Adaptation Field Control */
			uint8_t ContinuityCounter;          /**< Continuity Counter */
		};

		/** アダプテーションフィールドヘッダ */
		struct AdaptationFieldHeader {
			uint8_t AdaptationFieldLength;      /**< Adaptation Field Length */
			uint8_t Flags;                      /**< フラグ */
			bool DiscontinuityIndicator;        /**< Discontinuity Indicator */
			uint8_t OptionSize;                 /**< オプションフィールド長 */
		};

		struct AdaptationFieldFlag {
			static constexpr uint8_t DiscontinuityIndicator   = 0x80_u8;
			static constexpr uint8_t RandomAccessIndicator    = 0x40_u8;
			static constexpr uint8_t ESPriorityIndicator      = 0x20_u8;
			static constexpr uint8_t PCRFlag                  = 0x10_u8;
			static constexpr uint8_t OPCRFlag                 = 0x08_u8;
			static constexpr uint8_t SplicingPointFlag        = 0x04_u8;
			static constexpr uint8_t TransportPrivateDataFlag = 0x02_u8;
			static constexpr uint8_t AdaptationFieldExtFlag   = 0x01_u8;
		};

	public:
		static constexpr unsigned long TypeID = 0x5453504BUL; // 'TSPK'

		/** ParsePacket() エラーコード */
		enum class ParseResult {
			OK,              /**< 正常パケット */
			FormatError,     /**< フォーマットエラー */
			TransportError,  /**< トランスポートエラー(ビットエラー) */
			ContinuityError, /**< 連続性カウンタエラー(ドロップ) */
		};

		TSPacket();
		TSPacket(const TSPacket &Src);
		TSPacket(TSPacket &&Src);
		~TSPacket();

		TSPacket & operator = (const TSPacket &) = default;
		TSPacket & operator = (TSPacket &&Src);

		ParseResult ParsePacket(uint8_t *pContinuityCounter = nullptr);
		void ReparsePacket();

		uint8_t * GetPayloadData();
		const uint8_t * GetPayloadData() const;
		uint8_t GetPayloadSize() const;

		uint16_t GetPID() const noexcept { return m_Header.PID; }
		void SetPID(uint16_t PID);
		bool HaveAdaptationField() const noexcept { return (m_Header.AdaptationFieldControl & 0x02) != 0; }
		bool HavePayload() const noexcept { return (m_Header.AdaptationFieldControl & 0x01) != 0; }
		bool IsScrambled() const noexcept { return (m_Header.TransportScramblingControl & 0x02) != 0; }

		bool GetTransportErrorIndicator() const noexcept { return m_Header.TransportErrorIndicator; }
		bool GetPayloadUnitStartIndicator() const noexcept { return m_Header.PayloadUnitStartIndicator; }
		bool GetTransportPriority() const noexcept { return m_Header.TransportPriority; }
		uint8_t GetTransportScramblingControl() const noexcept { return m_Header.TransportScramblingControl; }
		bool GetDiscontinuityIndicator() const noexcept { return m_AdaptationField.DiscontinuityIndicator; }
		bool GetRandomAccessIndicator() const noexcept { return (m_AdaptationField.Flags & AdaptationFieldFlag::RandomAccessIndicator) != 0; }
		bool GetESPriorityIndicator() const noexcept { return (m_AdaptationField.Flags & AdaptationFieldFlag::ESPriorityIndicator) != 0; }
		bool GetPCRFlag() const noexcept { return (m_AdaptationField.Flags & AdaptationFieldFlag::PCRFlag) != 0; }
		bool GetOPCRFlag() const noexcept { return (m_AdaptationField.Flags & AdaptationFieldFlag::OPCRFlag) != 0; }
		bool GetSplicingPointFlag() const noexcept { return (m_AdaptationField.Flags & AdaptationFieldFlag::SplicingPointFlag) != 0; }
		bool GetTransportPrivateDataFlag() const noexcept { return (m_AdaptationField.Flags & AdaptationFieldFlag::TransportPrivateDataFlag) != 0; }
		bool GetAdaptationFieldExtFlag() const noexcept { return (m_AdaptationField.Flags & AdaptationFieldFlag::AdaptationFieldExtFlag) != 0; }
		const uint8_t * GetOptionData() const noexcept { return m_AdaptationField.OptionSize ? &m_pData[6] : nullptr; }
		uint8_t GetOptionSize() const noexcept { return m_AdaptationField.OptionSize; }

	private:
	// DataBuffer
		void * Allocate(size_t Size) override;
		void Free(void *pBuffer) override;
		void * ReAllocate(void *pBuffer, size_t Size) override;

#ifdef LIBISDB_TS_PACKET_PAYLOAD_ALIGN
		uint8_t m_Data[LIBISDB_TS_PACKET_PAYLOAD_ALIGN + RoundUp(TS_PACKET_SIZE, (size_t)LIBISDB_TS_PACKET_PAYLOAD_ALIGN)];
#else
		uint8_t m_Data[TS_PACKET_SIZE];
#endif
		TSPacketHeader m_Header;
		AdaptationFieldHeader m_AdaptationField;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TS_PACKET_H
