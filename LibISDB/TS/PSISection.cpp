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
 @file   PSISection.cpp
 @brief  PSI セクション
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "PSISection.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/CRC.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


PSISection::PSISection() noexcept
	: m_Header()
{
}


PSISection::PSISection(size_t BufferSize)
	: DataBuffer(BufferSize)
	, m_Header()
{
}


bool PSISection::operator == (const PSISection &rhs) const noexcept
{
	if (m_Header != rhs.m_Header)
		return false;

	if (GetPayloadSize() != rhs.GetPayloadSize())
		return false;

	const uint8_t *pData1 = GetPayloadData();
	const uint8_t *pData2 = rhs.GetPayloadData();

	if ((pData1 == nullptr) && (pData2 == nullptr))
		return true;
	if ((pData1 == nullptr) || (pData2 == nullptr))
		return false;

	return std::memcmp(pData1, pData2, GetPayloadSize()) == 0;
}


bool PSISection::ParseHeader(bool IsExtended, bool IgnoreSectionNumber)
{
	const size_t HeaderSize = (IsExtended ? 8 : 3);

	if (m_DataSize < HeaderSize)
		return false;

	m_Header.TableID                = m_pData[0];
	m_Header.SectionSyntaxIndicator = (m_pData[1] & 0x80) != 0;
	m_Header.PrivateIndicator       = (m_pData[1] & 0x40) != 0;
	m_Header.SectionLength          = static_cast<uint16_t>(((m_pData[1] & 0x0F) << 8) | m_pData[2]);

	if (m_Header.SectionSyntaxIndicator && IsExtended) {
		m_Header.TableIDExtension     = Load16(&m_pData[3]);
		m_Header.VersionNumber        = (m_pData[5] & 0x3E) >> 1;
		m_Header.CurrentNextIndicator = (m_pData[5] & 0x01) != 0;
		m_Header.SectionNumber        = m_pData[6];
		m_Header.LastSectionNumber    = m_pData[7];
	}

	if (m_Header.TableID == 0xFF_u8)
		return false;
	if ((m_pData[1] & 0x30) != 0x30)
		return false;
	if (m_Header.SectionLength > 4093)
		return false;
	if (m_Header.SectionSyntaxIndicator != IsExtended)
		return false;

	if (m_Header.SectionSyntaxIndicator) {
		if ((m_pData[5] & 0xC0) != 0xC0)
			return false;

		if (!IgnoreSectionNumber
				&& (m_Header.SectionNumber > m_Header.LastSectionNumber)) {
			LIBISDB_TRACE_WARNING(
				LIBISDB_STR("PSISection : Invalid section_number %u / %u | table_id %02X\n"),
				m_Header.SectionNumber, m_Header.LastSectionNumber, m_Header.TableID);
			return false;
		}

		if (m_Header.SectionLength < 9) {
			LIBISDB_TRACE_WARNING(
				LIBISDB_STR("PSISection : Invalid section_length %u | table_id %02X\n"),
				m_Header.SectionLength, m_Header.TableID);
			return false;
		}
	}

	return true;
}


void PSISection::Reset()
{
	ClearSize();
	m_Header = PSIHeader();
}


const uint8_t * PSISection::GetPayloadData() const
{
	const size_t HeaderSize = (m_Header.SectionSyntaxIndicator ? 8 : 3);

	if (m_DataSize < HeaderSize)
		return nullptr;

	return &m_pData[HeaderSize];
}


uint16_t PSISection::GetPayloadSize() const
{
	const size_t HeaderSize = (m_Header.SectionSyntaxIndicator ? 8 : 3);

	if (m_DataSize <= HeaderSize)
		return 0;

	if (m_DataSize < 3_z + m_Header.SectionLength)
		return static_cast<uint16_t>(m_DataSize - HeaderSize);

	if (m_Header.SectionSyntaxIndicator) {
		// 拡張セクション
		if (m_Header.SectionLength < 9)
			return 0;
		return m_Header.SectionLength - 9;
	} else {
		// 標準セクション
		return m_Header.SectionLength;
	}
}


bool PSISection::PSIHeader::operator == (const PSIHeader &rhs) const noexcept
{
	return (TableID == rhs.TableID)
		&& (SectionSyntaxIndicator == rhs.SectionSyntaxIndicator)
		&& (PrivateIndicator == rhs.PrivateIndicator)
		&& (SectionLength == rhs.SectionLength)
		&& (!SectionSyntaxIndicator
			|| ((TableIDExtension == rhs.TableIDExtension)
				&& (VersionNumber == rhs.VersionNumber)
				&& (CurrentNextIndicator == rhs.CurrentNextIndicator)
				&& (SectionNumber == rhs.SectionNumber)
				&& (LastSectionNumber == rhs.LastSectionNumber)));
}




PSISectionParser::PSISectionParser(
	PSISectionHandler *pPSISectionHandler, bool IsExtended, bool IgnoreSectionNumber)
	: m_pPSISectionHandler(pPSISectionHandler)
	, m_PSISection(3 + 4093)
	, m_IsExtended(IsExtended)
	, m_IgnoreSectionNumber(IgnoreSectionNumber)
	, m_IsPayloadStoring(false)
	, m_StoreSize(0)
	, m_CRCErrorCount(0)
{
}


void PSISectionParser::StorePacket(const TSPacket *pPacket)
{
	const uint8_t *pData = pPacket->GetPayloadData();
	const uint8_t PayloadSize = pPacket->GetPayloadSize();
	if ((PayloadSize == 0) || (pData == nullptr))
		return;

	uint8_t Pos, Size;

	if (pPacket->GetPayloadUnitStartIndicator()) {
		// [ヘッダ断片 | ペイロード断片] + [スタッフィングバイト] + ヘッダ先頭 + [ヘッダ断片] + [ペイロード断片] + [スタッフィングバイト]
		const uint8_t UnitStartPos = pData[0] + 1;
		if (UnitStartPos >= PayloadSize)
			return;

		if (UnitStartPos > 1) {
			Pos = 1;
			Size = UnitStartPos - Pos;
			if (m_IsPayloadStoring) {
				StorePayload(&pData[Pos], &Size);
			} else if ((m_PSISection.GetSize() > 0)
					&& StoreHeader(&pData[Pos], &Size)) {
				Pos += Size;
				Size = UnitStartPos - Pos;
				StorePayload(&pData[Pos], &Size);
			}
		}

		m_PSISection.Reset();
		m_IsPayloadStoring = false;

		Pos = UnitStartPos;
		while (Pos < PayloadSize) {
			Size = PayloadSize - Pos;
			if (!m_IsPayloadStoring) {
				if (!StoreHeader(&pData[Pos], &Size))
					break;
				Pos += Size;
				Size = PayloadSize - Pos;
			}
			if (!StorePayload(&pData[Pos], &Size))
				break;
			Pos += Size;
			if ((Pos >= PayloadSize) || (pData[Pos] == 0xFF))
				break;
		}
	} else {
		// [ヘッダ断片] + ペイロード + [スタッフィングバイト]
		Pos = 0;
		Size = PayloadSize;
		if (!m_IsPayloadStoring) {
			if ((m_PSISection.GetSize() == 0)
					|| !StoreHeader(&pData[Pos], &Size))
				return;
			Pos += Size;
			Size = PayloadSize - Pos;
		}
		StorePayload(&pData[Pos], &Size);
	}
}


void PSISectionParser::Reset()
{
	m_IsPayloadStoring = false;
	m_StoreSize = 0;
	m_CRCErrorCount = 0;
	m_PSISection.Reset();
}


void PSISectionParser::SetPSISectionHandler(PSISectionHandler *pSectionHandler)
{
	m_pPSISectionHandler = pSectionHandler;
}


unsigned long PSISectionParser::GetCRCErrorCount() const
{
	return m_CRCErrorCount;
}


bool PSISectionParser::StoreHeader(const uint8_t *pData, uint8_t *pRemain)
{
	if (m_IsPayloadStoring) {
		*pRemain = 0;
		return false;
	}

	const uint8_t HeaderSize = (m_IsExtended ? 8 : 3);
	const uint8_t HeaderRemain = HeaderSize - (uint8_t)m_PSISection.GetSize();

	if (HeaderRemain > *pRemain) {
		// ヘッダの途中までしかまだデータが無い
		m_PSISection.AddData(pData, *pRemain);
		return false;
	}

	m_PSISection.AddData(pData, HeaderRemain);
	*pRemain = HeaderRemain;
	if (m_PSISection.ParseHeader(m_IsExtended, m_IgnoreSectionNumber)) {
		m_StoreSize = 3 + m_PSISection.GetSectionLength();
		m_IsPayloadStoring = true;
		return true;
	} else {
		LIBISDB_TRACE_WARNING(LIBISDB_STR("PSISection header format error\n"));
		m_PSISection.Reset();
		return false;
	}
}


bool PSISectionParser::StorePayload(const uint8_t *pData, uint8_t *pRemain)
{
	if (!m_IsPayloadStoring) {
		*pRemain = 0;
		return false;
	}

	const uint8_t Remain = *pRemain;
	const uint16_t StoreRemain = m_StoreSize - static_cast<uint16_t>(m_PSISection.GetSize());

	if (StoreRemain > Remain) {
		// ペイロードの途中までしかまだデータが無い
		m_PSISection.AddData(pData, Remain);
		return false;
	}

	m_PSISection.AddData(pData, StoreRemain);

	// CRC チェック
	if (CRC32MPEG2::Calc(m_PSISection.GetData(), m_PSISection.GetSize()) == 0) {
		if (m_pPSISectionHandler != nullptr)
			m_pPSISectionHandler->OnPSISection(this, &m_PSISection);
		LIBISDB_TRACE_VERBOSE(
			LIBISDB_STR("PSISection Stored: table_id %02X | %zu / %u bytes\n"),
			m_PSISection.GetTableID(), m_PSISection.GetSize(), m_StoreSize);
	} else {
		if (m_CRCErrorCount < std::numeric_limits<unsigned long>::max())
			m_CRCErrorCount++;
		LIBISDB_TRACE_WARNING(
			LIBISDB_STR("PSISection CRC Error: table_id %02X | %zu / %u bytes\n"),
			m_PSISection.GetTableID(), m_PSISection.GetSize(), m_StoreSize);
	}

	m_PSISection.Reset();
	m_IsPayloadStoring = false;

	*pRemain = static_cast<uint8_t>(StoreRemain);

	return true;
}


}	// namespace LibISDB
