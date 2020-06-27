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
 @file   AACDecoder.cpp
 @brief  AAC デコーダ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "AACDecoder.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


AACDecoder::AACDecoder() noexcept
	: m_ADTSParser(nullptr)
	, m_pADTSFrame(nullptr)
	, m_DecodeError(false)
{
}


AACDecoder::~AACDecoder()
{
}


bool AACDecoder::Open()
{
	if (!OpenDecoder())
		return false;

	m_ADTSParser.Reset();
	ClearAudioInfo();

	return true;
}


void AACDecoder::Close()
{
	CloseDecoder();
	m_ADTSParser.Reset();
	m_pADTSFrame = nullptr;
}


bool AACDecoder::Reset()
{
	if (!ResetDecoder())
		return false;

	m_ADTSParser.Reset();
	ClearAudioInfo();
	m_DecodeError = false;

	return true;
}


bool AACDecoder::Decode(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info)
{
	if (!IsOpened())
		return false;

	m_pADTSFrame = nullptr;

	ADTSFrame *pFrame;
	if (!m_ADTSParser.StoreES(pData, pDataSize, &pFrame))
		return false;

	if (!DecodeFrame(pFrame, Info)) {
		m_DecodeError = true;
		return false;
	}

	m_pADTSFrame = pFrame;
	m_DecodeError = false;

	return true;
}


bool AACDecoder::GetSPDIFFrameInfo(ReturnArg<SPDIFFrameInfo> Info) const
{
	if (!Info || (m_pADTSFrame == nullptr))
		return false;

	Info->Pc = 0x0007_u16;	// MPEG-2 AAC ADTS
	Info->FrameSize = m_pADTSFrame->GetFrameLength();
	Info->SamplesPerFrame = 1024;

	return true;
}


int AACDecoder::GetSPDIFBurstPayload(uint8_t *pBuffer, size_t BufferSize) const
{
	if ((pBuffer == nullptr) || (m_pADTSFrame == nullptr))
		return 0;

	if (m_pADTSFrame->GetRawDataBlockNum() != 0) {
		LIBISDB_TRACE(
			LIBISDB_STR("Invalid no_raw_data_blocks_in_frame (%d)\n"),
			m_pADTSFrame->GetRawDataBlockNum());
		return 0;
	}

	const int FrameSize = m_pADTSFrame->GetFrameLength();
	const int DataBurstSize = (FrameSize + 1) & ~1;
	if (BufferSize < static_cast<size_t>(DataBurstSize))
		return 0;

	::_swab(
		reinterpret_cast<char *>(const_cast<uint8_t *>(m_pADTSFrame->GetData())),
		reinterpret_cast<char *>(pBuffer), FrameSize & ~1);
	if (FrameSize & 1) {
		pBuffer[FrameSize - 1] = 0;
		pBuffer[FrameSize] = m_pADTSFrame->GetAt(FrameSize - 1);
	}

	return DataBurstSize;
}


bool AACDecoder::ResetDecoder()
{
	if (!IsOpened())
		return false;

	return OpenDecoder();
}


}	// namespace LibISDB::DirectShow
