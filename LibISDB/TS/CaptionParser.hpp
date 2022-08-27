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
 @file   CaptionParser.hpp
 @brief  字幕解析
 @author DBCTRADO
*/


#ifndef LIBISDB_CAPTION_PARSER_H
#define LIBISDB_CAPTION_PARSER_H


#include "PESPacket.hpp"
#include "../Base/ARIBString.hpp"
#include <vector>


namespace LibISDB
{

	/** 字幕解析クラス */
	class CaptionParser
		: public PESParser::PacketHandler
	{
	public:
		struct LanguageInfo {
			uint8_t LanguageTag;
			uint8_t DMF;
			uint8_t DC;
			uint32_t LanguageCode;
			uint8_t Format;
			uint8_t TCS;
			uint8_t RollupMode;

			bool operator == (const LanguageInfo &rhs) const noexcept = default;
		};

		struct TimeInfo {
			uint8_t Hour;
			uint8_t Minute;
			uint8_t Second;
			uint16_t Millisecond;
		};

		class CaptionHandler
		{
		public:
			virtual ~CaptionHandler() = default;

			virtual void OnLanguageUpdate(CaptionParser *pParser) {}
			virtual void OnCaption(
				CaptionParser *pParser, uint8_t Language, const CharType *pText,
				const ARIBStringDecoder::FormatList *pFormatList) {}
		};

		struct DRCSBitmap {
			uint8_t Width;
			uint8_t Height;
			uint8_t Depth;
			uint8_t BitsPerPixel;
			const uint8_t *pData;
			uint32_t DataSize;
		};

		class DRCSMap
			: public ARIBStringDecoder::DRCSMap
		{
		public:
			virtual bool SetDRCS(uint16_t Code, const DRCSBitmap *pBitmap) = 0;
		};

		CaptionParser(bool OneSeg = false) noexcept;

		void Reset();
		bool StorePacket(const TSPacket *pPacket);
		void SetCaptionHandler(CaptionHandler *pHandler);
		void SetDRCSMap(DRCSMap *pDRCSMap);
		int GetLanguageCount() const;
		bool GetLanguageInfo(int Index, ReturnArg<LanguageInfo> Info) const;
		int GetLanguageIndexByTag(uint8_t LanguageTag) const;
		uint32_t GetLanguageCodeByTag(uint8_t LanguageTag) const;
		bool Is1Seg() const noexcept { return m_1Seg; }

	private:
	// PESParser::PacketHandler
		void OnPESPacket(const PESParser *pParser, const PESPacket *pPacket) override;

		bool ParseManagementData(const uint8_t *pData, uint32_t DataSize);
		bool ParseCaptionData(const uint8_t *pData, uint32_t DataSize);
		bool ParseUnitData(const uint8_t *pData, uint32_t *pDataSize);
		bool ParseDRCSUnitData(const uint8_t *pData, uint32_t DataSize);
		void OnCaption(const CharType *pText, const ARIBStringDecoder::FormatList *pFormatList);

		PESParser m_PESParser;
		ARIBStringDecoder m_StringDecoder;
		CaptionHandler *m_pHandler;
		DRCSMap *m_pDRCSMap;
		bool m_1Seg;

		std::vector<LanguageInfo> m_LanguageList;
		uint8_t m_DataGroupVersion;
		uint8_t m_DataGroupID;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_CAPTION_PARSER_H
