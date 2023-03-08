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
 @file   PESPacket.cpp
 @brief  PES パケット
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "PESPacket.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{

namespace
{

namespace StreamID
{

	constexpr uint8_t PROGRAM_STREAM_MAP                   = 0xBC; // program_stream_map
	constexpr uint8_t PRIVATE_STREAM_1                     = 0xBD; // private_stram_1
	constexpr uint8_t PADDING_STREAM                       = 0xBE; // padding_stream
	constexpr uint8_t PRIVATE_STREAM_2                     = 0xBF; // private_stream_2
	constexpr uint8_t ECM_STREAM                           = 0xF0; // ECM_stream
	constexpr uint8_t EMM_STREAM                           = 0xF1; // EMM_stream
	constexpr uint8_t DSMCC_STREAM                         = 0xF2; // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A or ISO/IEC 13818-6_DSMCC_stream
	constexpr uint8_t ISO_IEC_13522_STREAM                 = 0xF3; // ISO/IEC_13522_stream
	constexpr uint8_t ITU_T_REC_H222_1_TYPE_A              = 0xF4; // ITU-T Rec. H.222.1 type A
	constexpr uint8_t ITU_T_REC_H222_1_TYPE_B              = 0xF5; // ITU-T Rec. H.222.1 type B
	constexpr uint8_t ITU_T_REC_H222_1_TYPE_C              = 0xF6; // ITU-T Rec. H.222.1 type C
	constexpr uint8_t ITU_T_REC_H222_1_TYPE_D              = 0xF7; // ITU-T Rec. H.222.1 type D
	constexpr uint8_t ITU_T_REC_H222_1_TYPE_E              = 0xF8; // ITU-T Rec. H.222.1 type E
	constexpr uint8_t ANCILLARY_STREAM                     = 0xF9; // ancillary_stram
	constexpr uint8_t ISO_IEC_14496_1_SL_PACKETIZED_STREAM = 0xFA; // ISO/IEC14496-1_SL-packetized_stream
	constexpr uint8_t ISO_IEC_14496_1_FLEXMUX_STREAM       = 0xFB; // ISO/IEC14496-1_FlexMux_stream
	constexpr uint8_t PROGRAM_STREAM_DIRECTORY             = 0xFF; // program_stream_directory

}

constexpr bool IsAdditionalHeaderStreamID(uint8_t ID)
{
	return (ID != StreamID::PROGRAM_STREAM_MAP)
	    && (ID != StreamID::PADDING_STREAM)
	    && (ID != StreamID::PRIVATE_STREAM_2)
	    && (ID != StreamID::ECM_STREAM)
	    && (ID != StreamID::EMM_STREAM)
	    && (ID != StreamID::PROGRAM_STREAM_DIRECTORY)
	    && (ID != StreamID::DSMCC_STREAM)
	    && (ID != StreamID::ITU_T_REC_H222_1_TYPE_E);
}

}




PESPacket::PESPacket() noexcept
	: m_Header()
{
}


PESPacket::PESPacket(size_t BufferSize)
	: DataBuffer(BufferSize)
	, m_Header()
{
}


bool PESPacket::ParseHeader()
{
	m_Header = PESHeader();

	if (m_DataSize < 6)
		return false;
	if ((m_pData[0] != 0x00) || (m_pData[1] != 0x00) || (m_pData[2] != 0x01))
		return false;	// packet_start_code_prefix 異常

	m_Header.StreamID     = m_pData[3];
	m_Header.PacketLength = Load16(&m_pData[4]);

	if (IsAdditionalHeaderStreamID(m_Header.StreamID)) {
		if (m_DataSize < 9)
			return false;
		if ((m_pData[6] & 0xC0) != 0x80)
			return false;	// 固定ビット異常

		m_Header.ScramblingControl      = (m_pData[6] & 0x30) >> 4;
		m_Header.Priority               = (m_pData[6] & 0x08) != 0;
		m_Header.DataAlignmentIndicator = (m_pData[6] & 0x04) != 0;
		m_Header.Copyright              = (m_pData[6] & 0x02) != 0;
		m_Header.OriginalOrCopy         = (m_pData[6] & 0x01) != 0;
		m_Header.PTSDTSFlags            = (m_pData[7] & 0xC0) >> 6;
		m_Header.ESCRFlag               = (m_pData[7] & 0x20) != 0;
		m_Header.ESRateFlag             = (m_pData[7] & 0x10) != 0;
		m_Header.DSMTrickModeFlag       = (m_pData[7] & 0x08) != 0;
		m_Header.AdditionalCopyInfoFlag = (m_pData[7] & 0x04) != 0;
		m_Header.CRCFlag                = (m_pData[7] & 0x02) != 0;
		m_Header.ExtensionFlag          = (m_pData[7] & 0x01) != 0;
		m_Header.HeaderDataLength       = m_pData[8];

		if (m_Header.ScramblingControl != 0)
			return false;	// Not scrambled のみ対応
		if (m_Header.PTSDTSFlags == 1)
			return false;	// 未定義のフラグ
	}

	return true;
}


void PESPacket::Reset() noexcept
{
	ClearSize();

	m_Header = PESHeader();
}


int64_t PESPacket::GetPTSCount() const
{
	if (m_Header.PTSDTSFlags != 0)
		return GetPTS(&m_pData[9]);

	return -1;
}


uint16_t PESPacket::GetPacketCRC() const
{
	if (!m_Header.CRCFlag)
		return 0x0000_u16;

	size_t Pos = 9;

	if (m_Header.PTSDTSFlags == 2)
		Pos += 5;
	else if (m_Header.PTSDTSFlags == 3)
		Pos += 10;
	if (m_Header.ESCRFlag)
		Pos += 6;
	if (m_Header.ESRateFlag)
		Pos += 3;
	if (m_Header.DSMTrickModeFlag)
		Pos += 1;
	if (m_Header.AdditionalCopyInfoFlag)
		Pos += 1;

	if (m_DataSize < Pos + 2)
		return 0x0000_u16;

	return Load16(&m_pData[Pos]);
}


const uint8_t * PESPacket::GetPayloadData() const
{
	const size_t PayloadPos =
		IsAdditionalHeaderStreamID(m_Header.StreamID) ? (m_Header.HeaderDataLength + 9) : 6;
	if (m_DataSize <= PayloadPos)
		return nullptr;

	return &m_pData[PayloadPos];
}


size_t PESPacket::GetPayloadSize() const
{
	const size_t HeaderSize =
		IsAdditionalHeaderStreamID(m_Header.StreamID) ? (m_Header.HeaderDataLength + 9) : 6;
	if (m_DataSize <= HeaderSize)
		return 0;

	return m_DataSize - HeaderSize;
}




PESParser::PESParser(PacketHandler *pPacketHandler) noexcept
	: m_pPacketHandler(pPacketHandler)
	, m_PESPacket(0x10005_z)
	, m_IsStoring(false)
	, m_StoreSize(0)
{
}


bool PESParser::StorePacket(const TSPacket *pPacket)
{
	const uint8_t *pData = pPacket->GetPayloadData();
	const uint8_t Size = pPacket->GetPayloadSize();
	if ((pData == nullptr) || (Size == 0))
		return false;

	bool Trigger = false;
	uint8_t Pos = 0;

	if (pPacket->GetPayloadUnitStartIndicator()) {
		// ヘッダ先頭 + [ペイロード断片]

		if (m_IsStoring && (m_PESPacket.GetPacketLength() == 0)) {
			OnPESPacket(&m_PESPacket);
		}

		m_IsStoring = false;
		Trigger = true;
		m_PESPacket.ClearSize();

		Pos += StoreHeader(&pData[Pos], Size - Pos);
		Pos += StorePayload(&pData[Pos], Size - Pos);
	} else {
		// [ヘッダ断片] + ペイロード + [スタッフィングバイト]
		Pos += StoreHeader(&pData[Pos], Size - Pos);
		Pos += StorePayload(&pData[Pos], Size - Pos);
	}

	return Trigger;
}


void PESParser::Reset() noexcept
{
	m_PESPacket.Reset();
	m_IsStoring = false;
	m_StoreSize = 0;
}


void PESParser::OnPESPacket(const PESPacket *pPacket) const
{
	// ハンドラ呼び出し
	if (m_pPacketHandler != nullptr)
		m_pPacketHandler->OnPESPacket(this, pPacket);
}


uint8_t PESParser::StoreHeader(const uint8_t *pPayload, uint8_t Remain)
{
	// ヘッダを解析してセクションのストアを開始する
	if (m_IsStoring)
		return 0;

	const uint8_t HeaderRemain = static_cast<uint8_t>(9 - m_PESPacket.GetSize());

	if (Remain >= HeaderRemain) {
		// ヘッダストア完了、ヘッダを解析してペイロードのストアを開始する
		m_PESPacket.AddData(pPayload, HeaderRemain);
		if (m_PESPacket.ParseHeader()) {
			// ヘッダフォーマットOK
			m_StoreSize = m_PESPacket.GetPacketLength();
			if (m_StoreSize != 0)
				m_StoreSize += 6;
			m_IsStoring = true;
			return HeaderRemain;
		} else {
			// ヘッダエラー
			m_PESPacket.Reset();
			return Remain;
		}
	} else {
		// ヘッダストア未完了、次のデータを待つ
		m_PESPacket.AddData(pPayload, Remain);
		return Remain;
	}
}


uint8_t PESParser::StorePayload(const uint8_t *pPayload, uint8_t Remain)
{
	// セクションのストアを完了する
	if (!m_IsStoring)
		return 0;

	const size_t StoreRemain = m_StoreSize - m_PESPacket.GetSize();

	if ((m_StoreSize != 0) && (StoreRemain <= Remain)) {
		// ストア完了
		m_PESPacket.AddData(pPayload, StoreRemain);

		// TODO: ここでCRCチェック

		OnPESPacket(&m_PESPacket);

		// 状態を初期化し、次のセクション受信に備える
		m_PESPacket.Reset();
		m_IsStoring = false;

		return static_cast<uint8_t>(StoreRemain);
	} else {
		// ストア未完了、次のペイロードを待つ
		m_PESPacket.AddData(pPayload, Remain);
		return Remain;
	}
}


}	// namespace LibISDB
