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
 @file   CaptionParser.cpp
 @brief  字幕解析
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "CaptionParser.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/CRC.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


CaptionParser::CaptionParser(bool OneSeg)
	: m_PESParser(this)
	, m_pHandler(nullptr)
	, m_pDRCSMap(nullptr)
	, m_1Seg(OneSeg)

	, m_DataGroupVersion(0xFF)
	, m_DataGroupID(0x00)
{
}


void CaptionParser::Reset()
{
	m_PESParser.Reset();
	m_LanguageList.clear();
	m_DataGroupVersion = 0xFF;
	m_DataGroupID = 0x00;
}


bool CaptionParser::StorePacket(const TSPacket *pPacket)
{
	return m_PESParser.StorePacket(pPacket);
}


void CaptionParser::SetCaptionHandler(CaptionHandler *pHandler)
{
	m_pHandler = pHandler;
}


void CaptionParser::SetDRCSMap(DRCSMap *pDRCSMap)
{
	m_pDRCSMap = pDRCSMap;
}


int CaptionParser::GetLanguageCount() const
{
	return static_cast<int>(m_LanguageList.size());
}


bool CaptionParser::GetLanguageInfo(int Index, ReturnArg<LanguageInfo> Info) const
{
	if ((static_cast<unsigned int>(Index) >= m_LanguageList.size()) || !Info)
		return false;

	*Info = m_LanguageList[Index];

	return true;
}


int CaptionParser::GetLanguageIndexByTag(uint8_t LanguageTag) const
{
	for (size_t i = 0; i < m_LanguageList.size(); i++) {
		if (m_LanguageList[i].LanguageTag == LanguageTag)
			return static_cast<int>(i);
	}

	return -1;
}


uint32_t CaptionParser::GetLanguageCodeByTag(uint8_t LanguageTag) const
{
	const int Index = GetLanguageIndexByTag(LanguageTag);
	if (Index < 0)
		return 0;

	return m_LanguageList[Index].LanguageCode;
}


void CaptionParser::OnPESPacket(const PESParser *pParser, const PESPacket *pPacket)
{
	const uint8_t *pData = pPacket->GetPayloadData();
	const size_t DataSize = pPacket->GetPayloadSize();

	if (LIBISDB_TRACE_ERROR_IF((pData == nullptr) || (DataSize < 3)))
		return;

	if (LIBISDB_TRACE_ERROR_IF((pData[0] != 0x80) && (pData[0] != 0x81)))	// data_identifier
		return;
	if (LIBISDB_TRACE_ERROR_IF(pData[1] != 0xFF))	// private_stream_id
		return;

	const uint8_t HeaderLength = pData[2] & 0x0F;	// PES_data_packet_header_length
	if (LIBISDB_TRACE_ERROR_IF(3_z + HeaderLength + 5_z >= DataSize))
		return;

	size_t Pos = 3 + HeaderLength;

	// data_group()

	const uint8_t DataGroupID             = pData[Pos] >> 2;         // data_group_id
	const uint8_t DataGroupVersion        = pData[Pos] & 0x03;       // data_group_version
//	const uint8_t DataGroupLinkNumber     = pData[Pos + 1];          // data_group_link_number
//	const uint8_t LastDataGroupLinkNumber = pData[Pos + 2];          // last_data_group_link_number
	const uint16_t DataGroupSize          = Load16(&pData[Pos + 3]); // data_group_size
	if (LIBISDB_TRACE_ERROR_IF(Pos + 5 + DataGroupSize + 2 > DataSize))
		return;
	if (CRC16CCITT::Calc(&pData[Pos], 5 + DataGroupSize + 2) != 0) {
		LIBISDB_TRACE_ERROR(LIBISDB_STR("Caption data_group() CRC_16 error\n"));
		return;
	}
	Pos += 5;

	if (m_DataGroupVersion != DataGroupVersion) {
		m_LanguageList.clear();
		m_DataGroupVersion = DataGroupVersion;
	}

	m_DataGroupID = DataGroupID;

	if ((DataGroupID == 0x00) || (DataGroupID == 0x20)) {
		// 字幕管理データ
		ParseManagementData(&pData[Pos], DataGroupSize);
	} else {
		// 字幕データ
		ParseCaptionData(&pData[Pos], DataGroupSize);
	}
}


bool CaptionParser::ParseManagementData(const uint8_t *pData, uint32_t DataSize)
{
	// caption_management_data()

	if (LIBISDB_TRACE_ERROR_IF(DataSize < 2 + 5 + 3))
		return false;

	uint32_t Pos = 0;

	const uint8_t TMD = pData[Pos++] >> 6;
	if (TMD == 0b10) {
		// OTM
		/*
		TimeInfo OTM;
		OTM.Hour        = GetBCD(pData[Pos + 0]);
		OTM.Minute      = GetBCD(pData[Pos + 1]);
		OTM.Second      = GetBCD(pData[Pos + 2]);
		OTM.Millisecond = GetBCD(&pData[Pos + 3], 3);
		*/
		Pos += 5;
	}

	const uint8_t NumLanguages = pData[Pos++];
	if (Pos + NumLanguages * 5 + 3 > DataSize)
		return false;

	bool Changed = false;

	for (uint8_t i = 0; i < NumLanguages; i++) {
		LanguageInfo LangInfo;

		LangInfo.LanguageTag  = pData[Pos] >> 5;
		LangInfo.DMF          = pData[Pos] & 0x0F;
		if ((LangInfo.DMF == 0b1100) || (LangInfo.DMF == 0b1101) || (LangInfo.DMF == 0b1110)) {
			LangInfo.DC       = pData[Pos + 1];
			Pos++;
		}
		LangInfo.LanguageCode = Load24(&pData[Pos + 1]);
		LangInfo.Format       = pData[Pos + 4] >> 4;
		LangInfo.TCS          = (pData[Pos + 4] & 0x0C) >> 2;
		LangInfo.RollupMode   = pData[Pos + 4] & 0x03;

		int Index = GetLanguageIndexByTag(LangInfo.LanguageTag);
		if (Index < 0) {
			m_LanguageList.push_back(LangInfo);
			Changed  = true;
		} else {
			if (m_LanguageList[Index] != LangInfo) {
				m_LanguageList[Index] = LangInfo;
				Changed  = true;
			}
		}

		Pos += 5;
	}

	if (Changed && (m_pHandler != nullptr))
		m_pHandler->OnLanguageUpdate(this);

	const uint32_t UnitLoopLength = Load24(&pData[Pos]);
	Pos += 3;
	if ((UnitLoopLength > 0) && (Pos + UnitLoopLength <= DataSize)) {
		uint32_t ReadSize = 0;
		do {
			uint32_t Size = UnitLoopLength - ReadSize;
			if (!ParseUnitData(&pData[Pos + ReadSize], &Size))
				return false;
			ReadSize += Size;
		} while (ReadSize < UnitLoopLength);
	}

	return true;
}


bool CaptionParser::ParseCaptionData(const uint8_t *pData, uint32_t DataSize)
{
	// caption_data()

	if (LIBISDB_TRACE_ERROR_IF(DataSize <= 1 + 3))
		return false;

	uint32_t Pos = 0;
	const uint8_t TMD = pData[Pos++] >> 6;
	if ((TMD == 0b01) || (TMD == 0b10)) {
		// STM
		if (LIBISDB_TRACE_ERROR_IF(Pos + 5 + 3 >= DataSize))
			return false;
		/*
		TimeInfo STM;
		STM.Hour        = GetBCD(pData[Pos + 0]);
		STM.Minute      = GetBCD(pData[Pos + 1]);
		STM.Second      = GetBCD(pData[Pos + 2]);
		STM.Millisecond = GetBCD(&pData[Pos + 3], 3);
		*/
		Pos+=5;
	}

	const uint32_t UnitLoopLength = Load24(&pData[Pos]);
	Pos += 3;
	if ((UnitLoopLength > 0) && (Pos + UnitLoopLength <= DataSize)) {
		uint32_t ReadSize = 0;
		do {
			uint32_t Size = UnitLoopLength - ReadSize;
			if (!ParseUnitData(&pData[Pos + ReadSize], &Size))
				return false;
			ReadSize += Size;
		} while (ReadSize < UnitLoopLength);
	}

	return true;
}


bool CaptionParser::ParseUnitData(const uint8_t *pData, uint32_t *pDataSize)
{
	// data_unit()

	if (LIBISDB_TRACE_ERROR_IF((pData == nullptr) || (*pDataSize < 5)))
		return false;
	if (LIBISDB_TRACE_ERROR_IF(pData[0] != 0x1F))	// unit_separator
		return false;

	const uint32_t UnitSize = Load24(&pData[2]);
	if (LIBISDB_TRACE_ERROR_IF(5 + UnitSize > *pDataSize))
		return false;

	const uint8_t DataUnitParameter = pData[1];
	if (((DataUnitParameter == 0x30) || (DataUnitParameter == 0x31)) && (m_pDRCSMap != nullptr)) {
		if (!ParseDRCSUnitData(pData + 5, UnitSize))
			return false;
		*pDataSize = 5 + UnitSize;
		return true;
	}
	if (DataUnitParameter != 0x20) {
		*pDataSize = 5 + UnitSize;
		return true;
	}

	if ((UnitSize > 0) && (m_pHandler != nullptr)) {
		ARIBStringDecoder::DecodeFlag Flags =
			m_1Seg ? ARIBStringDecoder::DecodeFlag::OneSeg : ARIBStringDecoder::DecodeFlag::None;
		ARIBStringDecoder::FormatList FormatList;
		String Text;

		if (m_StringDecoder.DecodeCaption(&pData[5], UnitSize, &Text, Flags, &FormatList, m_pDRCSMap)) {
			OnCaption(Text.c_str(), &FormatList);
		}
	}

	*pDataSize = 5 + UnitSize;

	return true;
}


bool CaptionParser::ParseDRCSUnitData(const uint8_t *pData, uint32_t DataSize)
{
	uint32_t RemainSize = DataSize;

	if (LIBISDB_TRACE_ERROR_IF(RemainSize < 1))
		return false;

	const int NumberOfCode = pData[0];
	pData++;
	RemainSize--;

	for (int i = 0; i < NumberOfCode; i++) {
		if (LIBISDB_TRACE_ERROR_IF(RemainSize < 3))
			return false;

		const uint16_t CharacterCode = Load16(&pData[0]);
		const int NumberOfFont = pData[2];
		pData += 3;
		RemainSize -= 3;

		for (int j = 0; j < NumberOfFont; j++) {
			if (LIBISDB_TRACE_ERROR_IF(RemainSize < 1))
				return false;

		//	const uint8_t FontID = pData[0] >> 4;
			const uint8_t Mode   = pData[0] & 0x0F;
			pData++;
			RemainSize--;

			if (Mode <= 0x0001) {
				if (LIBISDB_TRACE_ERROR_IF(RemainSize < 3))
					return false;

				const uint8_t Depth  = pData[0];
				const uint8_t Width  = pData[1];
				const uint8_t Height = pData[2];
				if (LIBISDB_TRACE_ERROR_IF((Width == 0) || (Height == 0)))
					return false;

				pData += 3;
				RemainSize -= 3;

				uint8_t BitsPerPixel;

				if (Mode == 0x0000) {
					BitsPerPixel = 1;
				} else {
					if (Depth == 0)
						BitsPerPixel = 1;
					else if (Depth <= 2)
						BitsPerPixel = 2;
					else if (Depth <= 6)
						BitsPerPixel = 3;
					else if (Depth <= 14)
						BitsPerPixel = 4;
					else if (Depth <= 30)
						BitsPerPixel = 5;
					else if (Depth <= 62)
						BitsPerPixel = 6;
					else if (Depth <= 126)
						BitsPerPixel = 7;
					else if (Depth <= 254)
						BitsPerPixel = 8;
					else
						BitsPerPixel = 9;
				}

				const uint32_t DataSize = (Width * Height * BitsPerPixel + 7) >> 3;
				if (LIBISDB_TRACE_ERROR_IF(RemainSize < DataSize))
					return false;

				if (j == 0) {
					DRCSBitmap Bitmap;

					Bitmap.Width        = Width;
					Bitmap.Height       = Height;
					Bitmap.Depth        = Depth;
					Bitmap.BitsPerPixel = BitsPerPixel;
					Bitmap.pData        = pData;
					Bitmap.DataSize     = DataSize;
					m_pDRCSMap->SetDRCS(CharacterCode, &Bitmap);
				}

				pData += DataSize;
				RemainSize -= DataSize;
			} else {
				// ジオメトリック(非対応)
				if (LIBISDB_TRACE_ERROR_IF(RemainSize < 4))
					return false;

				//const uint8_t RegionX = pData[0];
				//const uint8_t RegionY = pData[1];
				const uint16_t GeometricDataLength = Load16(&pData[2]);
				pData += 4;
				RemainSize -= 4;
				if (LIBISDB_TRACE_ERROR_IF(RemainSize < GeometricDataLength))
					return false;
				pData += GeometricDataLength;
				RemainSize -= GeometricDataLength;
			}
		}
	}

	return true;
}


void CaptionParser::OnCaption(const CharType *pText, const ARIBStringDecoder::FormatList *pFormatList)
{
	if (m_pHandler != nullptr)
		m_pHandler->OnCaption(this, (m_DataGroupID & 0x0F) - 1, pText, pFormatList);
}


}	// namespace LibISDB
