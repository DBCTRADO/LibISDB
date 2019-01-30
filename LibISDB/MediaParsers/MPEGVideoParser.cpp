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
 @file   MPEGVideoParser.cpp
 @brief  MPEG 系映像解析
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "MPEGVideoParser.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


MPEGVideoParserBase::MPEGVideoParserBase()
	: m_SyncState(0xFFFFFFFF_u32)
{
}


bool MPEGVideoParserBase::StorePacket(const PESPacket *pPacket)
{
	if (pPacket == nullptr)
		return false;

	return StoreES(pPacket->GetPayloadData(), pPacket->GetPayloadSize());
}


void MPEGVideoParserBase::Reset()
{
	m_SyncState = 0xFFFFFFFF_u32;
}


void MPEGVideoParserBase::OnPESPacket(const PESParser *pParser, const PESPacket *pPacket)
{
	StorePacket(pPacket);
}


bool MPEGVideoParserBase::ParseSequence(
	const uint8_t *pData, size_t Size, uint32_t StartCode, uint32_t StartCodeMask, DataBuffer *pSequenceData)
{
	bool Sequence = false;
	uint32_t SyncState = m_SyncState;
	size_t Start;

	for (size_t Pos = 0; Pos < Size; Pos += Start) {
		// スタートコードを検索する
		const size_t Remain = Size - Pos;

		if (StartCodeMask == 0xFFFFFFFF_u32) {
			for (Start = 0; Start < Remain; Start++) {
				SyncState = (SyncState << 8) | pData[Start + Pos];
				if (SyncState == StartCode) {
					// スタートコード発見
					break;
				}
			}
		} else {
			for (Start = 0; Start < Remain; Start++) {
				SyncState = (SyncState << 8) | pData[Start + Pos];
				if ((SyncState & StartCodeMask) == StartCode) {
					// スタートコード発見
					break;
				}
			}
		}

		if (Start < Remain) {
			Start++;
			if (pSequenceData->GetSize() >= 4) {
				if (Start > 4) {
					pSequenceData->AddData(&pData[Pos], Start - 4);
				} else if (Start < 4) {
					// スタートコードの断片を取り除く
					pSequenceData->TrimTail(4 - Start);
				}

				// シーケンスを出力する
				OnSequence(pSequenceData);
			}

			// スタートコードをセットする
			uint8_t StartCode[4];
			StartCode[0] = static_cast<uint8_t>( SyncState >> 24);
			StartCode[1] = static_cast<uint8_t>((SyncState >> 16) & 0xFF);
			StartCode[2] = static_cast<uint8_t>((SyncState >>  8) & 0xFF);
			StartCode[3] = static_cast<uint8_t>( SyncState        & 0xFF);
			pSequenceData->SetData(StartCode, 4);

			// シフトレジスタを初期化する
			SyncState = 0xFFFFFFFF_u32;
			Sequence = true;
		} else {
			if (pSequenceData->GetSize() >= 4) {
				// シーケンスストア
				if (pSequenceData->AddData(&pData[Pos], Remain) >= 0x1000000_z) {
					// 例外(シーケンスが16MBを超える)
					pSequenceData->ClearSize();
				}
			}
			break;
		}
	}

	m_SyncState = SyncState;

	return Sequence;
}




size_t EBSPToRBSP(uint8_t *pData, size_t DataSize)
{
	size_t ConvertedSize = 0;
	int Count = 0;

	for (size_t i = 0; i < DataSize; i++) {
		if (Count == 2) {
			if (pData[i] < 0x03)
				return static_cast<size_t>(-1);
			if (pData[i] == 0x03) {
				if ((i < DataSize - 1) && (pData[i + 1] > 0x03))
					return static_cast<size_t>(-1);
				if (i == DataSize - 1)
					break;
				i++;
				Count = 0;
			}
		}
		pData[ConvertedSize++] = pData[i];
		if (pData[i] == 0x00)
			Count++;
		else
			Count = 0;
	}

	return ConvertedSize;
}


}	// namespace LibISDB
