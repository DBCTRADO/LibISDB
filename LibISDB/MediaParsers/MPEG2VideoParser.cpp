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
 @file   MPEG2VideoParser.cpp
 @brief  MPEG-2 映像解析
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "MPEG2VideoParser.hpp"
#include <algorithm>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MPEG2Sequence::MPEG2Sequence()
	: m_Header()
{
}


bool MPEG2Sequence::ParseHeader()
{
	// next_start_code()
	size_t HeaderSize = 12;
	if (m_DataSize < HeaderSize)
		return false;
	if ((m_pData[0] != 0x00) || (m_pData[1] != 0x00) || (m_pData[2] != 0x01) || (m_pData[3] != 0xB3))
		return false;

	m_Header = SequenceHeader();

	m_Header.HorizontalSize              = static_cast<uint16_t>((m_pData[4] << 4) | ((m_pData[5] & 0xF0) >> 4));
	m_Header.VerticalSize                = static_cast<uint16_t>(((m_pData[5] & 0x0F) << 8) | m_pData[6]);
	m_Header.AspectRatioInfo             = (m_pData[7] & 0xF0) >> 4;
	m_Header.FrameRateCode               = m_pData[7] & 0x0F;
	m_Header.BitRate                     = (static_cast<uint32_t>(m_pData[8]) << 10) |
	                                       (static_cast<uint32_t>(m_pData[9]) <<  2) |
	                                       (static_cast<uint32_t>(m_pData[10] & 0xC0) >> 6);
	m_Header.MarkerBit                   = (m_pData[10] & 0x20) != 0;
	m_Header.VBVBufferSize               = static_cast<uint16_t>(((m_pData[10] & 0x1F) << 5) | ((m_pData[11] & 0xF8) >> 3));
	m_Header.ConstrainedParametersFlag   = (m_pData[11] & 0x04) != 0;
	m_Header.LoadIntraQuantiserMatrix    = (m_pData[11] & 0x02) != 0;
	m_Header.LoadNonIntraQuantiserMatrix = (m_pData[11] & 0x01) != 0;

	if (m_Header.LoadIntraQuantiserMatrix) {
		HeaderSize += 64;
		if (m_DataSize < HeaderSize)
			return false;
	}
	if (m_Header.LoadNonIntraQuantiserMatrix) {
		HeaderSize += 64;
		if (m_DataSize < HeaderSize)
			return false;
	}

	if ((m_Header.HorizontalSize == 0) || (m_Header.VerticalSize == 0))
		return false;
	if ((m_Header.AspectRatioInfo == 0) || (m_Header.AspectRatioInfo > 4))
		return false;	// アスペクト比が異常
	if ((m_Header.FrameRateCode == 0) || (m_Header.FrameRateCode > 8))
		return false;	// フレームレートが異常
	if (!m_Header.MarkerBit)
		return false;	// マーカービットが異常
	if (m_Header.ConstrainedParametersFlag)
		return false;	// Constrained Parameters Flag が異常

	// 拡張ヘッダを検索する
	uint32_t SyncState = 0xFFFFFFFF_u32;
	for (size_t i = HeaderSize; i < std::min(m_DataSize - 1, 1024_z);) {
		SyncState = (SyncState << 8) | m_pData[i++];
		if (SyncState == 0x000001B5_u32) {
			// 拡張ヘッダ発見
			switch (m_pData[i] >> 4) {
			case 1:
				// シーケンス拡張(48ビット)
				if (i + 6 > m_DataSize)
					break;
				m_Header.Extension.Sequence.ProfileAndLevel = (m_pData[i + 0] & 0x0F) << 4 | (m_pData[i + 1] >> 4);
				m_Header.Extension.Sequence.Progressive     = (m_pData[i + 1] & 0x08) != 0;
				m_Header.Extension.Sequence.ChromaFormat    = (m_pData[i + 1] & 0x06) >> 1;
				m_Header.HorizontalSize |= (((m_pData[i + 1] & 0x01) << 1) | ((m_pData[i + 2] & 0x80) >> 7)) << 12;	// horizontal size extension
				m_Header.VerticalSize   |= ((m_pData[i + 2] & 0x60) >> 5) << 12;	// vertical size extension
				m_Header.BitRate        |= (static_cast<uint32_t>(m_pData[i + 2] & 0x1F) << 7) |
				                           (static_cast<uint32_t>(m_pData[i + 3] >> 1) << 18);	// bit rate extension
				if ((m_pData[i + 3] & 0x01) == 0)	// marker bit
					break;
				m_Header.VBVBufferSize  |= static_cast<uint32_t>(m_pData[i + 4]) << 10;	// vbv buffer size extension
				m_Header.Extension.Sequence.LowDelay      = (m_pData[i + 5] & 0x80) != 0;
				m_Header.Extension.Sequence.FrameRateExtN = (m_pData[i + 5] & 0x60) >> 5;
				m_Header.Extension.Sequence.FrameRateExtD = (m_pData[i + 5] & 0x18) >> 3;
				m_Header.Extension.Sequence.IsValid = true;
				i += 6;
				break;

			case 2:
				// ディスプレイ拡張(40ビット[+24ビット])
				if (i + 5 > m_DataSize)
					break;
				m_Header.Extension.Display.VideoFormat     = (m_pData[i + 0] & 0x0E) >> 1;
				m_Header.Extension.Display.ColorDescrption = (m_pData[i + 0] & 0x01) != 0;
				if (m_Header.Extension.Display.ColorDescrption) {
					if (i + 5 + 3 > m_DataSize)
						break;
					m_Header.Extension.Display.Color.ColorPrimaries          = m_pData[i + 1];
					m_Header.Extension.Display.Color.TransferCharacteristics = m_pData[i + 2];
					m_Header.Extension.Display.Color.MatrixCoefficients      = m_pData[i + 3];
					i += 3;
				}
				if ((m_pData[i + 2] & 0x02) == 0)	// marker bit
					break;
				if ((m_pData[i + 4] & 0x07) != 0)	// marker bit
					break;
				m_Header.Extension.Display.DisplayHorizontalSize =
					static_cast<uint16_t>((m_pData[i + 1] << 6) | ((m_pData[i + 2] & 0xFC) >> 2));
				m_Header.Extension.Display.DisplayVerticalSize   =
					static_cast<uint16_t>(((m_pData[i + 2] & 0x01) << 13) | (m_pData[i + 3] << 5) | ((m_pData[i + 4] & 0xF8) >> 3));
				m_Header.Extension.Display.IsValid = true;
				i += 5;
				break;
			}
		}
	}

	return true;
}


void MPEG2Sequence::Reset()
{
	ClearSize();

	m_Header = SequenceHeader();
}


bool MPEG2Sequence::GetAspectRatio(ReturnArg<uint8_t> AspectX, ReturnArg<uint8_t> AspectY) const noexcept
{
	switch (m_Header.AspectRatioInfo) {
	case 1:
		AspectX = 1;
		AspectY = 1;
		break;
	case 2:
		AspectX = 4;
		AspectY = 3;
		break;
	case 3:
		AspectX = 16;
		AspectY = 9;
		break;
	case 4:
		AspectX = 221;
		AspectY = 100;
		break;
	default:
		return false;
	}

	return true;
}


bool MPEG2Sequence::GetFrameRate(ReturnArg<uint32_t> Num, ReturnArg<uint32_t> Denom) const noexcept
{
	static const struct {
		uint16_t Num, Denom;
	} FrameRateList[] = {
		{24000, 1001},	// 23.976
		{   24,    1},	// 24
		{   25,    1},	// 25
		{30000, 1001},	// 29.97
		{   30,    1},	// 30
		{   50,    1},	// 50
		{60000, 1001},	// 59.94
		{   60,    1},	// 60
	};

	if ((m_Header.FrameRateCode == 0) || (m_Header.FrameRateCode > 8))
		return false;

	Num   = FrameRateList[m_Header.FrameRateCode - 1].Num;
	Denom = FrameRateList[m_Header.FrameRateCode - 1].Denom;

	return true;
}




MPEG2VideoParser::MPEG2VideoParser(SequenceHandler *pSequenceHandler)
	: m_pSequenceHandler(pSequenceHandler)
{
}


bool MPEG2VideoParser::StoreES(const uint8_t *pData, size_t Size)
{
	return ParseSequence(pData, Size, 0x000001B3_u32, 0xFFFFFFFF_u32, &m_MPEG2Sequence);
}


void MPEG2VideoParser::Reset()
{
	MPEGVideoParserBase::Reset();

	m_MPEG2Sequence.Reset();
}


void MPEG2VideoParser::OnSequence(DataBuffer *pSequenceData)
{
	MPEG2Sequence *pSequence = static_cast<MPEG2Sequence *>(pSequenceData);

	if (pSequence->ParseHeader()) {
		if (m_pSequenceHandler != nullptr)
			m_pSequenceHandler->OnMPEG2Sequence(this, pSequence);
	}
}


}	// namespace LibISDB
