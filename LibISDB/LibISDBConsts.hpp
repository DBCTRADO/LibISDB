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
 @file   LibISDBConsts.hpp
 @brief  定数定義ヘッダ
 @author DBCTRADO
*/


#ifndef LIBISDB_CONSTS_H
#define LIBISDB_CONSTS_H


namespace LibISDB
{

	constexpr size_t TS_PACKET_SIZE     = 188;
	constexpr size_t TS_PACKET_SIZE_MAX = 204; // 188 + 16(FEC)

	constexpr uint16_t TRANSPORT_STREAM_ID_INVALID = 0x0000_u16;
	constexpr uint16_t NETWORK_ID_INVALID = 0x0000_u16;
	constexpr uint16_t SERVICE_ID_INVALID = 0x0000_u16;

	constexpr uint16_t EVENT_ID_INVALID = 0x0000_u16;
	constexpr uint8_t COMPONENT_TAG_INVALID = 0xFF_u8;
	constexpr uint8_t COMPONENT_TYPE_INVALID = 0xFF_u8;
	constexpr uint8_t STREAM_CONTENT_INVALID = 0xFF_u8;

	constexpr uint64_t PCR_INVALID = 0xFFFFFFFFFFFFFFFF_u64;

	/*
		PID
	*/
	constexpr uint16_t PID_PAT     = 0x0000_u16; // PAT
	constexpr uint16_t PID_CAT     = 0x0001_u16; // CAT
	constexpr uint16_t PID_NIT     = 0x0010_u16; // NIT
	constexpr uint16_t PID_SDT     = 0x0011_u16; // SDT
	constexpr uint16_t PID_HEIT    = 0x0012_u16; // H-EIT
	constexpr uint16_t PID_TOT     = 0x0014_u16; // TOT
	constexpr uint16_t PID_SDTT    = 0x0023_u16; // SDTT
	constexpr uint16_t PID_BIT     = 0x0024_u16; // BIT
	constexpr uint16_t PID_NBIT    = 0x0025_u16; // NBIT
	constexpr uint16_t PID_MEIT    = 0x0026_u16; // M-EIT
	constexpr uint16_t PID_LEIT    = 0x0027_u16; // L-EIT
	constexpr uint16_t PID_CDT     = 0x0029_u16; // CDT
	constexpr uint16_t PID_NULL    = 0x1FFF_u16; // Null
	constexpr uint16_t PID_MAX     = 0x1FFF_u16; // 最大
	constexpr uint16_t PID_INVALID = 0xFFFF_u16; // 無効

	// ワンセグPMT PID
	constexpr uint16_t ONESEG_PMT_PID_FIRST = 0x1FC8_u16;
	constexpr uint16_t ONESEG_PMT_PID_LAST  = 0x1FCF_u16;
	constexpr int ONESEG_PMT_PID_COUNT = ONESEG_PMT_PID_LAST - ONESEG_PMT_PID_FIRST + 1;

	constexpr bool Is1SegPMTPID(uint16_t Pid) {
		return (Pid >= ONESEG_PMT_PID_FIRST) && (Pid <= ONESEG_PMT_PID_LAST);
	}

	/*
		stream_type
	*/
	constexpr uint8_t STREAM_TYPE_MPEG1_VIDEO                   = 0x01_u8; // ISO/IEC 11172-2 Video
	constexpr uint8_t STREAM_TYPE_MPEG2_VIDEO                   = 0x02_u8; // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
	constexpr uint8_t STREAM_TYPE_MPEG1_AUDIO                   = 0x03_u8; // ISO/IEC 11172-3 Audio
	constexpr uint8_t STREAM_TYPE_MPEG2_AUDIO                   = 0x04_u8; // ISO/IEC 13818-3 Audio
	constexpr uint8_t STREAM_TYPE_PRIVATE_SECTIONS              = 0x05_u8; // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections
	constexpr uint8_t STREAM_TYPE_PRIVATE_DATA                  = 0x06_u8; // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data
	constexpr uint8_t STREAM_TYPE_MHEG                          = 0x07_u8; // ISO/IEC 13522 MHEG
	constexpr uint8_t STREAM_TYPE_DSM_CC                        = 0x08_u8; // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC
	constexpr uint8_t STREAM_TYPE_ITU_T_REC_H222_1              = 0x09_u8; // ITU-T Rec. H.222.1
	constexpr uint8_t STREAM_TYPE_ISO_IEC_13818_6_TYPE_A        = 0x0A_u8; // ISO/IEC 13818-6 type A
	constexpr uint8_t STREAM_TYPE_ISO_IEC_13818_6_TYPE_B        = 0x0B_u8; // ISO/IEC 13818-6 type B
	constexpr uint8_t STREAM_TYPE_ISO_IEC_13818_6_TYPE_C        = 0x0C_u8; // ISO/IEC 13818-6 type C
	constexpr uint8_t STREAM_TYPE_ISO_IEC_13818_6_TYPE_D        = 0x0D_u8; // ISO/IEC 13818-6 type D
	constexpr uint8_t STREAM_TYPE_ISO_IEC_13818_1_AUXILIARY     = 0x0E_u8; // ITU-T Rec. H.222.0 | ISO/IEC 13818-1 auxiliary
	constexpr uint8_t STREAM_TYPE_AAC                           = 0x0F_u8; // ISO/IEC 13818-7 Audio with ADTS transport syntax
	constexpr uint8_t STREAM_TYPE_MPEG4_VISUAL                  = 0x10_u8; // ISO/IEC 14496-2 Visual
	constexpr uint8_t STREAM_TYPE_MPEG4_AUDIO                   = 0x11_u8; // ISO/IEC 14496-3 Audio with the LATM transport syntax as defined in ISO/IEC 14496-3 / AMD 1
	constexpr uint8_t STREAM_TYPE_ISO_IEC_14496_1_IN_PES        = 0x12_u8; // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets
	constexpr uint8_t STREAM_TYPE_ISO_IEC_14496_1_IN_SECTIONS   = 0x13_u8; // ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC 14496_sections
	constexpr uint8_t STREAM_TYPE_ISO_IEC_13818_6_DOWNLOAD      = 0x14_u8; // ISO/IEC 13818-6 Synchronized Download Protocol
	constexpr uint8_t STREAM_TYPE_METADATA_IN_PES               = 0x15_u8; // Metadata carried in PES packets
	constexpr uint8_t STREAM_TYPE_METADATA_IN_SECTIONS          = 0x16_u8; // Metadata carried in metadata_sections
	constexpr uint8_t STREAM_TYPE_METADATA_IN_DATA_CAROUSEL     = 0x17_u8; // Metadata carried in ISO/IEC 13818-6 Data Carousel
	constexpr uint8_t STREAM_TYPE_METADATA_IN_OBJECT_CAROUSEL   = 0x18_u8; // Metadata carried in ISO/IEC 13818-6 Object Carousel
	constexpr uint8_t STREAM_TYPE_METADATA_IN_DOWNLOAD_PROTOCOL = 0x19_u8; // Metadata carried in ISO/IEC 13818-6 Synchronized Download Protocol
	constexpr uint8_t STREAM_TYPE_IPMP                          = 0x1A_u8; // ISO/IEC 13818-11 IPMP on MPEG-2 systems
	constexpr uint8_t STREAM_TYPE_H264                          = 0x1B_u8; // ITU-T Rec. H.264 | ISO/IEC 14496-10 Video
	constexpr uint8_t STREAM_TYPE_H265                          = 0x24_u8; // ITU-T Rec. H.265 | ISO/IEC 23008-2
	constexpr uint8_t STREAM_TYPE_USER_PRIVATE                  = 0x80_u8; // ISO/IEC User Private
	constexpr uint8_t STREAM_TYPE_AC3                           = 0x81_u8; // Dolby AC-3
	constexpr uint8_t STREAM_TYPE_DTS                           = 0x82_u8; // DTS
	constexpr uint8_t STREAM_TYPE_TRUEHD                        = 0x83_u8; // Dolby TrueHD
	constexpr uint8_t STREAM_TYPE_DOLBY_DIGITAL_PLUS            = 0x87_u8; // Dolby Digital Plus

	constexpr uint8_t STREAM_TYPE_UNINITIALIZED                 = 0x00_u8; // 未初期化
	constexpr uint8_t STREAM_TYPE_INVALID                       = 0xFF_u8; // 無効

	constexpr uint8_t STREAM_TYPE_CAPTION                       = STREAM_TYPE_PRIVATE_DATA;           // 字幕
	constexpr uint8_t STREAM_TYPE_DATA_CARROUSEL                = STREAM_TYPE_ISO_IEC_13818_6_TYPE_D; // データ放送

	/*
		service_type
	*/
	constexpr uint8_t SERVICE_TYPE_DIGITAL_TV               = 0x01_u8; // デジタルTVサービス
	constexpr uint8_t SERVICE_TYPE_DIGITAL_AUDIO            = 0x02_u8; // デジタル音声サービス
	// 0x03 - 0x7F 未定義
	// 0x80 - 0xA0 事業者定義
	constexpr uint8_t SERVICE_TYPE_TEMPORARY_VIDEO          = 0xA1_u8; // 臨時映像サービス
	constexpr uint8_t SERVICE_TYPE_TEMPORARY_AUDIO          = 0xA2_u8; // 臨時音声サービス
	constexpr uint8_t SERVICE_TYPE_TEMPORARY_DATA           = 0xA3_u8; // 臨時データサービス
	constexpr uint8_t SERVICE_TYPE_ENGINEERING              = 0xA4_u8; // エンジニアリングサービス
	constexpr uint8_t SERVICE_TYPE_PROMOTION_VIDEO          = 0xA5_u8; // プロモーション映像サービス
	constexpr uint8_t SERVICE_TYPE_PROMOTION_AUDIO          = 0xA6_u8; // プロモーション音声サービス
	constexpr uint8_t SERVICE_TYPE_PROMOTION_DATA           = 0xA7_u8; // プロモーションデータサービス
	constexpr uint8_t SERVICE_TYPE_ACCUMULATION_DATA        = 0xA8_u8; // 事前蓄積用データサービス
	constexpr uint8_t SERVICE_TYPE_ACCUMULATION_ONLY_DATA   = 0xA9_u8; // 蓄積専用データサービス
	constexpr uint8_t SERVICE_TYPE_BOOKMARK_LIST_DATA       = 0xAA_u8; // ブックマーク一覧データサービス
	constexpr uint8_t SERVICE_TYPE_SERVER_TYPE_SIMULTANEOUS = 0xAB_u8; // サーバー型サイマルサービス
	constexpr uint8_t SERVICE_TYPE_INDEPENDENT_FILE         = 0xAC_u8; // 独立ファイルサービス
	constexpr uint8_t SERVICE_TYPE_4K_TV                    = 0xAD_u8; // 超高精細度4K専用TVサービス
	// 0xAD - 0xBF 未定義(標準化機関定義領域)
	constexpr uint8_t SERVICE_TYPE_DATA                     = 0xC0_u8; // データサービス
	constexpr uint8_t SERVICE_TYPE_TLV_ACCUMULATION         = 0xC1_u8; // TLVを用いた蓄積型サービス
	constexpr uint8_t SERVICE_TYPE_MULTIMEDIA               = 0xC2_u8; // マルチメディアサービス
	// 0xC3 - 0xFF 未定義
	constexpr uint8_t SERVICE_TYPE_INVALID                  = 0xFF_u8; // 無効

	/*
		ISO 639 language code
	*/
	constexpr uint32_t LANGUAGE_CODE_JPN     = 0x6A706E_u32; // 日本語
	constexpr uint32_t LANGUAGE_CODE_ENG     = 0x656E67_u32; // 英語
	constexpr uint32_t LANGUAGE_CODE_DEU     = 0x646575_u32; // ドイツ語
	constexpr uint32_t LANGUAGE_CODE_FRA     = 0x667261_u32; // フランス語
	constexpr uint32_t LANGUAGE_CODE_ITA     = 0x697461_u32; // イタリア語
	constexpr uint32_t LANGUAGE_CODE_RUS     = 0x727573_u32; // ロシア語
	constexpr uint32_t LANGUAGE_CODE_ZHO     = 0x7A686F_u32; // 中国語
	constexpr uint32_t LANGUAGE_CODE_KOR     = 0x6B6F72_u32; // 韓国語
	constexpr uint32_t LANGUAGE_CODE_SPA     = 0x737061_u32; // スペイン語
	constexpr uint32_t LANGUAGE_CODE_ETC     = 0x657463_u32; // その他
	constexpr uint32_t LANGUAGE_CODE_INVALID = 0x000000_u32; // 無効

}	// namespace LibISDB


#endif	// ifndef LIBISDB_CONSTS_H
