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
 @file   H265Parser.cpp
 @brief  H.265 解析
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "H265Parser.hpp"
#include "../Base/BitstreamReader.hpp"
#include <algorithm>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


H265AccessUnit::H265AccessUnit()
{
	Reset();
}


bool H265AccessUnit::ParseHeader()
{
	if ((m_DataSize < 6)
			|| (m_pData[0] != 0x00) || (m_pData[1] != 0x00) || (m_pData[2] != 0x01))
		return false;

	size_t Pos = 3;

	for (;;) {
		uint32_t SyncState = 0xFFFFFFFF_u32;
		size_t NextPos = Pos + 1;
		bool FoundStartCode = false;
		for (; NextPos < m_DataSize - 3; NextPos++) {
			SyncState = (SyncState << 8) | m_pData[NextPos];
			if ((SyncState & 0x00FFFFFF_u32) == 0x00000001_u32) {
				FoundStartCode = true;
				NextPos++;
				break;
			}
		}
		if (!FoundStartCode)
			break;

		if (m_pData[Pos] & 0x80)	// forbidden_zero_bit	f(1)
			break;
		const uint8_t NALUnitType = (m_pData[Pos] & 0x7E) >> 1;	// nal_unit_type	u(6)
		// nuh_layer_id	u(6)
		// nuh_temporal_id_plus1	u(3)
		Pos += 2;
		size_t NALUnitSize = NextPos - 3 - Pos;

		NALUnitSize = EBSPToRBSP(&m_pData[Pos], NALUnitSize);
		if (NALUnitSize == (size_t)-1)
			break;

		if (NALUnitType == 0x21) {
			// Sequence parameter set
			BitstreamReader Bitstream(&m_pData[Pos], NALUnitSize);

			m_Header.SPS.SPSVideoParameterSetID = static_cast<uint8_t>(Bitstream.GetBits(4));
			m_Header.SPS.SPSMaxSubLayersMinus1 = static_cast<uint8_t>(Bitstream.GetBits(3));
			m_Header.SPS.SPSTemporalIDNestingFlag = Bitstream.GetFlag();
			// profile_tier_level
			m_Header.SPS.PTL.GeneralProfileSpace = static_cast<uint8_t>(Bitstream.GetBits(2));
			m_Header.SPS.PTL.GeneralTierFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.GeneralProfileIDC = static_cast<uint8_t>(Bitstream.GetBits(5));
			for (int i = 0; i < 32; i++)
				m_Header.SPS.PTL.GeneralProfileCompatibilityFlag[i] = Bitstream.GetFlag();
			m_Header.SPS.PTL.GeneralProgressiveSourceFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.GeneralInterlacedSourceFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.GeneralNonPackedConstraintFlag = Bitstream.GetFlag();
			m_Header.SPS.PTL.GeneralFrameOnlyConstraintFlag = Bitstream.GetFlag();
			Bitstream.Skip(44);	// general_reserved_zero_44bits
			m_Header.SPS.PTL.GeneralLevelIDC = static_cast<uint8_t>(Bitstream.GetBits(8));
			for (int i = 0; i < m_Header.SPS.SPSMaxSubLayersMinus1; i++) {
				m_Header.SPS.PTL.SubLayer[i].SubLayerProfilePresentFlag = Bitstream.GetFlag();
				m_Header.SPS.PTL.SubLayer[i].SubLayerLevelPresentFlag = Bitstream.GetFlag();
			}
			if (m_Header.SPS.SPSMaxSubLayersMinus1 > 0)
				Bitstream.Skip((8 - m_Header.SPS.SPSMaxSubLayersMinus1) * 2);	// reserved_zero_2bits
			for (int i = 0; i < m_Header.SPS.SPSMaxSubLayersMinus1; i++) {
				if (m_Header.SPS.PTL.SubLayer[i].SubLayerProfilePresentFlag) {
					m_Header.SPS.PTL.SubLayer[i].SubLayerProfileSpace = static_cast<uint8_t>(Bitstream.GetBits(2));
					m_Header.SPS.PTL.SubLayer[i].SubLayerTierFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].SubLayerProfileIDC = static_cast<uint8_t>(Bitstream.GetBits(5));
					for (int j = 0; j < 32; j++)
						m_Header.SPS.PTL.SubLayer[i].SubLayerProfileCompatibilityFlag[j] = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].SubLayerProgressiveSourceFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].SubLayerInterlacedSourceFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].SubLayerNonPackedConstraintFlag = Bitstream.GetFlag();
					m_Header.SPS.PTL.SubLayer[i].SubLayerFrameOnlyConstraintFlag = Bitstream.GetFlag();
					Bitstream.Skip(44);	// sub_layer_reserved_zero_44bits
				}
				if (m_Header.SPS.PTL.SubLayer[i].SubLayerLevelPresentFlag) {
					m_Header.SPS.PTL.SubLayer[i].SubLayerLevelIDC = static_cast<uint8_t>(Bitstream.GetBits(8));
				}
			}
			m_Header.SPS.SPSSeqParameterSetID = Bitstream.GetUE_V();
			m_Header.SPS.ChromaFormatIDC = Bitstream.GetUE_V();
			if (m_Header.SPS.ChromaFormatIDC == 3)
				m_Header.SPS.SeparateColourPlaneFlag = Bitstream.GetFlag();
			m_Header.SPS.PicWidthInLumaSamples = Bitstream.GetUE_V();
			m_Header.SPS.PicHeightInLumaSamples = Bitstream.GetUE_V();
			m_Header.SPS.ConformanceWindowFlag = Bitstream.GetFlag();
			if (m_Header.SPS.ConformanceWindowFlag) {
				m_Header.SPS.ConfWinLeftOffset = Bitstream.GetUE_V();
				m_Header.SPS.ConfWinRightOffset = Bitstream.GetUE_V();
				m_Header.SPS.ConfWinTopOffset = Bitstream.GetUE_V();
				m_Header.SPS.ConfWinBottomOffset = Bitstream.GetUE_V();
			}
			m_Header.SPS.BitDepthLumaMinus8 = Bitstream.GetUE_V();
			m_Header.SPS.BitDepthChromaMinus8 = Bitstream.GetUE_V();
			m_Header.SPS.Log2MaxPicOrderCntLsbMinus4 = Bitstream.GetUE_V();
			m_Header.SPS.SPSSubLayerOrderingInfoPresentFlag = Bitstream.GetFlag();
			for (int i = m_Header.SPS.SPSSubLayerOrderingInfoPresentFlag ? 0 : m_Header.SPS.SPSMaxSubLayersMinus1;
					i <= m_Header.SPS.SPSMaxSubLayersMinus1; i++) {
				m_Header.SPS.SubLayerOrderingInfo[i].SPSMaxDecPicBufferingMinus1 = Bitstream.GetUE_V();
				m_Header.SPS.SubLayerOrderingInfo[i].SPSMaxNumReorderPics = Bitstream.GetUE_V();
				m_Header.SPS.SubLayerOrderingInfo[i].SPSMaxLatencyIncreasePlus1 = Bitstream.GetUE_V();
			}
			m_Header.SPS.Log2MinLumaCodingBlockSizeMinus3 = Bitstream.GetUE_V();
			m_Header.SPS.Log2DiffMaxMinLumaCodingBlockSize = Bitstream.GetUE_V();
			m_Header.SPS.Log2MinTransformBlockSizeMinus2 = Bitstream.GetUE_V();
			m_Header.SPS.Log2DiffMaxMinTransformBlockSize = Bitstream.GetUE_V();
			m_Header.SPS.MaxTransformHierarchyDepthInter = Bitstream.GetUE_V();
			m_Header.SPS.MaxTransformHierarchyDepthIntra = Bitstream.GetUE_V();
			m_Header.SPS.ScalingListEnabledFlag = Bitstream.GetFlag();
			if (m_Header.SPS.ScalingListEnabledFlag) {
				m_Header.SPS.SPSScalingListDataPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.SPSScalingListDataPresentFlag) {
					// scaling_list_data
					for (int SizeId = 0; SizeId < 4; SizeId++) {
						for (int MatrixId = 0; MatrixId < ((SizeId == 3) ? 2 : 6); MatrixId++) {
							bool bScalingListPredModeFlag = Bitstream.GetFlag();
							if (!bScalingListPredModeFlag) {
								Bitstream.GetUE_V();	// scaling_list_pred_matrix_id_delta
							} else {
								int NextCoef = 8, CoefNum = std::min(64, 1 << (4 + (SizeId << 1)));
								if (SizeId > 1) {
									int ScalingListDcCoefMinus8 = Bitstream.GetSE_V();
									NextCoef = ScalingListDcCoefMinus8 + 8;
								}
								for (int i = 0; i < CoefNum; i++) {
									int ScalingListDeltaCoef = Bitstream.GetSE_V();
									NextCoef = (NextCoef + ScalingListDeltaCoef + 256) % 256;
								}
							}
						}
					}
				}
			}
			m_Header.SPS.AmpEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.SampleAdaptiveOffsetEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.PCMEnabledFlag = Bitstream.GetFlag();
			if (m_Header.SPS.PCMEnabledFlag) {
				m_Header.SPS.PCMSampleBitDepthLumaMinus1 = static_cast<uint8_t>(Bitstream.GetBits(4));
				m_Header.SPS.PCMSampleBitDepthChromaMinus1 = static_cast<uint8_t>(Bitstream.GetBits(4));
				m_Header.SPS.Log2MinPCMLumaCodingBlockSizeMinus3 = Bitstream.GetUE_V();
				m_Header.SPS.Log2DiffMaxMinPCMLumaCodingBlockSize = Bitstream.GetUE_V();
				m_Header.SPS.PCMLoopFilterDisabledFlag = Bitstream.GetFlag();
			}
			m_Header.SPS.NumShortTermRefPicSets = Bitstream.GetUE_V();
			int NumPics = 0;
			for (int i = 0; i < m_Header.SPS.NumShortTermRefPicSets; i++) {
				// short_term_ref_pic_set
				bool InterRefPicSetPredictionFlag = false;
				if (i != 0)
					InterRefPicSetPredictionFlag = Bitstream.GetFlag();
				if (InterRefPicSetPredictionFlag) {
					Bitstream.GetFlag();	// delta_rps_sign
					Bitstream.GetUE_V();	// abs_delta_rps_minus1
					int NumPicsNew = 0;
					for (int j = 0; j <= NumPics; j++) {
						bool bUsedByCurrPicFlag = Bitstream.GetFlag();
						if (bUsedByCurrPicFlag) {
							NumPicsNew++;
						} else {
							bool bUseDeltaFlag = Bitstream.GetFlag();
							if (bUseDeltaFlag)
								NumPicsNew++;
						}
					}
					NumPics = NumPicsNew;
				} else {
					int NumNegativePics = Bitstream.GetUE_V();
					int NumPositivePics = Bitstream.GetUE_V();
					NumPics = NumNegativePics + NumPositivePics;
					for (int j = 0; j < NumNegativePics; j++) {
						Bitstream.GetUE_V();	// delta_poc_s0_minus1
						Bitstream.GetFlag();	// used_by_curr_pic_s0_flag
					}
					for (int j = 0; j < NumPositivePics; j++) {
						Bitstream.GetUE_V();	// delta_poc_s1_minus1
						Bitstream.GetFlag();	// used_by_curr_pic_s1_flag
					}
				}
			}
			m_Header.SPS.LongTermRefPicsPresentFlag = Bitstream.GetFlag();
			if (m_Header.SPS.LongTermRefPicsPresentFlag) {
				m_Header.SPS.NumLongTermRefPicsSPS = Bitstream.GetUE_V();
				for (int i = 0; i < m_Header.SPS.NumLongTermRefPicsSPS; i++) {
					Bitstream.Skip(m_Header.SPS.Log2MaxPicOrderCntLsbMinus4 + 4);	// lt_ref_pic_poc_lsb_sps
					Bitstream.GetFlag();	// used_by_curr_pic_lt_sps_flag
				}
			}
			m_Header.SPS.SPSTemporalMvpEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.StrongIntraSmoothingEnabledFlag = Bitstream.GetFlag();
			m_Header.SPS.VUIParametersPresentFlag = Bitstream.GetFlag();
			if (m_Header.SPS.VUIParametersPresentFlag) {
				// vui_parameters
				m_Header.SPS.VUI.AspectRatioInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.AspectRatioInfoPresentFlag) {
					m_Header.SPS.VUI.AspectRatioIDC = static_cast<uint8_t>(Bitstream.GetBits(8));
					if (m_Header.SPS.VUI.AspectRatioIDC == 0xFF) {	// EXTENDED_SAR
						m_Header.SPS.VUI.SARWidth = (uint16_t)Bitstream.GetBits(16);
						m_Header.SPS.VUI.SARHeight = (uint16_t)Bitstream.GetBits(16);
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
						m_Header.SPS.VUI.MatrixCoeffs = static_cast<uint8_t>(Bitstream.GetBits(8));
					}
				}
				m_Header.SPS.VUI.ChromaLocInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.ChromaLocInfoPresentFlag) {
					m_Header.SPS.VUI.ChromaSampleLocTypeTopField = Bitstream.GetUE_V();
					m_Header.SPS.VUI.ChromaSampleLocTypeBottomField = Bitstream.GetUE_V();
				}
				m_Header.SPS.VUI.NeutralChromaIndicationFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.FieldSeqFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.FrameFieldInfoPresentFlag = Bitstream.GetFlag();
				m_Header.SPS.VUI.DefaultDisplayWindowFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.DefaultDisplayWindowFlag) {
					m_Header.SPS.VUI.DefDispWinLeftOffset = Bitstream.GetUE_V();
					m_Header.SPS.VUI.DefDispWinRightOffset = Bitstream.GetUE_V();
					m_Header.SPS.VUI.DefDispWinTopOffset = Bitstream.GetUE_V();
					m_Header.SPS.VUI.DefDispWinBottomOffset = Bitstream.GetUE_V();
				}
				m_Header.SPS.VUI.VUITimingInfoPresentFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.VUITimingInfoPresentFlag) {
					m_Header.SPS.VUI.VUINumUnitsInTick = Bitstream.GetBits(32);
					m_Header.SPS.VUI.VUITimeScale = Bitstream.GetBits(32);
					m_Header.SPS.VUI.VUIPocProportionalToTimingFlag = Bitstream.GetFlag();
					if (m_Header.SPS.VUI.VUIPocProportionalToTimingFlag)
						m_Header.SPS.VUI.VUINumTicksPocDiffOneMinus1 = Bitstream.GetUE_V();
					m_Header.SPS.VUI.VUIHRDParametersPresentFlag = Bitstream.GetFlag();
#if 0	// 割愛…
					if (m_Header.SPS.VUI.VUIHRDParametersPresentFlag) {
						// hrd_parameters
					}
#endif
				}
#if 0
				m_Header.SPS.VUI.BitstreamRestrictionFlag = Bitstream.GetFlag();
				if (m_Header.SPS.VUI.BitstreamRestrictionFlag) {
					m_Header.SPS.VUI.TilesFixedStructureFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.MotionVectorsOverPicBoundariesFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.RestrictedRefPicListsFlag = Bitstream.GetFlag();
					m_Header.SPS.VUI.MinSpatialSegmentationIDC = Bitstream.GetUE_V();
					m_Header.SPS.VUI.MaxBytesPerPicDenom = Bitstream.GetUE_V();
					m_Header.SPS.VUI.MaxBitsPerMinCuDenom = Bitstream.GetUE_V();
					m_Header.SPS.VUI.Log2MaxMVLengthHorizontal = Bitstream.GetUE_V();
					m_Header.SPS.VUI.Log2MaxMVLengthVertical = Bitstream.GetUE_V();
				}
#endif
			}

			m_FoundSPS = true;
		} else if (NALUnitType == 0x23) {
			// Access unit delimiter
			m_Header.AUD.PicType = m_pData[Pos] >> 5;
		} else if (NALUnitType == 0x24) {
			// End of sequence
			break;
		}

		Pos = NextPos;
	}

	return m_FoundSPS;
}


void H265AccessUnit::Reset()
{
	m_FoundSPS = false;
	m_Header = Header();
}


uint16_t H265AccessUnit::GetHorizontalSize() const noexcept
{
	uint16_t Width = m_Header.SPS.PicWidthInLumaSamples;
	if (m_Header.SPS.ConformanceWindowFlag) {
		uint16_t Crop = (m_Header.SPS.ConfWinLeftOffset + m_Header.SPS.ConfWinRightOffset) * GetSubWidthC();
		if (Crop < Width)
			Width -= Crop;
	}
	return Width;
}


uint16_t H265AccessUnit::GetVerticalSize() const noexcept
{
	uint16_t Height = m_Header.SPS.PicHeightInLumaSamples;
	if (m_Header.SPS.ConformanceWindowFlag) {
		uint16_t Crop = (m_Header.SPS.ConfWinTopOffset + m_Header.SPS.ConfWinBottomOffset) * GetSubHeightC();
		if (Crop < Height)
			Height -= Crop;
	}
	return Height;
}


bool H265AccessUnit::GetSAR(ReturnArg<uint16_t> Horizontal, ReturnArg<uint16_t> Vertical) const noexcept
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
	if (m_Header.SPS.VUI.AspectRatioIDC < CountOf(SARList)) {
		Horz = SARList[m_Header.SPS.VUI.AspectRatioIDC].Horz;
		Vert = SARList[m_Header.SPS.VUI.AspectRatioIDC].Vert;
	} else if (m_Header.SPS.VUI.AspectRatioIDC == 255) {	// EXTENDED_SAR
		Horz = m_Header.SPS.VUI.SARWidth;
		Vert = m_Header.SPS.VUI.SARHeight;
	} else {
		return false;
	}

	Horizontal = Horz;
	Vertical = Vert;

	return true;
}


bool H265AccessUnit::GetTimingInfo(ReturnArg<TimingInfo> Info) const noexcept
{
	if (!m_Header.SPS.VUIParametersPresentFlag
			|| !m_Header.SPS.VUI.VUITimingInfoPresentFlag)
		return false;

	if (Info) {
		Info->NumUnitsInTick = m_Header.SPS.VUI.VUINumUnitsInTick;
		Info->TimeScale      = m_Header.SPS.VUI.VUITimeScale;
	}

	return true;
}


int H265AccessUnit::GetSubWidthC() const noexcept
{
	return (m_Header.SPS.ChromaFormatIDC == 1
			|| m_Header.SPS.ChromaFormatIDC == 2) ? 2 : 1;
}


int H265AccessUnit::GetSubHeightC() const noexcept
{
	return (m_Header.SPS.ChromaFormatIDC == 1) ? 2 : 1;
}




H265Parser::H265Parser(AccessUnitHandler *pAccessUnitHandler)
	: m_pAccessUnitHandler(pAccessUnitHandler)
{
}


bool H265Parser::StoreES(const uint8_t *pData, size_t Size)
{
	return ParseSequence(pData, Size, 0x00000146_u32, 0xFFFFFFFE_u32, &m_AccessUnit);
}


void H265Parser::Reset()
{
	MPEGVideoParserBase::Reset();

	m_AccessUnit.Reset();
}


void H265Parser::OnSequence(DataBuffer *pSequenceData)
{
	H265AccessUnit *pAccessUnit = static_cast<H265AccessUnit *>(pSequenceData);

	if (pAccessUnit->ParseHeader()) {
		if (m_pAccessUnitHandler != nullptr)
			m_pAccessUnitHandler->OnAccessUnit(this, pAccessUnit);
	}
}


}	// namespace LibISDB
