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
 @file   H264Parser.cpp
 @brief  H.264 解析
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "H264Parser.hpp"
#include "../Base/BitstreamReader.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


H264AccessUnit::H264AccessUnit() noexcept
{
	Reset();
}


bool H264AccessUnit::ParseHeader()
{
	if ((m_DataSize < 5)
			|| (m_pData[0] != 0x00) || (m_pData[1] != 0x00) || (m_pData[2] != 0x01))
		return false;

	size_t Pos = 3;

	for (;;) {
		uint32_t SyncState = 0xFFFFFFFF_u32;
		size_t NextPos = Pos + 1;
		bool FoundStartCode = false;
		for (; NextPos < m_DataSize - 2; NextPos++) {
			SyncState = (SyncState << 8) | m_pData[NextPos];
			if ((SyncState & 0x00FFFFFF_u32) == 0x00000001_u32) {
				FoundStartCode = true;
				NextPos++;
				break;
			}
		}
		if (!FoundStartCode)
			break;

		const uint8_t NALUnitType = m_pData[Pos++] & 0x1F;
		size_t NALUnitSize = NextPos - 3 - Pos;

		NALUnitSize = EBSPToRBSP(&m_pData[Pos], NALUnitSize);
		if (NALUnitSize == static_cast<size_t>(-1))
			break;

		if (NALUnitType == 0x07) {
			// Sequence parameter set
			BitstreamReader Bitstream(&m_pData[Pos], NALUnitSize);

			m_Header.SPS.ProfileIDC = static_cast<uint8_t>(Bitstream.GetBits(8));
			m_Header.SPS.ConstraintSet0Flag = Bitstream.GetFlag();
			m_Header.SPS.ConstraintSet1Flag = Bitstream.GetFlag();
			m_Header.SPS.ConstraintSet2Flag = Bitstream.GetFlag();
			m_Header.SPS.ConstraintSet3Flag = Bitstream.GetFlag();
			if (Bitstream.GetBits(4) != 0)	// reserved_zero_4bits
				return false;
			m_Header.SPS.LevelIDC = static_cast<uint8_t>(Bitstream.GetBits(8));
			m_Header.SPS.SeqParameterSetID = Bitstream.GetUE_V();
			m_Header.SPS.ChromaFormatIDC = 1;
			m_Header.SPS.SeparateColourPlaneFlag = false;
			m_Header.SPS.BitDepthLumaMinus8 = 0;
			m_Header.SPS.BitDepthChromaMinus8 = 0;
			m_Header.SPS.QpprimeYZeroTransformBypassFlag = false;
			m_Header.SPS.SeqScalingMatrixPresentFlag = false;
			if (m_Header.SPS.ProfileIDC == 100
					|| m_Header.SPS.ProfileIDC == 110
					|| m_Header.SPS.ProfileIDC == 122
					|| m_Header.SPS.ProfileIDC == 244
					|| m_Header.SPS.ProfileIDC == 44
					|| m_Header.SPS.ProfileIDC == 83
					|| m_Header.SPS.ProfileIDC == 86) {
				// High profile
				m_Header.SPS.ChromaFormatIDC = Bitstream.GetUE_V();
				if (m_Header.SPS.ChromaFormatIDC == 3)	// YUY444
					m_Header.SPS.SeparateColourPlaneFlag = Bitstream.GetFlag();
				m_Header.SPS.BitDepthLumaMinus8 = Bitstream.GetUE_V();
				m_Header.SPS.BitDepthChromaMinus8 = Bitstream.GetUE_V();
				m_Header.SPS.QpprimeYZeroTransformBypassFlag = Bitstream.GetFlag();
				m_Header.SPS.SeqScalingMatrixPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.SeqScalingMatrixPresentFlag) {
					const int Length = (m_Header.SPS.ChromaFormatIDC != 3) ? 8 : 12;
					for (int i = 0; i < Length; i++) {
						if (Bitstream.GetFlag()) {	// seq_scaling_list_present_flag
							int LastScale = 8, NextScale = 8;
							for (int j = 0; j < (i < 6 ? 16 : 64); j++) {
								if (NextScale != 0) {
									const int DeltaScale = Bitstream.GetSE_V();
									NextScale = (LastScale + DeltaScale + 256) % 256;
									LastScale = NextScale;
								}
							}
						}
					}
				}
			}
			m_Header.SPS.Log2MaxFrameNumMinus4 = Bitstream.GetUE_V();
			m_Header.SPS.PicOrderCntType = Bitstream.GetUE_V();
			if (m_Header.SPS.PicOrderCntType == 0) {
				m_Header.SPS.Log2MaxPicOrderCntLSBMinus4 = Bitstream.GetUE_V();
			} else if (m_Header.SPS.PicOrderCntType == 1) {
				m_Header.SPS.DeltaPicOrderAlwaysZeroFlag = Bitstream.GetFlag();
				m_Header.SPS.OffsetForNonRefPic = Bitstream.GetSE_V();
				m_Header.SPS.OffsetForTopToBottomField = Bitstream.GetSE_V();
				m_Header.SPS.NumRefFramesInPicOrderCntCycle = Bitstream.GetUE_V();
				for (int i = 0; i < m_Header.SPS.NumRefFramesInPicOrderCntCycle; i++)
					Bitstream.GetSE_V();	// offset_for_ref_frame
			}
			m_Header.SPS.NumRefFrames = Bitstream.GetUE_V();
			m_Header.SPS.GapsInFrameNumValueAllowedFlag = Bitstream.GetFlag();
			m_Header.SPS.PicWidthInMBSMinus1 = Bitstream.GetUE_V();
			m_Header.SPS.PicHeightInMapUnitsMinus1 = Bitstream.GetUE_V();
			m_Header.SPS.FrameMBSOnlyFlag = Bitstream.GetFlag();
			if (!m_Header.SPS.FrameMBSOnlyFlag)
				m_Header.SPS.MBAdaptiveFrameFieldFlag = Bitstream.GetFlag();
			m_Header.SPS.Direct8x8InferenceFlag = Bitstream.GetFlag();
			m_Header.SPS.FrameCroppingFlag = Bitstream.GetFlag();
			if (m_Header.SPS.FrameCroppingFlag) {
				m_Header.SPS.FrameCropLeftOffset = Bitstream.GetUE_V();
				m_Header.SPS.FrameCropRightOffset = Bitstream.GetUE_V();
				m_Header.SPS.FrameCropTopOffset = Bitstream.GetUE_V();
				m_Header.SPS.FrameCropBottomOffset = Bitstream.GetUE_V();
			}
			m_Header.SPS.VUIParametersPresentFlag = Bitstream.GetFlag();
			if (m_Header.SPS.VUIParametersPresentFlag) {
				m_Header.SPS.VUI.AspectRatioInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.AspectRatioInfoPresentFlag) {
					m_Header.SPS.VUI.AspectRatioIDC = static_cast<uint8_t>(Bitstream.GetBits(8));
					if (m_Header.SPS.VUI.AspectRatioIDC == 255) {
						m_Header.SPS.VUI.SARWidth = static_cast<uint16_t>(Bitstream.GetBits(16));
						m_Header.SPS.VUI.SARHeight = static_cast<uint16_t>(Bitstream.GetBits(16));
					}
				}
				m_Header.SPS.VUI.OverscanInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.OverscanInfoPresentFlag)
					m_Header.SPS.VUI.OverscanAppropriateFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.VideoSignalTypePresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.VideoSignalTypePresentFlag) {
					m_Header.SPS.VUI.VideoFormat = static_cast<uint8_t>(Bitstream.GetBits(3));
					m_Header.SPS.VUI.VideoFullRangeFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.ColourDescriptionPresentFlag = Bitstream.GetFlag();
					if (m_Header.SPS.VUI.ColourDescriptionPresentFlag) {
						m_Header.SPS.VUI.ColourPrimaries = static_cast<uint8_t>(Bitstream.GetBits(8));
						m_Header.SPS.VUI.TransferCharacteristics = static_cast<uint8_t>(Bitstream.GetBits(8));
						m_Header.SPS.VUI.MatrixCoefficients = static_cast<uint8_t>(Bitstream.GetBits(8));
					}
				}
				m_Header.SPS.VUI.ChromaLocInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.ChromaLocInfoPresentFlag) {
					m_Header.SPS.VUI.ChromaSampleLocTypeTopField = Bitstream.GetUE_V();
					m_Header.SPS.VUI.ChromaSampleLocTypeBottomField = Bitstream.GetUE_V();
				}
				m_Header.SPS.VUI.TimingInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.TimingInfoPresentFlag) {
					m_Header.SPS.VUI.NumUnitsInTick = Bitstream.GetBits(32);
					m_Header.SPS.VUI.TimeScale = Bitstream.GetBits(32);
					m_Header.SPS.VUI.FixedFrameRateFlag = Bitstream.GetFlag();
				}
				// 以下まだまだあるが省略
			}

			if (m_Header.SPS.SeparateColourPlaneFlag)
				m_Header.SPS.ChromaArrayType = 0;
			else
				m_Header.SPS.ChromaArrayType = m_Header.SPS.ChromaFormatIDC;

#ifdef LIBISDB_H264_STRICT_1SEG
			// ワンセグ規格に合致している?
			if (m_Header.SPS.ProfileIDC != 66
					|| !m_Header.SPS.ConstraintSet0Flag
					|| !m_Header.SPS.ConstraintSet1Flag
					|| !m_Header.SPS.ConstraintSet2Flag
					|| m_Header.SPS.LevelIDC != 12
					|| m_Header.SPS.SeqParameterSetID > 31
					|| m_Header.SPS.Log2MaxFrameNumMinus4 > 12
					|| m_Header.SPS.PicOrderCntType != 2
					|| m_Header.SPS.NumRefFrames == 0
						|| m_Header.SPS.NumRefFrames > 3
					|| m_Header.SPS.GapsInFrameNumValueAllowedFlag
					|| m_Header.SPS.PicWidthInMBSMinus1 != 19
					|| (m_Header.SPS.PicHeightInMapUnitsMinus1 != 11
						&& m_Header.SPS.PicHeightInMapUnitsMinus1 != 14)
					|| !m_Header.SPS.FrameMBSOnlyFlag
					|| !m_Header.SPS.Direct8x8InferenceFlag
					|| (m_Header.SPS.FrameCroppingFlag !=
						(m_Header.SPS.PicHeightInMapUnitsMinus1 == 11))
					|| (m_Header.SPS.FrameCroppingFlag
						&& (m_Header.SPS.FrameCropLeftOffset != 0
							|| m_Header.SPS.FrameCropRightOffset != 0
							|| m_Header.SPS.FrameCropTopOffset != 0
							|| m_Header.SPS.FrameCropBottomOffset != 6))
					|| !m_Header.SPS.VUIParametersPresentFlag
					|| m_Header.SPS.VUI.AspectRatioInfoPresentFlag
					|| m_Header.SPS.VUI.OverscanInfoPresentFlag
					|| m_Header.SPS.VUI.VideoSignalTypePresentFlag
					|| m_Header.SPS.VUI.ChromaLocInfoPresentFlag
					|| !m_Header.SPS.VUI.TimingInfoPresentFlag
					|| m_Header.SPS.VUI.NumUnitsInTick == 0
					|| m_Header.SPS.VUI.NumUnitsInTick % 1001 != 0
					|| (m_Header.SPS.VUI.TimeScale != 24000
						&& m_Header.SPS.VUI.TimeScale != 30000)) {
				return false;
			}
#endif

			m_FoundSPS = true;
		} else if (NALUnitType == 0x09) {
			// Access unit delimiter
			m_Header.AUD.PrimaryPicType = m_pData[Pos] >> 5;
		} else if (NALUnitType == 0x0A) {
			// End of sequence
			break;
		}

		Pos = NextPos;
	}

	return m_FoundSPS;
}


void H264AccessUnit::Reset() noexcept
{
	m_FoundSPS = false;
	m_Header = Header();
}


uint16_t H264AccessUnit::GetHorizontalSize() const noexcept
{
	uint16_t Width = (m_Header.SPS.PicWidthInMBSMinus1 + 1) * 16;
	if (m_Header.SPS.FrameCroppingFlag) {
		uint16_t Crop = m_Header.SPS.FrameCropLeftOffset + m_Header.SPS.FrameCropRightOffset;
		if (m_Header.SPS.ChromaArrayType != 0)
			Crop *= GetSubWidthC();
		if (Crop < Width)
			Width -= Crop;
	}
	return Width;
}


uint16_t H264AccessUnit::GetVerticalSize() const noexcept
{
	uint16_t Height = (m_Header.SPS.PicHeightInMapUnitsMinus1 + 1) * 16;
	if (!m_Header.SPS.FrameMBSOnlyFlag)
		Height *= 2;
	if (m_Header.SPS.FrameCroppingFlag) {
		uint16_t Crop = m_Header.SPS.FrameCropTopOffset + m_Header.SPS.FrameCropBottomOffset;
		if (m_Header.SPS.ChromaArrayType != 0)
			 Crop *= GetSubHeightC();
		if (!m_Header.SPS.FrameMBSOnlyFlag)
			Crop *= 2;
		if (Crop < Height)
			Height -= Crop;
	}
	return Height;
}


bool H264AccessUnit::GetSAR(ReturnArg<uint16_t> Horizontal, ReturnArg<uint16_t> Vertical) const noexcept
{
	static const struct {
		uint8_t Horz, Vert;
	} SARList[] = {
		{  0,   0},
		{  1,   1},
		{ 12,  11},
		{ 10,  11},
		{ 16,  11},
		{ 40,  33},
		{ 24,  11},
		{ 20,  11},
		{ 32,  11},
		{ 80,  33},
		{ 18,  11},
		{ 15,  11},
		{ 64,  33},
		{160,  99},
		{  4,   3},
		{  3,   2},
		{  2,   1},
	};

	if (!m_Header.SPS.VUIParametersPresentFlag
			|| !m_Header.SPS.VUI.AspectRatioInfoPresentFlag)
		return false;

	uint16_t Horz, Vert;
	if (m_Header.SPS.VUI.AspectRatioIDC < std::size(SARList)) {
		Horz = SARList[m_Header.SPS.VUI.AspectRatioIDC].Horz;
		Vert = SARList[m_Header.SPS.VUI.AspectRatioIDC].Vert;
	} else if (m_Header.SPS.VUI.AspectRatioIDC == 255) {	// Extended_SAR
		Horz = m_Header.SPS.VUI.SARWidth;
		Vert = m_Header.SPS.VUI.SARHeight;
	} else {
		return false;
	}

	Horizontal = Horz;
	Vertical = Vert;

	return true;
}


bool H264AccessUnit::GetTimingInfo(ReturnArg<TimingInfo> Info) const noexcept
{
	if (!m_Header.SPS.VUIParametersPresentFlag
			|| !m_Header.SPS.VUI.TimingInfoPresentFlag)
		return false;

	if (Info) {
		Info->NumUnitsInTick     = m_Header.SPS.VUI.NumUnitsInTick;
		Info->TimeScale          = m_Header.SPS.VUI.TimeScale;
		Info->FixedFrameRateFlag = m_Header.SPS.VUI.FixedFrameRateFlag;
	}

	return true;
}


int H264AccessUnit::GetSubWidthC() const noexcept
{
	return (m_Header.SPS.ChromaFormatIDC == 1
			|| m_Header.SPS.ChromaFormatIDC == 2) ? 2 : 1;
}


int H264AccessUnit::GetSubHeightC() const noexcept
{
	return (m_Header.SPS.ChromaFormatIDC == 1) ? 2 : 1;
}




H264Parser::H264Parser(AccessUnitHandler *pAccessUnitHandler)
	: m_pAccessUnitHandler(pAccessUnitHandler)
{
}


bool H264Parser::StoreES(const uint8_t *pData, size_t Size)
{
	return ParseSequence(pData, Size, 0x00000109_u32, 0xFFFFFF1F_u32, &m_AccessUnit);
}


void H264Parser::Reset() noexcept
{
	MPEGVideoParserBase::Reset();

	m_AccessUnit.Reset();
}


void H264Parser::OnSequence(DataBuffer *pSequenceData)
{
	H264AccessUnit *pAccessUnit = static_cast<H264AccessUnit *>(pSequenceData);

	if (pAccessUnit->ParseHeader()) {
		if (m_pAccessUnitHandler != nullptr)
			m_pAccessUnitHandler->OnAccessUnit(this, pAccessUnit);
	}
}


}	// namespace LibISDB
