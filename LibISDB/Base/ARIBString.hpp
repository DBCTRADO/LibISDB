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
 @file   ARIBString.hpp
 @brief  8単位符号文字列の変換を行う
 @author DBCTRADO
*/


#ifndef LIBISDB_ARIB_STRING_H
#define LIBISDB_ARIB_STRING_H


// ARIB 文字列の変換に内部的に使うエンコーディング
#if defined(LIBISDB_WCHAR) || defined(LIBISDB_WINDOWS)
#define LIBISDB_ARIB_STR_IS_WCHAR
#else
#define LIBISDB_ARIB_STR_IS_UTF8
#endif

#include <vector>


namespace LibISDB
{

	/** 8単位符号文字列 */
	typedef std::basic_string<uint8_t> ARIBString;

	/** 8単位符号文字列デコードクラス */
	class ARIBStringDecoder
	{
	public:
#ifdef LIBISDB_ARIB_STR_IS_WCHAR
		typedef wchar_t InternalChar;
#define LIBISDB_ARIB_STR(s) LIBISDB_CAT_SYMBOL(L, s)
#else
		typedef char InternalChar;
#define LIBISDB_ARIB_STR(s) LIBISDB_CAT_SYMBOL(u8, s)
#endif
		typedef std::basic_string<InternalChar> InternalString;

		/** デコードフラグ */
		enum class DecodeFlag : unsigned int {
			None          = 0x0000U, /**< 指定なし */
			Caption       = 0x0001U, /**< 字幕 */
			OneSeg        = 0x0002U, /**< ワンセグ */
			UseCharSize   = 0x0004U, /**< 文字サイズを反映 */
			UnicodeSymbol = 0x0008U, /**< Unicodeの記号を利用(Unicode 5.2以降) */
		};

		/** 文字サイズ */
		enum class CharSize {
			Small,    /**< 小型 */
			Medium,   /**< 中型 */
			Normal,   /**< 標準 */
			Micro,    /**< 超小型 */
			HighW,    /**< 縦倍 */
			WidthW,   /**< 横倍 */
			SizeW,    /**< 縦横倍 */
			Special1, /**< 特殊1 */
			Special2, /**< 特殊2 */
		};

		/** フォーマット情報 */
		struct FormatInfo {
			size_t Pos;               /**< 位置 */
			CharSize Size;            /**< 文字サイズ */
			uint8_t CharColorIndex;   /**< 文字色 */
			uint8_t BackColorIndex;   /**< 背景色 */
			uint8_t RasterColorIndex; /**< ラスタ色 */

			bool operator == (const FormatInfo &rhs) const noexcept
			{
				return (Pos == rhs.Pos)
					&& (Size == rhs.Size)
					&& (CharColorIndex == rhs.CharColorIndex)
					&& (BackColorIndex == rhs.BackColorIndex)
					&& (RasterColorIndex == rhs.RasterColorIndex);
			}

			bool operator != (const FormatInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		typedef std::vector<FormatInfo> FormatList;

		/** DRCSマッピングクラス */
		class DRCSMap
		{
		public:
			virtual ~DRCSMap() = default;

			virtual const InternalChar * GetString(uint16_t Code) = 0;
		};

		bool Decode(
			const uint8_t *pSrcData, size_t SrcLength, ReturnArg<String> DstString,
			DecodeFlag Flags = DecodeFlag::UseCharSize);
		bool Decode(
			const ARIBString &SrcString, ReturnArg<String> DstString,
			DecodeFlag Flags = DecodeFlag::UseCharSize);
		bool DecodeCaption(
			const uint8_t *pSrcData, size_t SrcLength, ReturnArg<String> DstString,
			DecodeFlag Flags = DecodeFlag::None,
			OptionalReturnArg<FormatList> FormatList = std::nullopt,
			DRCSMap *pDRCSMap = nullptr);
		bool DecodeCaption(
			const ARIBString &SrcString, ReturnArg<String> DstString,
			DecodeFlag Flags = DecodeFlag::None,
			OptionalReturnArg<FormatList> FormatList = std::nullopt,
			DRCSMap *pDRCSMap = nullptr);

	private:
		/** 符号集合 */
		enum class CodeSet {
			Unknown,                  /**< 不明なグラフィックセット(非対応) */
			Kanji,                    /**< Kanji */
			Alphanumeric,             /**< Alphanumeric */
			Hiragana,                 /**< Hiragana */
			Katakana,                 /**< Katakana */
			Mosaic_A,                 /**< Mosaic A */
			Mosaic_B,                 /**< Mosaic B */
			Mosaic_C,                 /**< Mosaic C */
			Mosaic_D,                 /**< Mosaic D */
			ProportionalAlphanumeric, /**< Proportional Alphanumeric */
			ProportionalHiragana,     /**< Proportional Hiragana */
			ProportionalKatakana,     /**< Proportional Katakana */
			JIS_X0201_Katakana,       /**< JIS X 0201 Katakana */
			JIS_KanjiPlane1,          /**< JIS compatible Kanji Plane 1 */
			JIS_KanjiPlane2,          /**< JIS compatible Kanji Plane 2 */
			AdditionalSymbols,        /**< Additional symbols */
			DRCS_0,                   /**< DRCS-0 */
			DRCS_1,                   /**< DRCS-1 */
			DRCS_2,                   /**< DRCS-2 */
			DRCS_3,                   /**< DRCS-3 */
			DRCS_4,                   /**< DRCS-4 */
			DRCS_5,                   /**< DRCS-5 */
			DRCS_6,                   /**< DRCS-6 */
			DRCS_7,                   /**< DRCS-7 */
			DRCS_8,                   /**< DRCS-8 */
			DRCS_9,                   /**< DRCS-9 */
			DRCS_10,                  /**< DRCS-10 */
			DRCS_11,                  /**< DRCS-11 */
			DRCS_12,                  /**< DRCS-12 */
			DRCS_13,                  /**< DRCS-13 */
			DRCS_14,                  /**< DRCS-14 */
			DRCS_15,                  /**< DRCS-15 */
			Macro,                    /**< Macro */
		};

		CodeSet m_CodeG[4];
		int m_LockingGL;
		int m_LockingGR;
		int m_SingleGL;

		uint8_t m_ESCSeqCount;
		uint8_t m_ESCSeqIndex;
		bool m_IsESCSeqDRCS;

		CharSize m_CharSize;
		uint8_t m_CharColorIndex;
		uint8_t m_BackColorIndex;
		uint8_t m_RasterColorIndex;
		uint8_t m_DefPalette;
		uint8_t m_RPC;
		FormatList *m_pFormatList;
		DRCSMap *m_pDRCSMap;

		bool m_IsCaption;
		bool m_UseCharSize;
		bool m_UnicodeSymbol;

		bool DecodeInternal(
			const uint8_t *pSrcData, size_t SrcLength, ReturnArg<String> DstString,
			DecodeFlag Flags,
			OptionalReturnArg<FormatList> FormatList = std::nullopt,
			DRCSMap *pDRCSMap = nullptr);
		bool DecodeString(const uint8_t *pSrcData, size_t SrcLength, InternalString *pDstString);
		void DecodeChar(uint16_t Code, CodeSet Set, InternalString *pDstString);

		void PutKanjiChar(uint16_t Code, InternalString *pDstString);
		void PutKanjiPlane2Char(uint16_t Code, InternalString *pDstString);
		void PutAlphanumericChar(uint16_t Code, InternalString *pDstString);
		void PutHiraganaChar(uint16_t Code, InternalString *pDstString);
		void PutKatakanaChar(uint16_t Code, InternalString *pDstString);
		void PutJISKatakanaChar(uint16_t Code, InternalString *pDstString);
		void PutSymbolsChar(uint16_t Code, InternalString *pDstString);
		void PutMacroChar(uint16_t Code, InternalString *pDstString);
		void PutDRCSChar(uint16_t Code, InternalString *pDstString);

		void ProcessEscapeSeq(uint8_t Code);

		bool DesignationGSET(uint8_t IndexG, uint8_t Code);
		bool DesignationDRCS(uint8_t IndexG, uint8_t Code);

		bool SetFormat(size_t Pos);

		bool IsSmallCharMode() const noexcept
		{
			return (m_CharSize == CharSize::Small)
				|| (m_CharSize == CharSize::Medium)
				|| (m_CharSize == CharSize::Micro);
		}

		static bool IsDoubleByteCodeSet(CodeSet Set);
	};

	LIBISDB_ENUM_FLAGS(ARIBStringDecoder::DecodeFlag)

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ARIB_STRING_H
