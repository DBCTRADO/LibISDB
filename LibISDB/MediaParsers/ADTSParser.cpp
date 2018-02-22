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
 @file   ADTSParser.cpp
 @brief  ADTS 解析
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "ADTSParser.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


ADTSFrame::ADTSFrame()
	: m_Header()
{
}


bool ADTSFrame::ParseHeader()
{
	if (m_DataSize < 7)
		return false;
	if ((m_pData[0] != 0xFF) || ((m_pData[1] & 0xF6) != 0xF0))
		return false;	// syncword 及び layer 異常

	// adts_fixed_header()
	m_Header.MPEGVersion       = (m_pData[1] & 0x08) != 0;
	m_Header.ProtectionAbsent  = (m_pData[1] & 0x01) != 0;
	m_Header.Profile           = (m_pData[2] & 0xC0) >> 6;
	m_Header.SamplingFreqIndex = (m_pData[2] & 0x3C) >> 2;
	m_Header.PrivateBit        = (m_pData[2] & 0x02) != 0;
	m_Header.ChannelConfig     = ((m_pData[2] & 0x01) << 2) | ((m_pData[3] & 0xC0) >> 6);
	m_Header.OriginalCopy      = (m_pData[3] & 0x20) != 0;
	m_Header.Home              = (m_pData[3] & 0x10) != 0;

	// adts_variable_header()
	m_Header.CopyrightIDBit    = (m_pData[3] & 0x08U) != 0;
	m_Header.CopyrightIDStart  = (m_pData[3] & 0x04U) != 0;
	m_Header.FrameLength       = static_cast<uint16_t>(((m_pData[3] & 0x03) << 11) | (m_pData[4] << 3) | ((m_pData[5] & 0xE0) >> 5));
	m_Header.BufferFullness    = static_cast<uint16_t>(((m_pData[5] & 0x1F) << 6) | ((m_pData[6] & 0xFC) >> 2));
	m_Header.RawDataBlockNum   = m_pData[6] & 0x03;

	if (m_Header.Profile == 3)
		return false;	// 未定義のプロファイル
	if (m_Header.SamplingFreqIndex > 0x0B)
		return false;	// 未定義のサンプリング周波数
	if ((m_Header.ChannelConfig >= 3) && (m_Header.ChannelConfig != 6))
		return false;	// チャンネル数異常
	if (m_Header.FrameLength < (m_Header.ProtectionAbsent ? 7 : 9))
		return false;	// フレーム長異常
	if (m_Header.RawDataBlockNum != 0)
		return false;	// 複数の AAC フレームは非対応

	return true;
}


void ADTSFrame::Reset()
{
	ClearSize();

	m_Header = ADTSHeader();
}


uint32_t ADTSFrame::GetSamplingFreq() const noexcept
{
	static const uint32_t FreqTable[] = {
		96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000
	};
	return FreqTable[m_Header.SamplingFreqIndex];
}




ADTSParser::ADTSParser(FrameHandler *pFrameHandler)
	: m_pFrameHandler(pFrameHandler)
	, m_IsStoring(false)
{
	// ADTSフレーム最大長のバッファ確保
	m_ADTSFrame.AllocateBuffer(0x2000_z);
}


bool ADTSParser::StorePacket(const PESPacket *pPacket)
{
	if (pPacket == nullptr)
		return false;

	return StoreES(pPacket->GetPayloadData(), pPacket->GetPayloadSize());
}


bool ADTSParser::StoreES(const uint8_t *pData, size_t Size)
{
	if ((pData == nullptr) || (Size == 0))
		return false;

	bool FrameFound = false;
	size_t Pos = 0;

	do {
		if (!m_IsStoring) {
			// ヘッダを検索する
			m_IsStoring = SyncFrame(pData[Pos++]);
			if (m_IsStoring)
				FrameFound = true;
		} else {
			// データをストアする
			const size_t StoreRemain = static_cast<size_t>(m_ADTSFrame.GetFrameLength()) - m_ADTSFrame.GetSize();
			const size_t DataRemain = Size - Pos;

			if (StoreRemain <= DataRemain) {
				// ストア完了
				m_ADTSFrame.AddData(&pData[Pos], StoreRemain);
				Pos += StoreRemain;
				m_IsStoring = false;

				// フレーム出力
				OnADTSFrame(&m_ADTSFrame);

				// 次のフレームを処理するためリセット
				m_ADTSFrame.ClearSize();
			} else {
				// ストア未完了、次のペイロードを待つ
				m_ADTSFrame.AddData(&pData[Pos], DataRemain);
				break;
			}
		}
	} while (Pos < Size);

	return FrameFound;
}


bool ADTSParser::StoreES(const uint8_t *pData, size_t *pSize, ADTSFrame **ppFrame)
{
	if ((pData == nullptr) || (pSize == nullptr) || (*pSize == 0))
		return false;

	if (m_IsStoring) {
		if (m_ADTSFrame.GetSize() >= m_ADTSFrame.GetFrameLength()) {
			m_ADTSFrame.ClearSize();
			m_IsStoring = false;
		}
	}

	bool FrameStored = false;
	const size_t Size = *pSize;
	size_t Pos = 0;

	do {
		if (!m_IsStoring) {
			// ヘッダを検索する
			m_IsStoring = SyncFrame(pData[Pos++]);
		} else {
			// データをストアする
			const size_t StoreRemain = (size_t)m_ADTSFrame.GetFrameLength() - m_ADTSFrame.GetSize();
			const size_t DataRemain = Size - Pos;

			if (StoreRemain <= DataRemain) {
				// ストア完了
				m_ADTSFrame.AddData(&pData[Pos], StoreRemain);
				Pos += StoreRemain;
				if (ppFrame != nullptr)
					*ppFrame = &m_ADTSFrame;
				FrameStored = true;
			} else {
				// ストア未完了、次のペイロードを待つ
				m_ADTSFrame.AddData(&pData[Pos], DataRemain);
				Pos += DataRemain;
			}
			break;
		}
	} while (Pos < Size);

	*pSize = Pos;

	return FrameStored;
}


void ADTSParser::Reset()
{
	m_IsStoring = false;
	m_ADTSFrame.Reset();
}


void ADTSParser::OnPESPacket(const PESParser *pParser, const PESPacket *pPacket)
{
	StorePacket(pPacket);
}


void ADTSParser::OnADTSFrame(const ADTSFrame *pFrame) const
{
	if (m_pFrameHandler != nullptr)
		m_pFrameHandler->OnADTSFrame(this, pFrame);
}


bool ADTSParser::SyncFrame(uint8_t Data)
{
	switch (m_ADTSFrame.GetSize()) {
	case 0:
		// syncword(上位8ビット)
		if (Data == 0xFF)
			m_ADTSFrame.AddByte(Data);
		break;

	case 1:
		// syncword(下位4ビット)、ID、layer、protection_absent
		if ((Data & 0xF6) == 0xF0)
			m_ADTSFrame.AddByte(Data);
		else
			m_ADTSFrame.ClearSize();
		break;

	case 2:
	case 3:
	case 4:
	case 5:
		// ヘッダの途中
		m_ADTSFrame.AddByte(Data);
		break;

	case 6:
		// ヘッダが全てそろった
		m_ADTSFrame.AddByte(Data);

		// ヘッダを解析する
		if (m_ADTSFrame.ParseHeader())
			return true;
		m_ADTSFrame.ClearSize();
		break;

	default:
		// 例外
		m_ADTSFrame.ClearSize();
		break;
	}

	return false;
}


}	// namespace LibISDB
