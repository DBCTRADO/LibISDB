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
 @file   ADTSParser.hpp
 @brief  ADTS 解析
 @author DBCTRADO
*/


#ifndef LIBISDB_ADTS_PARSER_H
#define LIBISDB_ADTS_PARSER_H


#include "../TS/PESPacket.hpp"


namespace LibISDB
{

	/** ADTS フレームクラス */
	class ADTSFrame
		: public DataBuffer
	{
	public:
		ADTSFrame();

		bool ParseHeader();
		void Reset();

		uint8_t GetProfile() const noexcept { return m_Header.Profile; }
		uint8_t GetSamplingFreqIndex() const noexcept { return m_Header.SamplingFreqIndex; }
		uint32_t GetSamplingFreq() const noexcept;
		bool GetPrivateBit() const noexcept { return m_Header.PrivateBit; }
		uint8_t GetChannelConfig() const noexcept { return m_Header.ChannelConfig; }
		bool GetOriginalCopy() const noexcept { return m_Header.OriginalCopy; }
		bool GetHome() const noexcept { return m_Header.Home; }
		bool GetCopyrightIDBit() const noexcept { return m_Header.CopyrightIDBit; }
		bool GetCopyrightIDStart() const noexcept { return m_Header.CopyrightIDStart; }
		uint16_t GetFrameLength() const noexcept { return m_Header.FrameLength; }
		uint16_t GetBufferFullness() const noexcept { return m_Header.BufferFullness; }
		uint8_t GetRawDataBlockNum() const noexcept { return m_Header.RawDataBlockNum; }

	protected:
		/** ADTS ヘッダ */
		struct ADTSHeader {
			// adts_fixed_header()
			bool MPEGVersion;          /**< MPEG Version */
			bool ProtectionAbsent;     /**< Protection Absent */
			uint8_t Profile;           /**< Profile */
			uint8_t SamplingFreqIndex; /**< Sampling Frequency Index */
			bool PrivateBit;           /**< Private Bit */
			uint8_t ChannelConfig;     /**< Channel Configuration */
			bool OriginalCopy;         /**< Original/Copy */
			bool Home;                 /**< Home */

			// adts_variable_header()
			bool CopyrightIDBit;       /**< Copyright Identification Bit */
			bool CopyrightIDStart;     /**< Copyright Identification Start */
			uint16_t FrameLength;      /**< Frame Length */
			uint16_t BufferFullness;   /**< ADTS Buffer Fullness */
			uint8_t RawDataBlockNum;   /**< Number of Raw Data Blocks in Frame */
		};

		ADTSHeader m_Header;
	};

	/** ADTS 解析クラス */
	class ADTSParser
		: public PESParser::PacketHandler
	{
	public:
		class FrameHandler
		{
		public:
			virtual void OnADTSFrame(const ADTSParser *pParser, const ADTSFrame *pFrame) = 0;
		};

		ADTSParser(FrameHandler *pFrameHandler);

		bool StorePacket(const PESPacket *pPacket);
		bool StoreES(const uint8_t *pData, size_t Size);
		bool StoreES(const uint8_t *pData, size_t *pSize, ADTSFrame **ppFrame);
		void Reset();

	protected:
	// PESParser::PacketHandler
		void OnPESPacket(const PESParser *pParser, const PESPacket *pPacket) override;

		virtual void OnADTSFrame(const ADTSFrame *pFrame) const;

		FrameHandler *m_pFrameHandler;
		ADTSFrame m_ADTSFrame;

	private:
		bool SyncFrame(uint8_t Data);

		bool m_IsStoring;
		uint16_t m_StoreCRC;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ADTS_PARSER_H
