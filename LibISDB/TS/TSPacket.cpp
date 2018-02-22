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
 @file   TSPacket.cpp
 @brief  TS パケット
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "TSPacket.hpp"
#include "../Utilities/Utilities.hpp"
#ifdef LIBISDB_TS_PACKET_PAYLOAD_ALIGN
#include "../Utilities/AlignedAlloc.hpp"
#endif
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


TSPacket::TSPacket()
	: m_Header()
	, m_AdaptationField()
{
	m_TypeID = TypeID;

	AllocateBuffer(TS_PACKET_SIZE);
}


TSPacket::TSPacket(const TSPacket &Src)
{
	m_TypeID = TypeID;

	*this = Src;
}


TSPacket::TSPacket(TSPacket &&Src)
{
	m_TypeID = TypeID;

	*this = std::move(Src);
}


TSPacket::~TSPacket()
{
	FreeBuffer();
}


TSPacket & TSPacket::operator = (TSPacket &&Src)
{
	// move 不可
	return *this = Src;
}


TSPacket::ParseResult TSPacket::ParsePacket(uint8_t *pContinuityCounter)
{
	// TSパケットヘッダ解析
	const uint32_t Header = Load32(m_pData);
	m_Header.SyncByte                   = static_cast<uint8_t>(Header >> 24);
	m_Header.TransportErrorIndicator    = (Header & 0x800000_u32) != 0;
	m_Header.PayloadUnitStartIndicator  = (Header & 0x400000_u32) != 0;
	m_Header.TransportPriority          = (Header & 0x200000_u32) != 0;
	m_Header.PID                        = static_cast<uint16_t>((Header >> 8) & 0x1FFF);
	m_Header.TransportScramblingControl = static_cast<uint8_t>((Header >> 6) & 0x03);
	m_Header.AdaptationFieldControl     = static_cast<uint8_t>((Header >> 4) & 0x03);
	m_Header.ContinuityCounter          = static_cast<uint8_t>(Header & 0x0F);

	// アダプテーションフィールド解析
	m_AdaptationField = AdaptationFieldHeader();

	if (m_Header.AdaptationFieldControl & 0x02) {
		// アダプテーションフィールドあり
		m_AdaptationField.AdaptationFieldLength = m_pData[4];
		if (m_AdaptationField.AdaptationFieldLength > 0) {
			// フィールド長以降あり
			m_AdaptationField.Flags = m_pData[5];
			m_AdaptationField.DiscontinuityIndicator = (m_AdaptationField.Flags & AdaptationFieldFlag::DiscontinuityIndicator) != 0;

			if (m_AdaptationField.AdaptationFieldLength > 1) {
				m_AdaptationField.OptionSize = m_AdaptationField.AdaptationFieldLength - 1;
			}
		}
	}

	if (m_Header.SyncByte != 0x47_u8)
		return ParseResult::FormatError;	// 同期バイト不正
	if (m_Header.TransportErrorIndicator)
		return ParseResult::TransportError;	// ビット誤りあり
	if ((m_Header.PID >= 0x0002_u16) && (m_Header.PID <= 0x000F_u16))
		return ParseResult::FormatError;	// 未定義PID範囲
	if (m_Header.TransportScramblingControl == 0x01_u8)
		return ParseResult::FormatError;	// 未定義スクランブル制御値
	if (m_Header.AdaptationFieldControl == 0x00_u8)
		return ParseResult::FormatError;	// 未定義アダプテーションフィールド制御値
	if ((m_Header.AdaptationFieldControl == 0x02_u8) && (m_AdaptationField.AdaptationFieldLength > 183_u8))
		return ParseResult::FormatError;	// アダプテーションフィールド長異常
	if ((m_Header.AdaptationFieldControl == 0x03_u8) && (m_AdaptationField.AdaptationFieldLength > 182_u8))
		return ParseResult::FormatError;	// アダプテーションフィールド長異常

	if ((pContinuityCounter != nullptr) && (m_Header.PID != PID_NULL)) {
		// 連続性チェック
		const uint8_t OldCounter = pContinuityCounter[m_Header.PID];
		const uint8_t NewCounter = (m_Header.AdaptationFieldControl & 0x01_u8) ? m_Header.ContinuityCounter : 0x10_u8;
		pContinuityCounter[m_Header.PID] = NewCounter;

		if (!m_AdaptationField.DiscontinuityIndicator) {
			if ((OldCounter < 0x10_u8) && (NewCounter < 0x10_u8)) {
				if (((OldCounter + 1) & 0x0F) != NewCounter) {
					return ParseResult::ContinuityError;
				}
			}
		}
	}

	return ParseResult::OK;
}


void TSPacket::ReparsePacket()
{
	// TSパケットヘッダ解析
	const uint32_t Header = Load32(m_pData);
//	m_Header.SyncByte                   = static_cast<uint8_t>(Header >> 24);
//	m_Header.TransportErrorIndicator    = (Header & 0x800000_u32) != 0;
	m_Header.PayloadUnitStartIndicator  = (Header & 0x400000_u32) != 0;
	m_Header.TransportPriority          = (Header & 0x200000_u32) != 0;
	m_Header.PID                        = static_cast<uint16_t>((Header >> 8) & 0x1FFF);
	m_Header.TransportScramblingControl = static_cast<uint8_t>((Header >> 6) & 0x03);
	m_Header.AdaptationFieldControl     = static_cast<uint8_t>((Header >> 4) & 0x03);
	m_Header.ContinuityCounter          = static_cast<uint8_t>(Header & 0x0F);

	// アダプテーションフィールド解析
	m_AdaptationField = AdaptationFieldHeader();

	if (m_Header.AdaptationFieldControl & 0x02) {
		// アダプテーションフィールドあり
		m_AdaptationField.AdaptationFieldLength = m_pData[4];
		if (m_AdaptationField.AdaptationFieldLength > 0) {
			// フィールド長以降あり
			m_AdaptationField.Flags = m_pData[5];
			m_AdaptationField.DiscontinuityIndicator = (m_AdaptationField.Flags & AdaptationFieldFlag::DiscontinuityIndicator) != 0;

			if (m_AdaptationField.AdaptationFieldLength > 1) {
				m_AdaptationField.OptionSize = m_AdaptationField.AdaptationFieldLength - 1;
			}
		}
	}
}


uint8_t * TSPacket::GetPayloadData()
{
	switch (m_Header.AdaptationFieldControl) {
	case 1:	// ペイロードのみ
		return &m_pData[4];

	case 3:	// アダプテーションフィールド、ペイロードあり
		return &m_pData[m_AdaptationField.AdaptationFieldLength + 5];
	}

	// アダプテーションフィールドのみ or 例外
	return nullptr;
}


const uint8_t * TSPacket::GetPayloadData() const
{
	switch (m_Header.AdaptationFieldControl) {
	case 1:	// ペイロードのみ
		return &m_pData[4];

	case 3:	// アダプテーションフィールド、ペイロードあり
		return &m_pData[m_AdaptationField.AdaptationFieldLength + 5];
	}

	// アダプテーションフィールドのみ or 例外
	return nullptr;
}


uint8_t TSPacket::GetPayloadSize() const
{
	switch (m_Header.AdaptationFieldControl) {
	case 1:	// ペイロードのみ
		return (uint8_t)(TS_PACKET_SIZE - 4);

	case 3:	// アダプテーションフィールド、ペイロードあり
		return (uint8_t)(TS_PACKET_SIZE - m_AdaptationField.AdaptationFieldLength - 5);
	}

	// アダプテーションフィールドのみ or 例外
	return 0;
}


void TSPacket::SetPID(uint16_t PID)
{
	Store16(&m_pData[1], ((m_pData[1] & 0xE0) << 8) | (PID & 0x1FFF));
	m_Header.PID = PID;
}


void * TSPacket::Allocate(size_t Size)
{
#ifdef LIBISDB_TS_PACKET_PAYLOAD_ALIGN
	if (Size <= TS_PACKET_SIZE) {
		return reinterpret_cast<void *>(
			((reinterpret_cast<uintptr_t>(m_Data) + (4 + LIBISDB_TS_PACKET_PAYLOAD_ALIGN - 1)) &
			 ~reinterpret_cast<uintptr_t>(LIBISDB_TS_PACKET_PAYLOAD_ALIGN - 1)) - 4);
	}

	return AlignedAlloc(Size, LIBISDB_TS_PACKET_PAYLOAD_ALIGN, 4);
#else
	if (Size <= TS_PACKET_SIZE)
		return m_Data;

	return std::malloc(Size);
#endif
}


void TSPacket::Free(void *pBuffer)
{
	if (!((pBuffer >= m_Data) && (pBuffer < m_Data + sizeof(m_Data)))) {
#ifdef LIBISDB_TS_PACKET_PAYLOAD_ALIGN
		AlignedFree(pBuffer);
#else
		std::free(pBuffer);
#endif
	}
}


void * TSPacket::ReAllocate(void *pBuffer, size_t Size)
{
	if ((pBuffer >= m_Data) && (pBuffer < m_Data + sizeof(m_Data))) {
		const size_t Offset = static_cast<uint8_t *>(pBuffer) - m_Data;

		if (Size <= sizeof(m_Data) - Offset)
			return pBuffer;

		void *pNewBuffer =
#ifdef LIBISDB_TS_PACKET_PAYLOAD_ALIGN
			AlignedAlloc(Size, LIBISDB_TS_PACKET_PAYLOAD_ALIGN, 4);
#else
			std::malloc(Size);
#endif
		if (pNewBuffer != nullptr)
			std::memcpy(pNewBuffer, pBuffer, std::min(Size, sizeof(m_Data) - Offset));

		return pNewBuffer;
	}

#ifdef LIBISDB_TS_PACKET_PAYLOAD_ALIGN
	return AlignedRealloc(pBuffer, Size, LIBISDB_TS_PACKET_PAYLOAD_ALIGN, 4);
#else
	return std::realloc(pBuffer, Size);
#endif
}


}	//namespace LibISDB
