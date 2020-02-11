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
 @file   H264Parser.hpp
 @brief  H.264 解析
 @author DBCTRADO
*/


#ifndef LIBISDB_H264_PARSER_H
#define LIBISDB_H264_PARSER_H


#include "MPEGVideoParser.hpp"


namespace LibISDB
{

	/** H.264 アクセスユニットクラス */
	class H264AccessUnit
		: public DataBuffer
	{
	public:
		struct TimingInfo {
			uint32_t NumUnitsInTick;
			uint32_t TimeScale;
			bool FixedFrameRateFlag;
		};

		H264AccessUnit();

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
				uint8_t PrimaryPicType;
			} AUD;

			// SPS (Sequence Parameter Set)
			struct {
				uint8_t ProfileIDC;
				bool ConstraintSet0Flag;
				bool ConstraintSet1Flag;
				bool ConstraintSet2Flag;
				bool ConstraintSet3Flag;
				uint8_t LevelIDC;
				uint8_t SeqParameterSetID;
				uint8_t ChromaFormatIDC;
				bool SeparateColourPlaneFlag;
				uint8_t BitDepthLumaMinus8;
				uint8_t BitDepthChromaMinus8;
				bool QpprimeYZeroTransformBypassFlag;
				bool SeqScalingMatrixPresentFlag;
				uint8_t Log2MaxFrameNumMinus4;
				uint8_t PicOrderCntType;
				uint8_t Log2MaxPicOrderCntLSBMinus4;
				bool DeltaPicOrderAlwaysZeroFlag;
				uint8_t OffsetForNonRefPic;
				uint8_t OffsetForTopToBottomField;
				uint8_t NumRefFramesInPicOrderCntCycle;
				uint8_t NumRefFrames;
				bool GapsInFrameNumValueAllowedFlag;
				uint8_t PicWidthInMBSMinus1;
				uint8_t PicHeightInMapUnitsMinus1;
				bool FrameMBSOnlyFlag;
				bool MBAdaptiveFrameFieldFlag;
				bool Direct8x8InferenceFlag;
				bool FrameCroppingFlag;
				uint16_t FrameCropLeftOffset;
				uint16_t FrameCropRightOffset;
				uint16_t FrameCropTopOffset;
				uint16_t FrameCropBottomOffset;
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
					uint8_t MatrixCoefficients;
					bool ChromaLocInfoPresentFlag;
					uint16_t ChromaSampleLocTypeTopField;
					uint16_t ChromaSampleLocTypeBottomField;
					bool TimingInfoPresentFlag;
					uint32_t NumUnitsInTick;
					uint32_t TimeScale;
					bool FixedFrameRateFlag;
					// ...
				} VUI;

				uint8_t ChromaArrayType;
			} SPS;
		};

		bool m_FoundSPS;
		Header m_Header;
	};

	/** H.264 解析クラス */
	class H264Parser
		: public MPEGVideoParserBase
	{
	public:
		class AccessUnitHandler
		{
		public:
			virtual void OnAccessUnit(const H264Parser *pParser, const H264AccessUnit *pAccessUnit) = 0;
		};

		H264Parser(AccessUnitHandler *pAccessUnitHandler);

	// MPEGVideoParserBase
		bool StoreES(const uint8_t *pData, size_t Size) override;
		void Reset() override;

	protected:
	// MPEGVideoParserBase
		void OnSequence(DataBuffer *pSequenceData) override;

		AccessUnitHandler *m_pAccessUnitHandler;
		H264AccessUnit m_AccessUnit;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_H264_PARSER_H
