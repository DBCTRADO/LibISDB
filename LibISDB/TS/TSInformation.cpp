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
 @file   TSInformation.cpp
 @brief  TS 情報
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "TSInformation.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


const CharType * GetStreamTypeText(uint8_t StreamType)
{
	switch (StreamType) {
	case STREAM_TYPE_MPEG1_VIDEO:                   return LIBISDB_STR("MPEG-1 Video");
	case STREAM_TYPE_MPEG2_VIDEO:                   return LIBISDB_STR("MPEG-2 Video");
	case STREAM_TYPE_MPEG1_AUDIO:                   return LIBISDB_STR("MPEG-1 Audio");
	case STREAM_TYPE_MPEG2_AUDIO:                   return LIBISDB_STR("MPEG-2 Audio");
	case STREAM_TYPE_PRIVATE_SECTIONS:              return LIBISDB_STR("private_sections");
	case STREAM_TYPE_PRIVATE_DATA:                  return LIBISDB_STR("private data");
	case STREAM_TYPE_MHEG:                          return LIBISDB_STR("MHEG");
	case STREAM_TYPE_DSM_CC:                        return LIBISDB_STR("DSM-CC");
	case STREAM_TYPE_ITU_T_REC_H222_1:              return LIBISDB_STR("H.222.1");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_A:        return LIBISDB_STR("ISO/IEC 13818-6 type A");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_B:        return LIBISDB_STR("ISO/IEC 13818-6 type B");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_C:        return LIBISDB_STR("ISO/IEC 13818-6 type C");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_D:        return LIBISDB_STR("ISO/IEC 13818-6 type D");
	case STREAM_TYPE_ISO_IEC_13818_1_AUXILIARY:     return LIBISDB_STR("auxiliary");
	case STREAM_TYPE_AAC:                           return LIBISDB_STR("AAC");
	case STREAM_TYPE_MPEG4_VISUAL:                  return LIBISDB_STR("MPEG-4 Visual");
	case STREAM_TYPE_MPEG4_AUDIO:                   return LIBISDB_STR("MPEG-4 Audio");
	case STREAM_TYPE_ISO_IEC_14496_1_IN_PES:        return LIBISDB_STR("ISO/IEC 14496-1 in PES packets");
	case STREAM_TYPE_ISO_IEC_14496_1_IN_SECTIONS:   return LIBISDB_STR("ISO/IEC 14496-1 in ISO/IEC 14496_sections");
	case STREAM_TYPE_ISO_IEC_13818_6_DOWNLOAD:      return LIBISDB_STR("ISO/IEC 13818-6 Synchronized Download Protocol");
	case STREAM_TYPE_METADATA_IN_PES:               return LIBISDB_STR("Metadata in PES packets");
	case STREAM_TYPE_METADATA_IN_SECTIONS:          return LIBISDB_STR("Metadata in metadata_sections");
	case STREAM_TYPE_METADATA_IN_DATA_CAROUSEL:     return LIBISDB_STR("Metadata in ISO/IEC 13818-6 Data Carousel");
	case STREAM_TYPE_METADATA_IN_OBJECT_CAROUSEL:   return LIBISDB_STR("Metadata in ISO/IEC 13818-6 Object Carousel");
	case STREAM_TYPE_METADATA_IN_DOWNLOAD_PROTOCOL: return LIBISDB_STR("Metadata in ISO/IEC 13818-6 Synchronized Download Protocol");
	case STREAM_TYPE_IPMP:                          return LIBISDB_STR("IPMP");
	case STREAM_TYPE_H264:                          return LIBISDB_STR("H.264");
	case STREAM_TYPE_H265:                          return LIBISDB_STR("H.265");
	case STREAM_TYPE_USER_PRIVATE:                  return LIBISDB_STR("user private");
	case STREAM_TYPE_AC3:                           return LIBISDB_STR("AC-3");
	case STREAM_TYPE_DTS:                           return LIBISDB_STR("DTS");
	case STREAM_TYPE_TRUEHD:                        return LIBISDB_STR("TrueHD");
	case STREAM_TYPE_DOLBY_DIGITAL_PLUS:            return LIBISDB_STR("Dolby Digital Plus");
	}

	return nullptr;
}


const CharType * GetAreaText_ja(uint16_t AreaCode)
{
	switch (AreaCode) {
	// 広域符号
	case 0x5A5: return LIBISDB_STR("関東広域圏");
	case 0x72A: return LIBISDB_STR("中京広域圏");
	case 0x8D5: return LIBISDB_STR("近畿広域圏");
	case 0x699: return LIBISDB_STR("鳥取・島根圏");
	case 0x553: return LIBISDB_STR("岡山・香川圏");
	// 県域符号
	case 0x16B: return LIBISDB_STR("北海道");
	case 0x467: return LIBISDB_STR("青森");
	case 0x5D4: return LIBISDB_STR("岩手");
	case 0x758: return LIBISDB_STR("宮城");
	case 0xAC6: return LIBISDB_STR("秋田");
	case 0xE4C: return LIBISDB_STR("山形");
	case 0x1AE: return LIBISDB_STR("福島");
	case 0xC69: return LIBISDB_STR("茨城");
	case 0xE38: return LIBISDB_STR("栃木");
	case 0x98B: return LIBISDB_STR("群馬");
	case 0x64B: return LIBISDB_STR("埼玉");
	case 0x1C7: return LIBISDB_STR("千葉");
	case 0xAAC: return LIBISDB_STR("東京");
	case 0x56C: return LIBISDB_STR("神奈川");
	case 0x4CE: return LIBISDB_STR("新潟");
	case 0x539: return LIBISDB_STR("富山");
	case 0x6A6: return LIBISDB_STR("石川");
	case 0x92D: return LIBISDB_STR("福井");
	case 0xD4A: return LIBISDB_STR("山梨");
	case 0x9D2: return LIBISDB_STR("長野");
	case 0xA65: return LIBISDB_STR("岐阜");
	case 0xA5A: return LIBISDB_STR("静岡");
	case 0x966: return LIBISDB_STR("愛知");
	case 0x2DC: return LIBISDB_STR("三重");
	case 0xCE4: return LIBISDB_STR("滋賀");
	case 0x59A: return LIBISDB_STR("京都");
	case 0xCB2: return LIBISDB_STR("大阪");
	case 0x674: return LIBISDB_STR("兵庫");
	case 0xA93: return LIBISDB_STR("奈良");
	case 0x396: return LIBISDB_STR("和歌山");
	case 0xD23: return LIBISDB_STR("鳥取");
	case 0x31B: return LIBISDB_STR("島根");
	case 0x2B5: return LIBISDB_STR("岡山");
	case 0xB31: return LIBISDB_STR("広島");
	case 0xB98: return LIBISDB_STR("山口");
	case 0xE62: return LIBISDB_STR("徳島");
	case 0x9B4: return LIBISDB_STR("香川");
	case 0x19D: return LIBISDB_STR("愛媛");
	case 0x2E3: return LIBISDB_STR("高知");
	case 0x62D: return LIBISDB_STR("福岡");
	case 0x959: return LIBISDB_STR("佐賀");
	case 0xA2B: return LIBISDB_STR("長崎");
	case 0x8A7: return LIBISDB_STR("熊本");
	case 0xC8D: return LIBISDB_STR("大分");
	case 0xD1C: return LIBISDB_STR("宮崎");
	case 0xD45: return LIBISDB_STR("鹿児島");
	case 0x372: return LIBISDB_STR("沖縄");
	}

	return nullptr;
}


const CharType * GetComponentTypeText_ja(uint8_t StreamContent, uint8_t ComponentType)
{
	switch (StreamContent) {
	case 0x01:
	case 0x05:
		return GetVideoComponentTypeText_ja(ComponentType);

	case 0x02:
		return GetAudioComponentTypeText_ja(ComponentType);
	}

	return nullptr;
}


const CharType * GetVideoComponentTypeText_ja(uint8_t ComponentType)
{
	switch (ComponentType) {
	case 0x01: return LIBISDB_STR("480i[4:3]");
	case 0x02: return LIBISDB_STR("480i[16:9] パンベクトルあり");
	case 0x03: return LIBISDB_STR("480i[16:9]");
	case 0x04: return LIBISDB_STR("480i[>16:9]");
	case 0x91: return LIBISDB_STR("2160p[4:3]");
	case 0x92: return LIBISDB_STR("2160p[16:9] パンベクトルあり");
	case 0x93: return LIBISDB_STR("2160p[16:9]");
	case 0x94: return LIBISDB_STR("2160p[>16:9]");
	case 0xA1: return LIBISDB_STR("480p[4:3]");
	case 0xA2: return LIBISDB_STR("480p[16:9] パンベクトルあり");
	case 0xA3: return LIBISDB_STR("480p[16:9]");
	case 0xA4: return LIBISDB_STR("480p[>16:9]");
	case 0xB1: return LIBISDB_STR("1080i[4:3]");
	case 0xB2: return LIBISDB_STR("1080i[16:9] パンベクトルあり");
	case 0xB3: return LIBISDB_STR("1080i[16:9]");
	case 0xB4: return LIBISDB_STR("1080i[>16:9]");
	case 0xC1: return LIBISDB_STR("720p[4:3]");
	case 0xC2: return LIBISDB_STR("720p[16:9] パンベクトルあり");
	case 0xC3: return LIBISDB_STR("720p[16:9]");
	case 0xC4: return LIBISDB_STR("720p[>16:9]");
	case 0xD1: return LIBISDB_STR("240p[4:3]");
	case 0xD2: return LIBISDB_STR("240p[16:9] パンベクトルあり");
	case 0xD3: return LIBISDB_STR("240p[16:9]");
	case 0xD4: return LIBISDB_STR("240p[>16:9]");
	case 0xE1: return LIBISDB_STR("1080p[4:3]");
	case 0xE2: return LIBISDB_STR("1080p[16:9] パンベクトルあり");
	case 0xE3: return LIBISDB_STR("1080p[16:9]");
	case 0xE4: return LIBISDB_STR("1080p[>16:9]");
	case 0xF1: return LIBISDB_STR("180p[4:3]");
	case 0xF2: return LIBISDB_STR("180p[16:9] パンベクトルあり");
	case 0xF3: return LIBISDB_STR("180p[16:9]");
	case 0xF4: return LIBISDB_STR("180p[>16:9]");
	}

	return nullptr;
}


const CharType * GetAudioComponentTypeText_ja(uint8_t ComponentType)
{
	switch (ComponentType) {
	case 0x01: return LIBISDB_STR("Mono");      // 1/0
	case 0x02: return LIBISDB_STR("Dual mono"); // 1/0 + 1/0
	case 0x03: return LIBISDB_STR("Stereo");    // 2/0
	case 0x04: return LIBISDB_STR("3ch[2/1]");
	case 0x05: return LIBISDB_STR("3ch[3/0]");
	case 0x06: return LIBISDB_STR("4ch[2/2]");
	case 0x07: return LIBISDB_STR("4ch[3/1]");
	case 0x08: return LIBISDB_STR("5ch");       // 3/2
	case 0x09: return LIBISDB_STR("5.1ch");     // 3/2.1
	case 0x0A: return LIBISDB_STR("6.1ch[3/3.1]");
	case 0x0B: return LIBISDB_STR("6.1ch[2/0/0-2/0/2-0.1]");
	case 0x0C: return LIBISDB_STR("7.1ch[5/2.1]");
	case 0x0D: return LIBISDB_STR("7.1ch[3/2/2.1]");
	case 0x0E: return LIBISDB_STR("7.1ch[2/0/0-3/0/2-0.1]");
	case 0x0F: return LIBISDB_STR("7.1ch[0/2/0-3/0/2-0.1]");
	case 0x10: return LIBISDB_STR("10.2ch");    // 2/0/0-3/2/3-0.2
	case 0x11: return LIBISDB_STR("22.2ch");    // 3/3/3-5/2/3-3/0/0.2
	case 0x40: return LIBISDB_STR("視覚障害者用音声解説");
	case 0x41: return LIBISDB_STR("聴覚障害者用音声");
	}

	return nullptr;
}


const CharType * GetPredefinedPIDText(uint16_t PID)
{
	switch (PID) {
	case PID_PAT:  return LIBISDB_STR("PAT");
	case PID_CAT:  return LIBISDB_STR("CAT");
	case PID_NIT:  return LIBISDB_STR("NIT");
	case PID_SDT:  return LIBISDB_STR("SDT");
	case PID_HEIT: return LIBISDB_STR("H-EIT");
	case PID_TOT:  return LIBISDB_STR("TOT");
	case PID_SDTT: return LIBISDB_STR("SDTT");
	case PID_BIT:  return LIBISDB_STR("BIT");
	case PID_NBIT: return LIBISDB_STR("NBIT");
	case PID_MEIT: return LIBISDB_STR("M-EIT");
	case PID_LEIT: return LIBISDB_STR("L-EIT");
	case PID_CDT:  return LIBISDB_STR("CDT");
	case PID_NULL: return LIBISDB_STR("Null");
	}

	return nullptr;
}


bool GetLanguageText_ja(
	uint32_t LanguageCode, CharType *pText, size_t MaxText, LanguageTextType Type)
{
	static const struct {
		uint32_t LanguageCode;
		const CharType *pLongText;
		const CharType *pSimpleText;
		const CharType *pShortText;
	} LanguageList[] = {
		{LANGUAGE_CODE_JPN, LIBISDB_STR("日本語"),     LIBISDB_STR("日本語"), LIBISDB_STR("日")},
		{LANGUAGE_CODE_ENG, LIBISDB_STR("英語"),       LIBISDB_STR("英語"),   LIBISDB_STR("英")},
		{LANGUAGE_CODE_DEU, LIBISDB_STR("ドイツ語"),   LIBISDB_STR("独語"),   LIBISDB_STR("独")},
		{LANGUAGE_CODE_FRA, LIBISDB_STR("フランス語"), LIBISDB_STR("仏語"),   LIBISDB_STR("仏")},
		{LANGUAGE_CODE_ITA, LIBISDB_STR("イタリア語"), LIBISDB_STR("伊語"),   LIBISDB_STR("伊")},
		{LANGUAGE_CODE_RUS, LIBISDB_STR("ロシア語"),   LIBISDB_STR("露語"),   LIBISDB_STR("露")},
		{LANGUAGE_CODE_ZHO, LIBISDB_STR("中国語"),     LIBISDB_STR("中国語"), LIBISDB_STR("中")},
		{LANGUAGE_CODE_KOR, LIBISDB_STR("韓国語"),     LIBISDB_STR("韓国語"), LIBISDB_STR("韓")},
		{LANGUAGE_CODE_SPA, LIBISDB_STR("スペイン語"), LIBISDB_STR("西語"),   LIBISDB_STR("西")},
		{LANGUAGE_CODE_ETC, LIBISDB_STR("外国語"),     LIBISDB_STR("外国語"), LIBISDB_STR("外")},
	};

	if ((pText == nullptr) || (MaxText < 1))
		return false;

	for (size_t i = 0; i < std::size(LanguageList); i++) {
		if (LanguageList[i].LanguageCode == LanguageCode) {
			const CharType *pLang;

			switch (Type) {
			default:
			case LanguageTextType::Long:   pLang = LanguageList[i].pLongText;   break;
			case LanguageTextType::Simple: pLang = LanguageList[i].pSimpleText; break;
			case LanguageTextType::Short:  pLang = LanguageList[i].pShortText;  break;
			}

			StringCopy(pText, pLang, MaxText);

			return true;
		}
	}

	return LanguageCodeToText(LanguageCode, pText, MaxText);
}


bool LanguageCodeToText(
	uint32_t LanguageCode, CharType *pText, size_t MaxText, bool UpperCase)
{
	if ((pText == nullptr) || (MaxText < 4))
		return false;

	pText[0] = static_cast<CharType>((LanguageCode >> 16) & 0xFF);
	pText[1] = static_cast<CharType>((LanguageCode >>  8) & 0xFF);
	pText[2] = static_cast<CharType>(LanguageCode & 0xFF);
	pText[3] = LIBISDB_CHAR('\0');

	if (UpperCase) {
		pText[0] = ToUpper(pText[0]);
		pText[1] = ToUpper(pText[1]);
		pText[2] = ToUpper(pText[2]);
	}

	return true;
}


}	// namespace LibISDB
