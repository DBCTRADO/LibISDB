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
 @file   MPEG2VideoParser.hpp
 @brief  MPEG-2 映像解析
 @author DBCTRADO
*/


#ifndef LIBISDB_MPEG2_VIDEO_PARSER_H
#define LIBISDB_MPEG2_VIDEO_PARSER_H


#include "MPEGVideoParser.hpp"


namespace LibISDB
{

	/** MPEG-2 シーケンスクラス */
	class MPEG2Sequence
		: public DataBuffer
	{
	public:
		MPEG2Sequence();

		bool ParseHeader();
		void Reset();

		uint16_t GetHorizontalSize() const noexcept { return m_Header.HorizontalSize; }
		uint16_t GetVerticalSize() const noexcept { return m_Header.VerticalSize; }
		uint8_t GetAspectRatioInfo() const noexcept { return m_Header.AspectRatioInfo; }
		bool GetAspectRatio(ReturnArg<uint8_t> AspectX, ReturnArg<uint8_t> AspectY) const noexcept;
		uint8_t GetFrameRateCode() const noexcept { return m_Header.FrameRateCode; }
		bool GetFrameRate(ReturnArg<uint32_t> Num, ReturnArg<uint32_t> Denom) const noexcept;
		uint32_t GetBitRate() const noexcept { return m_Header.BitRate; }
		bool GetMarkerBit() const noexcept { return m_Header.MarkerBit; }
		uint32_t GetVBVBufferSize() const noexcept { return m_Header.VBVBufferSize; }
		bool GetConstrainedParametersFlag() const noexcept { return m_Header.ConstrainedParametersFlag; }
		bool GetLoadIntraQuantiserMatrix() const noexcept { return m_Header.LoadIntraQuantiserMatrix; }

		bool HasExtendDisplayInfo() const noexcept { return m_Header.Extension.Display.IsValid; }
		uint16_t GetExtendDisplayHorizontalSize() const noexcept { return m_Header.Extension.Display.DisplayHorizontalSize; }
		uint16_t GetExtendDisplayVerticalSize() const noexcept { return m_Header.Extension.Display.DisplayVerticalSize; }

	protected:
		/** シーケンスヘッダ */
		struct SequenceHeader {
			// sequence_header()
			uint16_t HorizontalSize;          /**< Horizontal Size Value */
			uint16_t VerticalSize;            /**< Vertical Size Value */
			uint8_t AspectRatioInfo;          /**< Aspect Ratio Information */
			uint8_t FrameRateCode;            /**< Frame Rate Code */
			uint32_t BitRate;                 /**< Bit Rate Value */
			bool MarkerBit;                   /**< Marker Bit */
			uint32_t VBVBufferSize;           /**< VBV Buffer Size Value */
			bool ConstrainedParametersFlag;   /**< Constrained Parameters Flag */
			bool LoadIntraQuantiserMatrix;    /**< Load Intra Quantiser Matrix */
			bool LoadNonIntraQuantiserMatrix; /**< Load NonIntra Quantiser Matrix */

			// Sequence Extension
			struct {
				struct {
					bool IsValid;
					uint8_t ProfileAndLevel;
					bool Progressive;
					uint8_t ChromaFormat;
					bool LowDelay;
					uint8_t FrameRateExtN;
					uint8_t FrameRateExtD;
				} Sequence;

				struct {
					bool IsValid;
					uint8_t VideoFormat;
					bool ColorDescrption;
					struct {
						uint8_t ColorPrimaries;
						uint8_t TransferCharacteristics;
						uint8_t MatrixCoefficients;
					} Color;
					uint16_t DisplayHorizontalSize;
					uint16_t DisplayVerticalSize;
				} Display;
			} Extension;
		};

		SequenceHeader m_Header;
	};

	/** MPEG-2 映像解析クラス */
	class MPEG2VideoParser
		: public MPEGVideoParserBase
	{
	public:
		class SequenceHandler
		{
		public:
			virtual void OnMPEG2Sequence(const MPEG2VideoParser *pParser, const MPEG2Sequence *pSequence) = 0;
		};

		MPEG2VideoParser(SequenceHandler *pSequenceHandler);

	// MPEGVideoParserBase
		bool StoreES(const uint8_t *pData, size_t Size) override;
		void Reset() override;

	protected:
	// MPEGVideoParserBase
		void OnSequence(DataBuffer *pSequenceData) override;

		SequenceHandler *m_pSequenceHandler;
		MPEG2Sequence m_MPEG2Sequence;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_MPEG2_VIDEO_PARSER_H
