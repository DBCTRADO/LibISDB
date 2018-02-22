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
 @file   H265Parser.hpp
 @brief  H.265 解析
 @author DBCTRADO
*/


#ifndef LIBISDB_H265_PARSER_H
#define LIBISDB_H265_PARSER_H


#include "MPEGVideoParser.hpp"


namespace LibISDB
{

	/** H.265 アクセスユニットクラス */
	class H265AccessUnit
		: public DataBuffer
	{
	public:
		struct TimingInfo {
			uint32_t NumUnitsInTick;
			uint32_t TimeScale;
		};

		H265AccessUnit();

		bool ParseHeader();
		void Reset();

		uint16_t GetHorizontalSize() const noexcept;
		uint16_t GetVerticalSize() const noexcept;
		bool GetSAR(ReturnArg<uint16_t> Horizontal, ReturnArg<uint16_t> Vertical) const noexcept;
		bool GetTimingInfo(ReturnArg<TimingInfo> Info) const noexcept;

	protected:
		int GetSubWidthC() const noexcept;
		int GetSubHeightC() const noexcept;

		struct Header {
			// AUD (Access Unit Delimiter)
			struct {
				uint8_t PicType;
			} AUD;

			// SPS (Sequence Parameter Set)
			struct {
				uint8_t SPSVideoParameterSetID;
				uint8_t SPSMaxSubLayersMinus1;
				bool SPSTemporalIDNestingFlag;

				// profile_tier_level
				struct {
					uint8_t GeneralProfileSpace;
					bool GeneralTierFlag;
					uint8_t GeneralProfileIDC;
					bool GeneralProfileCompatibilityFlag[32];
					bool GeneralProgressiveSourceFlag;
					bool GeneralInterlacedSourceFlag;
					bool GeneralNonPackedConstraintFlag;
					bool GeneralFrameOnlyConstraintFlag;
					uint8_t GeneralLevelIDC;
					struct {
						bool SubLayerProfilePresentFlag;
						bool SubLayerLevelPresentFlag;
						uint8_t SubLayerProfileSpace;
						bool SubLayerTierFlag;
						uint8_t SubLayerProfileIDC;
						bool SubLayerProfileCompatibilityFlag[32];
						bool SubLayerProgressiveSourceFlag;
						bool SubLayerInterlacedSourceFlag;
						bool SubLayerNonPackedConstraintFlag;
						bool SubLayerFrameOnlyConstraintFlag;
						uint8_t SubLayerLevelIDC;
					} SubLayer[7];
				} PTL;

				uint8_t SPSSeqParameterSetID;
				uint8_t ChromaFormatIDC;
				bool SeparateColourPlaneFlag;
				uint16_t PicWidthInLumaSamples;
				uint16_t PicHeightInLumaSamples;
				bool ConformanceWindowFlag;
				uint8_t ConfWinLeftOffset;
				uint8_t ConfWinRightOffset;
				uint8_t ConfWinTopOffset;
				uint8_t ConfWinBottomOffset;
				uint8_t BitDepthLumaMinus8;
				uint8_t BitDepthChromaMinus8;
				uint8_t Log2MaxPicOrderCntLsbMinus4;
				bool SPSSubLayerOrderingInfoPresentFlag;
				struct {
					uint8_t SPSMaxDecPicBufferingMinus1;
					uint8_t SPSMaxNumReorderPics;
					uint8_t SPSMaxLatencyIncreasePlus1;
				} SubLayerOrderingInfo[8];
				uint8_t Log2MinLumaCodingBlockSizeMinus3;
				uint8_t Log2DiffMaxMinLumaCodingBlockSize;
				uint8_t Log2MinTransformBlockSizeMinus2;
				uint8_t Log2DiffMaxMinTransformBlockSize;
				uint8_t MaxTransformHierarchyDepthInter;
				uint8_t MaxTransformHierarchyDepthIntra;
				bool ScalingListEnabledFlag;
				bool SPSScalingListDataPresentFlag;
				bool AmpEnabledFlag;
				bool SampleAdaptiveOffsetEnabledFlag;
				bool PCMEnabledFlag;
				uint8_t PCMSampleBitDepthLumaMinus1;
				uint8_t PCMSampleBitDepthChromaMinus1;
				uint8_t Log2MinPCMLumaCodingBlockSizeMinus3;
				uint8_t Log2DiffMaxMinPCMLumaCodingBlockSize;
				bool PCMLoopFilterDisabledFlag;
				uint8_t NumShortTermRefPicSets;
				bool LongTermRefPicsPresentFlag;
				uint8_t NumLongTermRefPicsSPS;
				bool SPSTemporalMvpEnabledFlag;
				bool StrongIntraSmoothingEnabledFlag;
				bool VUIParametersPresentFlag;

				// VUI (Video Usability Information)
				struct {
					bool AspectRatioInfoPresentFlag;
					uint8_t AspectRatioIDC;
					uint16_t SARWidth;
					uint16_t SARHeight;
					bool OverscanInfoPresentFlag;
					bool OverscanAppropriateFlag;
					bool VideoSignalTypePresentFlag;
					uint8_t VideoFormat;
					bool VideoFullRangeFlag;
					bool ColourDescriptionPresentFlag;
					uint8_t ColourPrimaries;
					uint8_t TransferCharacteristics;
					uint8_t MatrixCoeffs;
					bool ChromaLocInfoPresentFlag;
					uint8_t ChromaSampleLocTypeTopField;
					uint8_t ChromaSampleLocTypeBottomField;
					bool NeutralChromaIndicationFlag;
					bool FieldSeqFlag;
					bool FrameFieldInfoPresentFlag;
					bool DefaultDisplayWindowFlag;
					uint8_t DefDispWinLeftOffset;
					uint8_t DefDispWinRightOffset;
					uint8_t DefDispWinTopOffset;
					uint8_t DefDispWinBottomOffset;
					bool VUITimingInfoPresentFlag;
					uint32_t VUINumUnitsInTick;
					uint32_t VUITimeScale;
					bool VUIPocProportionalToTimingFlag;
					uint8_t VUINumTicksPocDiffOneMinus1;
					bool VUIHRDParametersPresentFlag;
					bool BitstreamRestrictionFlag;
					bool TilesFixedStructureFlag;
					bool MotionVectorsOverPicBoundariesFlag;
					bool RestrictedRefPicListsFlag;
					uint8_t MinSpatialSegmentationIDC;
					uint8_t MaxBytesPerPicDenom;
					uint8_t MaxBitsPerMinCuDenom;
					uint8_t Log2MaxMVLengthHorizontal;
					uint8_t Log2MaxMVLengthVertical;
				} VUI;
			} SPS;
		};

		bool m_FoundSPS;
		Header m_Header;
	};

	/** H.265 解析クラス */
	class H265Parser
		: public MPEGVideoParserBase
	{
	public:
		class AccessUnitHandler
		{
		public:
			virtual void OnAccessUnit(const H265Parser *pParser, const H265AccessUnit *pAccessUnit) = 0;
		};

		H265Parser(AccessUnitHandler *pAccessUnitHandler);

	// MPEGVideoParserBase
		bool StoreES(const uint8_t *pData, size_t Size) override;
		void Reset() override;

	protected:
	// MPEGVideoParserBase
		void OnSequence(DataBuffer *pSequenceData) override;

		AccessUnitHandler *m_pAccessUnitHandler;
		H265AccessUnit m_AccessUnit;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_H265_PARSER_H
