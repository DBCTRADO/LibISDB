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
 @file   ARIBString.cpp
 @brief  8単位符号文字列の変換を行う
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#endif
#include "ARIBString.hpp"
#include "JISKanjiMap.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


#define ARIB_STR(s) LIBISDB_ARIB_STR(s)

#if defined(LIBISDB_ARIB_STR_IS_WCHAR)
	// 文字配列のテーブル
	typedef ARIBStringDecoder::InternalChar ARIBStrTableType;
#define ARIB_STR_TABLE_BEGIN
#define ARIB_STR_TABLE_END
#define ARIB_STR_TABLE(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15) \
	ARIB_STR(s0) ARIB_STR(s1) ARIB_STR(s2) ARIB_STR(s3) \
	ARIB_STR(s4) ARIB_STR(s5) ARIB_STR(s6) ARIB_STR(s7) \
	ARIB_STR(s8) ARIB_STR(s9) ARIB_STR(s10) ARIB_STR(s11) \
	ARIB_STR(s12) ARIB_STR(s13) ARIB_STR(s14) ARIB_STR(s15)
#else
	// 文字列配列のテーブル
	typedef const ARIBStringDecoder::InternalChar *ARIBStrTableType;
#define ARIB_STR_TABLE_BEGIN {
#define ARIB_STR_TABLE_END   }
#define ARIB_STR_TABLE(s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, s15) \
	ARIB_STR(s0), ARIB_STR(s1), ARIB_STR(s2), ARIB_STR(s3), \
	ARIB_STR(s4), ARIB_STR(s5), ARIB_STR(s6), ARIB_STR(s7), \
	ARIB_STR(s8), ARIB_STR(s9), ARIB_STR(s10), ARIB_STR(s11), \
	ARIB_STR(s12), ARIB_STR(s13), ARIB_STR(s14), ARIB_STR(s15),
#endif

#define TOFU_STR ARIB_STR("□")


bool ARIBStringDecoder::Decode(
	const uint8_t *pSrcData, size_t SrcLength, ReturnArg<String> DstString, DecodeFlag Flags)
{
	return DecodeInternal(pSrcData, SrcLength, DstString, Flags);
}


bool ARIBStringDecoder::Decode(
	const ARIBString &SrcString, ReturnArg<String> DstString, DecodeFlag Flags)
{
	return DecodeInternal(SrcString.data(), SrcString.length(), DstString, Flags);
}


bool ARIBStringDecoder::DecodeCaption(
	const uint8_t *pSrcData, size_t SrcLength, ReturnArg<String> DstString,
	DecodeFlag Flags, OptionalReturnArg<FormatList> FormatList, DRCSMap *pDRCSMap)
{
	return DecodeInternal(pSrcData, SrcLength, DstString, Flags | DecodeFlag::Caption, FormatList, pDRCSMap);
}


bool ARIBStringDecoder::DecodeCaption(
	const ARIBString &SrcString, ReturnArg<String> DstString,
	DecodeFlag Flags, OptionalReturnArg<FormatList> FormatList, DRCSMap *pDRCSMap)
{
	return DecodeInternal(SrcString.data(), SrcString.length(), DstString, Flags | DecodeFlag::Caption, FormatList, pDRCSMap);
}


bool ARIBStringDecoder::DecodeInternal(
	const uint8_t *pSrcData, size_t SrcLength, ReturnArg<String> DstString,
	DecodeFlag Flags, OptionalReturnArg<FormatList> FormatList, DRCSMap *pDRCSMap)
{
	if (!DstString)
		return false;

	DstString->clear();

	if ((pSrcData == nullptr) || (SrcLength == 0))
		return false;

	const bool IsCaption = !!(Flags & DecodeFlag::Caption);
	const bool Is1Seg    = !!(Flags & DecodeFlag::OneSeg);
	const bool IsLatin   = !!(Flags & DecodeFlag::Latin);

	// 状態初期設定
	m_ESCSeqCount = 0;
	m_SingleGL = -1;

	m_CodeG[0] = CodeSet::Kanji;
	m_CodeG[1] = CodeSet::Alphanumeric;
	m_CodeG[2] = CodeSet::Hiragana;
	m_CodeG[3] = IsCaption ? CodeSet::Macro : CodeSet::Katakana;

	if (IsLatin) {
		m_CodeG[0] = CodeSet::Alphanumeric;
		m_CodeG[2] = CodeSet::LatinExtension;
		m_CodeG[3] = CodeSet::LatinSpecial;
		m_LockingGL = 0;
		m_LockingGR = 2;
	} else if (IsCaption && Is1Seg) {
		m_CodeG[1] = CodeSet::DRCS_1;
		m_LockingGL = 1;
		m_LockingGR = 0;
	} else {
		m_LockingGL = 0;
		m_LockingGR = 2;
	}

	m_CharSize = IsLatin ? CharSize::Medium : CharSize::Normal;
	if (IsCaption) {
		m_CharColorIndex = 7;
		m_BackColorIndex = 8;
		m_RasterColorIndex = 8;
	} else {
		m_CharColorIndex = 0;
		m_BackColorIndex = 0;
		m_RasterColorIndex = 0;
	}
	m_DefPalette = 0;
	m_RPC = 1;

	m_IsCaption = IsCaption;
	m_IsLatin = IsLatin;
	m_pFormatList = FormatList ? &*FormatList : nullptr;
	m_pDRCSMap = pDRCSMap;

	m_IsUCS         = !!(Flags & DecodeFlag::UCS);
	m_UseCharSize   = !!(Flags & DecodeFlag::UseCharSize);
	m_UnicodeSymbol = !!(Flags & DecodeFlag::UnicodeSymbol);

#if defined(LIBISDB_WINDOWS) && !defined(LIBISDB_WCHAR)
	// 一応 UNICODE でなくてもコンパイルできるようにはしているが、
	// 日本語ロケール(CP932)以外では多くの文字が変換できないため実用にならない
	InternalString String;
	const bool Result = DecodeString(pSrcData, SrcLength, &String);
	if (!String.empty()) {
		const int Length = ::WideCharToMultiByte(
			CP_ACP, 0, String.data(), static_cast<int>(String.length()), nullptr, 0, nullptr, nullptr);
		if (Length > 0) {
			DstString->resize(Length);
			::WideCharToMultiByte(
				CP_ACP, 0, String.data(), static_cast<int>(String.length()), DstString.data(), Length, nullptr, nullptr);
		}
	}
	return Result;
#else
	return DecodeString(pSrcData, SrcLength, &*DstString);
#endif
}


bool ARIBStringDecoder::DecodeString(const uint8_t *pSrcData, size_t SrcLength, InternalString *pDstString)
{
	for (size_t SrcPos = 0; SrcPos < SrcLength; SrcPos++) {
		if (m_ESCSeqCount == 0) {
			if (m_IsUCS && (((pSrcData[SrcPos] >= 0x21) && (pSrcData[SrcPos] <= 0x7E))
					|| ((pSrcData[SrcPos] >= 0x80)
						&& ((pSrcData[SrcPos] != 0xC2) || (SrcLength - SrcPos < 2)
							|| (pSrcData[SrcPos + 1] < 0x80) || (pSrcData[SrcPos + 1] >= 0xA1))))) {
				// UCSの制御コード以外
				if (pSrcData[SrcPos] >= 0xFE) {
					// UTF-16のBOM。未サポート
					return false;
				}
				size_t OldLength = pDstString->length();
				uint32_t CodePoint;
				const size_t CodeLength = UTF8ToCodePoint(pSrcData + SrcPos, SrcLength - SrcPos, &CodePoint);

				if (CodePoint == 0) {
					*pDstString += TOFU_STR;
				} else if ((CodePoint >= 0xEC00) && (CodePoint <= 0xF8FF)) {
					// U+EC00以降の私用領域はDRCS
					PutDRCSChar(static_cast<uint16_t>(CodePoint), pDstString);
#ifdef LIBISDB_ARIB_STR_IS_WCHAR
				} else if (CodePoint >= 0x10000) {
					// サロゲートペア
					*pDstString += static_cast<InternalChar>(0xD800 | ((CodePoint - 0x10000) >> 10));
					*pDstString += static_cast<InternalChar>(0xDC00 | ((CodePoint - 0x10000) & 0x03FF));
				} else {
					*pDstString += static_cast<InternalChar>(CodePoint);
#else
				} else {
					for (size_t i = 0; i < CodeLength; i++)
						*pDstString += static_cast<InternalChar>(pSrcData[SrcPos + i]);
#endif
				}
				for (; m_RPC > 1; m_RPC--) {
					const size_t Length = pDstString->length();
					*pDstString += pDstString->substr(OldLength);
					OldLength = Length;
				}
				SrcPos += CodeLength - 1;
			} else if (!m_IsUCS && (pSrcData[SrcPos] >= 0x21) && (pSrcData[SrcPos] <= 0x7E)) {
				// GL領域
				const CodeSet CurCodeSet = m_CodeG[(m_SingleGL >= 0) ? m_SingleGL : m_LockingGL];
				m_SingleGL = -1;

				if (IsDoubleByteCodeSet(CurCodeSet)) {
					// 2バイトコード
					if (SrcLength - SrcPos < 2)
						return false;
					DecodeChar(Load16(&pSrcData[SrcPos]), CurCodeSet, pDstString);
					SrcPos++;
				} else {
					// 1バイトコード
					DecodeChar(pSrcData[SrcPos], CurCodeSet, pDstString);
				}
			} else if (!m_IsUCS && (pSrcData[SrcPos] >= 0xA1) && (pSrcData[SrcPos] <= 0xFE)) {
				// GR領域
				const CodeSet CurCodeSet = m_CodeG[m_LockingGR];

				if (IsDoubleByteCodeSet(CurCodeSet)) {
					// 2バイトコード
					if (SrcLength - SrcPos < 2)
						return false;
					DecodeChar(Load16(&pSrcData[SrcPos]) & 0x7F7F, CurCodeSet, pDstString);
					SrcPos++;
				} else {
					// 1バイトコード
					DecodeChar(pSrcData[SrcPos] & 0x7F, CurCodeSet, pDstString);
				}
			} else {
				// 制御コード
				if (m_IsUCS && (pSrcData[SrcPos] == 0xC2)) {
					// UCSのC1制御コード
					SrcPos++;
				}
				switch (pSrcData[SrcPos]) {
				case 0x0D:	// APR
					*pDstString += ARIB_STR(LIBISDB_NEWLINE);
					break;
				case 0x0F:	// LS0
					m_LockingGL = 0;
					break;
				case 0x0E:	// LS1
					m_LockingGL = 1;
					break;
				case 0x19:	// SS2
					m_SingleGL = 2;
					break;
				case 0x1D:	// SS3
					m_SingleGL = 3;
					break;
				case 0x1B:	// ESC
					m_ESCSeqCount = 1;
					break;
				case 0x20:	// SP
					if (IsSmallCharMode()) {
						*pDstString += ARIB_STR(' ');
					} else {
						*pDstString += ARIB_STR("　");
					}
					break;
				case 0xA0:	// SP
					*pDstString += ARIB_STR(' ');
					break;

				case 0x80:
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x85:
				case 0x86:
				case 0x87:
					m_CharColorIndex = (m_DefPalette << 4) | (pSrcData[SrcPos] & 0x0F);
					SetFormat(pDstString->length());
					break;

				case 0x88:	// SSZ 小型
					m_CharSize = CharSize::Small;
					SetFormat(pDstString->length());
					break;
				case 0x89:	// MSZ 中型
					m_CharSize = CharSize::Medium;
					SetFormat(pDstString->length());
					break;
				case 0x8A:	// NSZ 標準
					m_CharSize = CharSize::Normal;
					SetFormat(pDstString->length());
					break;
				case 0x8B:	// SZX 指定サイズ
					if (++SrcPos >= SrcLength)
						return false;
					switch (pSrcData[SrcPos]) {
					case 0x60: m_CharSize = CharSize::Micro;    break;	// 超小型
					case 0x41: m_CharSize = CharSize::HighW;    break;	// 縦倍
					case 0x44: m_CharSize = CharSize::WidthW;   break;	// 横倍
					case 0x45: m_CharSize = CharSize::SizeW;    break;	// 縦横倍
					case 0x6B: m_CharSize = CharSize::Special1; break;	// 特殊1
					case 0x64: m_CharSize = CharSize::Special2; break;	// 特殊2
					}
					SetFormat(pDstString->length());
					break;

				case 0x0C:	// CS
					pDstString->push_back(ARIB_STR('\f'));
					break;
				case 0x16:	// PAPF
					SrcPos++;
					break;
				case 0x1C:	// APS
					SrcPos += 2;
					break;
				case 0x90:	// COL
					if (++SrcPos >= SrcLength)
						return false;
					if (pSrcData[SrcPos] == 0x20) {
						m_DefPalette = pSrcData[++SrcPos] & 0x0F;
					} else {
						switch (pSrcData[SrcPos] & 0xF0) {
						case 0x40:
							m_CharColorIndex = pSrcData[SrcPos] & 0x0F;
							break;
						case 0x50:
							m_BackColorIndex = pSrcData[SrcPos] & 0x0F;
							break;
						}
						SetFormat(pDstString->length());
					}
					break;
				case 0x91:	// FLC
					SrcPos++;
					break;
				case 0x93:	// POL
					SrcPos++;
					break;
				case 0x94:	// WMM
					SrcPos++;
					break;
				case 0x95:	// MACRO
					do {
						if (++SrcPos >= SrcLength)
							return false;
					} while (pSrcData[SrcPos] != 0x4F);
					break;
				case 0x97:	// HLC
					SrcPos++;
					break;
				case 0x98:	// RPC
					if (++SrcPos >= SrcLength)
						return false;
					m_RPC = pSrcData[SrcPos] & 0x3F;
					break;
				case 0x9B:	//CSI
					{
						int Length;
						for (Length = 0; (++SrcPos < SrcLength) && (pSrcData[SrcPos] <= 0x3B); Length++);
						if (SrcPos < SrcLength) {
							if (pSrcData[SrcPos] == 0x69) {	// ACS
								if (Length != 2)
									return false;
								if (pSrcData[SrcPos - 2] >= 0x32) {
									while ((++SrcPos < SrcLength) && (pSrcData[SrcPos] != 0x9B));
									SrcPos += 3;
								}
							}
						}
					}
					break;
				case 0x9D:	// TIME
					if (++SrcPos >= SrcLength)
						return false;
					if (pSrcData[SrcPos] == 0x20) {
						SrcPos++;
					} else {
						while ((SrcPos < SrcLength) && ((pSrcData[SrcPos] < 0x40) || (pSrcData[SrcPos] > 0x43)))
							SrcPos++;
					}
					break;

				default:	// 非対応
					break;
				}
			}
		} else {
			// エスケープシーケンス処理
			ProcessEscapeSeq(pSrcData[SrcPos]);
		}
	}

	return true;
}


void ARIBStringDecoder::DecodeChar(uint16_t Code, CodeSet Set, InternalString *pDstString)
{
	const size_t OldLength = pDstString->length();

	switch (Set) {
	case CodeSet::Kanji:
	case CodeSet::JIS_KanjiPlane1:
		// 漢字(1面)コード出力
		PutKanjiChar(Code, pDstString);
		break;

	case CodeSet::JIS_KanjiPlane2:
		// 漢字(2面)コード出力
		PutKanjiPlane2Char(Code, pDstString);
		break;

	case CodeSet::Alphanumeric:
	case CodeSet::ProportionalAlphanumeric:
		// 英数字コード出力
		PutAlphanumericChar(Code, pDstString);
		break;

	case CodeSet::Hiragana:
	case CodeSet::ProportionalHiragana:
		// ひらがなコード出力
		PutHiraganaChar(Code, pDstString);
		break;

	case CodeSet::Katakana:
	case CodeSet::ProportionalKatakana:
		// カタカナコード出力
		PutKatakanaChar(Code, pDstString);
		break;

	case CodeSet::JIS_X0201_Katakana:
		// JISカタカナコード出力
		PutJISKatakanaChar(Code, pDstString);
		break;

	case CodeSet::LatinExtension:
		// ラテン文字拡張コード出力
		PutLatinExtensionChar(Code, pDstString);
		break;

	case CodeSet::LatinSpecial:
		// ラテン文字特殊コード出力
		PutLatinSpecialChar(Code, pDstString);
		break;

	case CodeSet::AdditionalSymbols:
		// 追加シンボルコード出力
		PutSymbolsChar(Code, pDstString);
		break;

	case CodeSet::Macro:
		PutMacroChar(Code, pDstString);
		break;

	case CodeSet::DRCS_0:
		PutDRCSChar(Code, pDstString);
		break;

	case CodeSet::DRCS_1:
	case CodeSet::DRCS_2:
	case CodeSet::DRCS_3:
	case CodeSet::DRCS_4:
	case CodeSet::DRCS_5:
	case CodeSet::DRCS_6:
	case CodeSet::DRCS_7:
	case CodeSet::DRCS_8:
	case CodeSet::DRCS_9:
	case CodeSet::DRCS_10:
	case CodeSet::DRCS_11:
	case CodeSet::DRCS_12:
	case CodeSet::DRCS_13:
	case CodeSet::DRCS_14:
	case CodeSet::DRCS_15:
		PutDRCSChar(((static_cast<int>(Set) - static_cast<int>(CodeSet::DRCS_0) + 0x40) << 8) | Code, pDstString);
		break;

	default:
		*pDstString += TOFU_STR;
		break;
	}

	if ((m_RPC > 1) && (pDstString->length() > OldLength)) {
		const String Str = pDstString->substr(OldLength, pDstString->length() - OldLength);
		int Count = m_RPC;
		while (--Count > 0)
			*pDstString += Str;
	}
	m_RPC = 1;
}


void ARIBStringDecoder::PutKanjiChar(uint16_t Code, InternalString *pDstString)
{
	if (Code >= 0x7521)
		return PutSymbolsChar(Code, pDstString);

	const uint8_t First = Code >> 8, Second = Code & 0x00FF;

	// 全角 -> 半角英数字変換
	if (m_UseCharSize && (m_CharSize == CharSize::Medium)) {
		uint8_t AlnumCode = 0;

		if (First == 0x23) {
			if ((Second >= 0x30 && Second <= 0x39)
					|| (Second >= 0x41 && Second <= 0x5A)
					|| (Second >= 0x61 && Second <= 0x7A))
				AlnumCode = Second;
		} else if (First == 0x21) {
			static const struct {
				uint8_t From, To;
			} Map[] = {
				{0x21, 0x20}, {0x24, 0x2C}, {0x25, 0x2E}, {0x27, 0x3A},
				{0x28, 0x3B}, {0x29, 0x3F}, {0x2A, 0x21}, {0x2E, 0x60},
				{0x30, 0x5E}, {0x31, 0x7E}, {0x32, 0x5F}, {0x3F, 0x2F},
				{0x43, 0x7C}, {0x4A, 0x28}, {0x4B, 0x29}, {0x4E, 0x5B},
				{0x4F, 0x5D}, {0x50, 0x7B}, {0x51, 0x7D}, {0x5C, 0x2B},
				{0x61, 0x3D}, {0x63, 0x3C}, {0x64, 0x3E}, {0x6F, 0x5C},
				{0x70, 0x24}, {0x73, 0x25}, {0x74, 0x23}, {0x75, 0x26},
				{0x76, 0x2A}, {0x77, 0x40},
			};

			for (size_t i = 0; (i < std::size(Map)) && (Map[i].From <= Second); i++) {
				if (Map[i].From == Second) {
					AlnumCode = Map[i].To;
					break;
				}
			}
		}

		if (AlnumCode != 0)
			return PutAlphanumericChar(AlnumCode, pDstString);
	}

// Windows API を使って変換する場合
//#ifdef LIBISDB_WINDOWS
#if 0

	// JIS -> Shift_JIS 漢字コード変換
	First -= 0x21;
	if ((First & 0x01) == 0) {
		Second += 0x1F;
		if (Second >= 0x7F)
			Second++;
	} else {
		Second += 0x7E;
	}
	First >>= 1;
	if (First >= 0x1F)
		First += 0xC1;
	else
		First += 0x81;

	char ShiftJIS[2];
	ShiftJIS[0] = First;
	ShiftJIS[1] = Second;

	// Shift_JIS (CP932) -> UTF-16
	WCHAR Unicode[2];
	const int Length = ::MultiByteToWideChar(932, MB_PRECOMPOSED, ShiftJIS, 2, Unicode, static_cast<int>(std::size(Unicode)));
	if (Length > 0)
		pDstString->append(Unicode, Length);
	else
		pDstString->append(TOFU_STR);

#else

#ifdef LIBISDB_ARIB_STR_IS_UTF8
	// JIS -> UTF-8 漢字コード変換
	char Buffer[4];
	const size_t Length = JISX0213KanjiToUTF8(1, Code, Buffer, std::size(Buffer));
	if (Length > 0)
		pDstString->append(Buffer, Length);
	else
		pDstString->append(TOFU_STR);
#else
	// JIS -> wchar_t 漢字コード変換
	wchar_t Buffer[2];
	const size_t Length = JISX0213KanjiToWChar(1, Code, Buffer, std::size(Buffer));
	if (Length > 0)
		pDstString->append(Buffer, Length);
	else
		pDstString->append(TOFU_STR);
#endif

#endif
}


void ARIBStringDecoder::PutKanjiPlane2Char(uint16_t Code, InternalString *pDstString)
{
#ifdef LIBISDB_ARIB_STR_IS_UTF8
	// JIS X 0213 漢字集合2面 -> UTF-8
	char Buffer[4];
	const size_t Length = JISX0213KanjiToUTF8(2, Code, Buffer, std::size(Buffer));
	if (Length > 0)
		pDstString->append(Buffer, Length);
	else
		pDstString->append(TOFU_STR);
#else
	// JIS X 0213 漢字集合2面 -> wchar_t
	wchar_t Buffer[2];
	const size_t Length = JISX0213KanjiToWChar(2, Code, Buffer, std::size(Buffer));
	if (Length > 0)
		pDstString->append(Buffer, Length);
	else
		pDstString->append(TOFU_STR);
#endif
}


void ARIBStringDecoder::PutAlphanumericChar(uint16_t Code, InternalString *pDstString)
{
	// 英数字文字コード変換
	static const ARIBStrTableType AlphanumericTable[] =
		ARIB_STR_TABLE_BEGIN
		ARIB_STR_TABLE("　", "！", "”", "＃", "＄", "％", "＆", "’", "（", "）", "＊", "＋", "，", "－", "．", "／")
		ARIB_STR_TABLE("０", "１", "２", "３", "４", "５", "６", "７", "８", "９", "：", "；", "＜", "＝", "＞", "？")
		ARIB_STR_TABLE("＠", "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ", "Ｈ", "Ｉ", "Ｊ", "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ")
		ARIB_STR_TABLE("Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ", "Ｗ", "Ｘ", "Ｙ", "Ｚ", "［", "￥", "］", "＾", "＿")
		ARIB_STR_TABLE("｀", "ａ", "ｂ", "ｃ", "ｄ", "ｅ", "ｆ", "ｇ", "ｈ", "ｉ", "ｊ", "ｋ", "ｌ", "ｍ", "ｎ", "ｏ")
		ARIB_STR_TABLE("ｐ", "ｑ", "ｒ", "ｓ", "ｔ", "ｕ", "ｖ", "ｗ", "ｘ", "ｙ", "ｚ", "｛", "｜", "｝", "￣", "　")
		ARIB_STR_TABLE_END;
	static const ARIBStrTableType AlphanumericHalfWidthTable[] =
		ARIB_STR_TABLE_BEGIN
		ARIB_STR_TABLE(" ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/")
		ARIB_STR_TABLE("0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?")
		ARIB_STR_TABLE("@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O")
		ARIB_STR_TABLE("P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\u00a5", "]", "^", "_")
		ARIB_STR_TABLE("`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o")
		ARIB_STR_TABLE("p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "\u203e", " ")
		ARIB_STR_TABLE_END;

	const ARIBStrTableType * Table =
		(m_IsLatin || (m_UseCharSize && m_CharSize == CharSize::Medium)) ? AlphanumericHalfWidthTable : AlphanumericTable;

	*pDstString += Table[Code < 0x20 ? 0 : Code - 0x20];
}


void ARIBStringDecoder::PutHiraganaChar(uint16_t Code, InternalString *pDstString)
{
	// ひらがな文字コード変換
	static const ARIBStrTableType HiraganaTable[] =
		ARIB_STR_TABLE_BEGIN
		ARIB_STR_TABLE("　", "ぁ", "あ", "ぃ", "い", "ぅ", "う", "ぇ", "え", "ぉ", "お", "か", "が", "き", "ぎ", "く")
		ARIB_STR_TABLE("ぐ", "け", "げ", "こ", "ご", "さ", "ざ", "し", "じ", "す", "ず", "せ", "ぜ", "そ", "ぞ", "た")
		ARIB_STR_TABLE("だ", "ち", "ぢ", "っ", "つ", "づ", "て", "で", "と", "ど", "な", "に", "ぬ", "ね", "の", "は")
		ARIB_STR_TABLE("ば", "ぱ", "ひ", "び", "ぴ", "ふ", "ぶ", "ぷ", "へ", "べ", "ぺ", "ほ", "ぼ", "ぽ", "ま", "み")
		ARIB_STR_TABLE("む", "め", "も", "ゃ", "や", "ゅ", "ゆ", "ょ", "よ", "ら", "り", "る", "れ", "ろ", "ゎ", "わ")
		ARIB_STR_TABLE("ゐ", "ゑ", "を", "ん", "　", "　", "　", "ゝ", "ゞ", "ー", "。", "「", "」", "、", "・", "　")
		ARIB_STR_TABLE_END;

	*pDstString += HiraganaTable[Code < 0x20 ? 0 : Code - 0x20];
}


void ARIBStringDecoder::PutKatakanaChar(uint16_t Code, InternalString *pDstString)
{
	// カタカナ文字コード変換
	static const ARIBStrTableType KatakanaTable[] =
		ARIB_STR_TABLE_BEGIN
		ARIB_STR_TABLE("　", "ァ", "ア", "ィ", "イ", "ゥ", "ウ", "ェ", "エ", "ォ", "オ", "カ", "ガ", "キ", "ギ", "ク")
		ARIB_STR_TABLE("グ", "ケ", "ゲ", "コ", "ゴ", "サ", "ザ", "シ", "ジ", "ス", "ズ", "セ", "ゼ", "ソ", "ゾ", "タ")
		ARIB_STR_TABLE("ダ", "チ", "ヂ", "ッ", "ツ", "ヅ", "テ", "デ", "ト", "ド", "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ")
		ARIB_STR_TABLE("バ", "パ", "ヒ", "ビ", "ピ", "フ", "ブ", "プ", "ヘ", "ベ", "ペ", "ホ", "ボ", "ポ", "マ", "ミ")
		ARIB_STR_TABLE("ム", "メ", "モ", "ャ", "ヤ", "ュ", "ユ", "ョ", "ヨ", "ラ", "リ", "ル", "レ", "ロ", "ヮ", "ワ")
		ARIB_STR_TABLE("ヰ", "ヱ", "ヲ", "ン", "ヴ", "ヵ", "ヶ", "ヽ", "ヾ", "ー", "。", "「", "」", "、", "・", "　")
		ARIB_STR_TABLE_END;

	*pDstString += KatakanaTable[Code < 0x20 ? 0 : Code - 0x20];
}


void ARIBStringDecoder::PutJISKatakanaChar(uint16_t Code, InternalString *pDstString)
{
	// JISカタカナ文字コード変換
	static const ARIBStrTableType JISKatakanaTable[] =
		ARIB_STR_TABLE_BEGIN
		ARIB_STR_TABLE("　", "。", "「", "」", "、", "・", "ヲ", "ァ", "ィ", "ゥ", "ェ", "ォ", "ャ", "ュ", "ョ", "ッ")
		ARIB_STR_TABLE("ー", "ア", "イ", "ウ", "エ", "オ", "カ", "キ", "ク", "ケ", "コ", "サ", "シ", "ス", "セ", "ソ")
		ARIB_STR_TABLE("タ", "チ", "ツ", "テ", "ト", "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "ヒ", "フ", "ヘ", "ホ", "マ")
		ARIB_STR_TABLE("ミ", "ム", "メ", "モ", "ヤ", "ユ", "ヨ", "ラ", "リ", "ル", "レ", "ロ", "ワ", "ン", "゛", "゜")
		ARIB_STR_TABLE_END;

	*pDstString += JISKatakanaTable[(Code < 0x20 || Code >= 0x60) ? 0 : Code - 0x20];
}


void ARIBStringDecoder::PutLatinExtensionChar(uint16_t Code, InternalString *pDstString)
{
	// ラテン文字拡張コード変換
	static const ARIBStrTableType LatinExtensionTable[] =
		ARIB_STR_TABLE_BEGIN
		ARIB_STR_TABLE(" ", "\u00a1", "\u00a2", "\u00a3", "\u20ac", "\u00a5", "\u0160", "\u00a7", "\u0161", "\u00a9", "\u00aa", "\u00ab", "\u00ac", "\u00ff", "\u00ae", "\u00af")
		ARIB_STR_TABLE("\u00b0", "\u00b1", "\u00b2", "\u00b3", "\u017d", "\u03bc", "\u00b6", "\u00b7", "\u017e", "\u00b9", "\u00ba", "\u00bb", "\u0152", "\u0153", "\u0178", "\u00bf")
		ARIB_STR_TABLE("\u00c0", "\u00c1", "\u00c2", "\u00c3", "\u00c4", "\u00c5", "\u00c6", "\u00c7", "\u00c8", "\u00c9", "\u00ca", "\u00cb", "\u00cc", "\u00cd", "\u00ce", "\u00cf")
		ARIB_STR_TABLE("\u00d0", "\u00d1", "\u00d2", "\u00d3", "\u00d4", "\u00d5", "\u00d6", "\u00d7", "\u00d8", "\u00d9", "\u00da", "\u00db", "\u00dc", "\u00dd", "\u00de", "\u00df")
		ARIB_STR_TABLE("\u00e0", "\u00e1", "\u00e2", "\u00e3", "\u00e4", "\u00e5", "\u00e6", "\u00e7", "\u00e8", "\u00e9", "\u00ea", "\u00eb", "\u00ec", "\u00ed", "\u00ee", "\u00ef")
		ARIB_STR_TABLE("\u00f0", "\u00f1", "\u00f2", "\u00f3", "\u00f4", "\u00f5", "\u00f6", "\u00f7", "\u00f8", "\u00f9", "\u00fa", "\u00fb", "\u00fc", "\u00fd", "\u00fe", " ")
		ARIB_STR_TABLE_END;

	*pDstString += LatinExtensionTable[Code < 0x20 ? 0 : Code - 0x20];
}


void ARIBStringDecoder::PutLatinSpecialChar(uint16_t Code, InternalString *pDstString)
{
	// ラテン文字特殊コード変換
	static const ARIBStrTableType LatinSpecialTable[] =
		ARIB_STR_TABLE_BEGIN
		ARIB_STR_TABLE(" ", "\u266a", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " ")
		ARIB_STR_TABLE("\u00a4", "\u00a6", "\u00a8", "\u00b4", "\u00b8", "\u00bc", "\u00bd", "\u00be", " ", " ", " ", " ", " ", " ", " ", " ")
		ARIB_STR_TABLE("\u2026", "\u2588", "\u2018", "\u2019", "\u201c", "\u201d", "\u2022", "\u2122", "\u215b", "\u215c", "\u215d", "\u215e", " ", " ", " ", " ")
		ARIB_STR_TABLE_END;

	*pDstString += LatinSpecialTable[(Code < 0x20 || Code >= 0x50) ? 0 : Code - 0x20];
}


void ARIBStringDecoder::PutSymbolsChar(uint16_t Code, InternalString *pDstString)
{
	// 追加シンボル文字コード変換

	static const InternalChar * const SymbolsTable_90_01[] = {
		ARIB_STR("\u26cc"),     ARIB_STR("\u26cd"),     ARIB_STR("\u2757"),     ARIB_STR("\u26cf"),     // 0x7A21 - 0x7A24  90/01 - 90/04
		ARIB_STR("\u26d0"),     ARIB_STR("\u26d1"),     nullptr,                ARIB_STR("\u26d2"),     // 0x7A25 - 0x7A28  90/05 - 90/08
		ARIB_STR("\u26d5"),     ARIB_STR("\u26d3"),     ARIB_STR("\u26d4"),     nullptr,                // 0x7A29 - 0x7A2C  90/09 - 90/12
		nullptr,                nullptr,                nullptr,                ARIB_STR("\U0001f17f"), // 0x7A2D - 0x7A30  90/13 - 90/16
		ARIB_STR("\U0001f18a"), nullptr,                nullptr,                ARIB_STR("\u26d6"),     // 0x7A31 - 0x7A34  90/17 - 90/20
		ARIB_STR("\u26d7"),     ARIB_STR("\u26d8"),     ARIB_STR("\u26d9"),     ARIB_STR("\u26da"),     // 0x7A35 - 0x7A38  90/21 - 90/24
		ARIB_STR("\u26db"),     ARIB_STR("\u26dc"),     ARIB_STR("\u26dd"),     ARIB_STR("\u26de"),     // 0x7A39 - 0x7A3C  90/25 - 90/28
		ARIB_STR("\u26df"),     ARIB_STR("\u26e0"),     ARIB_STR("\u26e1"),     ARIB_STR("\u2b55"),     // 0x7A3D - 0x7A40  90/29 - 90/32
		ARIB_STR("\u3248"),     ARIB_STR("\u3249"),     ARIB_STR("\u324a"),     ARIB_STR("\u324b"),     // 0x7A41 - 0x7A44  90/33 - 90/36
		ARIB_STR("\u324c"),     ARIB_STR("\u324d"),     ARIB_STR("\u324e"),     ARIB_STR("\u324f"),     // 0x7A45 - 0x7A48  90/37 - 90/40
	};

	static const InternalChar * const SymbolsTable_90_45[] = {
		ARIB_STR("10."),        ARIB_STR("11."),        ARIB_STR("12."),        ARIB_STR("[HV]"),       // 0x7A4D - 0x7A50  90/45 - 90/48
		ARIB_STR("[SD]"),       ARIB_STR("[Ｐ]"),       ARIB_STR("[Ｗ]"),       ARIB_STR("[MV]"),       // 0x7A51 - 0x7A54  90/49 - 90/52
		ARIB_STR("[手]"),       ARIB_STR("[字]"),       ARIB_STR("[双]"),       ARIB_STR("[デ]"),       // 0x7A55 - 0x7A58  90/53 - 90/56
		ARIB_STR("[Ｓ]"),       ARIB_STR("[二]"),       ARIB_STR("[多]"),       ARIB_STR("[解]"),       // 0x7A59 - 0x7A5C  90/57 - 90/60
		ARIB_STR("[SS]"),       ARIB_STR("[Ｂ]"),       ARIB_STR("[Ｎ]"),       ARIB_STR("■"),         // 0x7A5D - 0x7A60  90/61 - 90/64
		ARIB_STR("●"),         ARIB_STR("[天]"),       ARIB_STR("[交]"),       ARIB_STR("[映]"),       // 0x7A61 - 0x7A64  90/65 - 90/68
		ARIB_STR("[無]"),       ARIB_STR("[料]"),       ARIB_STR("[年齢制限]"), ARIB_STR("[前]"),       // 0x7A65 - 0x7A68  90/69 - 90/72
		ARIB_STR("[後]"),       ARIB_STR("[再]"),       ARIB_STR("[新]"),       ARIB_STR("[初]"),       // 0x7A69 - 0x7A6C  90/73 - 90/76
		ARIB_STR("[終]"),       ARIB_STR("[生]"),       ARIB_STR("[販]"),       ARIB_STR("[声]"),       // 0x7A6D - 0x7A70  90/77 - 90/80
		ARIB_STR("[吹]"),       ARIB_STR("[PPV]"),      ARIB_STR("(秘)"),       ARIB_STR("ほか"),       // 0x7A71 - 0x7A74  90/81 - 90/84
	};
	static const InternalChar * const SymbolsTable_90_45_U[] = {
		ARIB_STR("\u2491"),     ARIB_STR("\u2492"),     ARIB_STR("\u2493"),     ARIB_STR("\U0001f14a"), // 0x7A4D - 0x7A50  90/45 - 90/48
		ARIB_STR("\U0001f14c"), ARIB_STR("\U0001f13f"), ARIB_STR("\U0001f146"), ARIB_STR("\U0001f14b"), // 0x7A51 - 0x7A54  90/49 - 90/52
		ARIB_STR("\U0001f210"), ARIB_STR("\U0001f211"), ARIB_STR("\U0001f212"), ARIB_STR("\U0001f213"), // 0x7A55 - 0x7A58  90/53 - 90/56
		ARIB_STR("\U0001f142"), ARIB_STR("\U0001f214"), ARIB_STR("\U0001f215"), ARIB_STR("\U0001f216"), // 0x7A59 - 0x7A5C  90/57 - 90/60
		ARIB_STR("\U0001f14d"), ARIB_STR("\U0001f131"), ARIB_STR("\U0001f13d"), ARIB_STR("\u2b1b"),     // 0x7A5D - 0x7A60  90/61 - 90/64
		ARIB_STR("\u2b24"),     ARIB_STR("\U0001f217"), ARIB_STR("\U0001f218"), ARIB_STR("\U0001f219"), // 0x7A61 - 0x7A64  90/65 - 90/68
		ARIB_STR("\U0001f21a"), ARIB_STR("\U0001f21b"), ARIB_STR("\u26bf"),     ARIB_STR("\U0001f21c"), // 0x7A65 - 0x7A68  90/69 - 90/72
		ARIB_STR("\U0001f21d"), ARIB_STR("\U0001f21e"), ARIB_STR("\U0001f21f"), ARIB_STR("\U0001f220"), // 0x7A69 - 0x7A6C  90/73 - 90/76
		ARIB_STR("\U0001f221"), ARIB_STR("\U0001f222"), ARIB_STR("\U0001f223"), ARIB_STR("\U0001f224"), // 0x7A6D - 0x7A70  90/77 - 90/80
		ARIB_STR("\U0001f225"), ARIB_STR("\U0001f14e"), ARIB_STR("\u3299"),     ARIB_STR("\U0001f200"), // 0x7A71 - 0x7A74  90/81 - 90/84
	};

	static const InternalChar * const SymbolsTable_91[] = {
		ARIB_STR("\u26e3"),     ARIB_STR("\u2b56"),     ARIB_STR("\u2b57"),     ARIB_STR("\u2b58"),     // 0x7B21 - 0x7B24  91/01 - 91/04
		ARIB_STR("\u2b59"),     ARIB_STR("\u2613"),     ARIB_STR("\u328b"),     ARIB_STR("\u3012"),     // 0x7B25 - 0x7B28  91/05 - 91/08
		ARIB_STR("\u26e8"),     ARIB_STR("\u3246"),     ARIB_STR("\u3245"),     ARIB_STR("\u26e9"),     // 0x7B29 - 0x7B2C  91/09 - 91/12
		ARIB_STR("\u0fd6"),     ARIB_STR("\u26ea"),     ARIB_STR("\u26eb"),     ARIB_STR("\u26ec"),     // 0x7B2D - 0x7B30  91/13 - 91/16
		ARIB_STR("\u2668"),     ARIB_STR("\u26ed"),     ARIB_STR("\u26ee"),     ARIB_STR("\u26ef"),     // 0x7B31 - 0x7B34  91/17 - 91/20
		ARIB_STR("\u2693"),     ARIB_STR("\u2708"),     ARIB_STR("\u26f0"),     ARIB_STR("\u26f1"),     // 0x7B35 - 0x7B38  91/21 - 91/24
		ARIB_STR("\u26f2"),     ARIB_STR("\u26f3"),     ARIB_STR("\u26f4"),     ARIB_STR("\u26f5"),     // 0x7B39 - 0x7B3C  91/25 - 91/28
		ARIB_STR("\U0001f157"), ARIB_STR("\u24b9"),     ARIB_STR("\u24c8"),     ARIB_STR("\u26f6"),     // 0x7B3D - 0x7B40  91/29 - 91/32
		ARIB_STR("\U0001f15f"), ARIB_STR("\U0001f18b"), ARIB_STR("\U0001f18d"), ARIB_STR("\U0001f18c"), // 0x7B41 - 0x7B44  91/33 - 91/36
		ARIB_STR("\U0001f179"), ARIB_STR("\u26f7"),     ARIB_STR("\u26f8"),     ARIB_STR("\u26f9"),     // 0x7B45 - 0x7B48  91/37 - 91/40
		ARIB_STR("\u26fa"),     ARIB_STR("\U0001f17b"), ARIB_STR("\u260e"),     ARIB_STR("\u26fb"),     // 0x7B49 - 0x7B4C  91/41 - 91/44
		ARIB_STR("\u26fc"),     ARIB_STR("\u26fd"),     ARIB_STR("\u26fe"),     ARIB_STR("\U0001f17c"), // 0x7B4D - 0x7B50  91/45 - 91/48
		ARIB_STR("\u26ff"),                                                                             // 0x7B51 - 0x7B51  91/49 - 91/49
	};

	static const InternalChar * const SymbolsTable_92[] = {
		ARIB_STR("→"),         ARIB_STR("←"),         ARIB_STR("↑"),         ARIB_STR("↓"),         // 0x7C21 - 0x7C24  92/01 - 92/04
		ARIB_STR("○"),         ARIB_STR("●"),         ARIB_STR("年"),         ARIB_STR("月"),         // 0x7C25 - 0x7C28  92/05 - 92/08
		ARIB_STR("日"),         ARIB_STR("円"),         ARIB_STR("㎡"),         ARIB_STR("立方ｍ"),     // 0x7C29 - 0x7C2C  92/09 - 92/12
		ARIB_STR("㎝"),         ARIB_STR("平方㎝"),     ARIB_STR("立方㎝"),     ARIB_STR("０."),        // 0x7C2D - 0x7C30  92/13 - 92/16
		ARIB_STR("１."),        ARIB_STR("２."),        ARIB_STR("３."),        ARIB_STR("４."),        // 0x7C31 - 0x7C34  92/17 - 92/20
		ARIB_STR("５."),        ARIB_STR("６."),        ARIB_STR("７."),        ARIB_STR("８."),        // 0x7C35 - 0x7C38  92/21 - 92/24
		ARIB_STR("９."),        ARIB_STR("氏"),         ARIB_STR("副"),         ARIB_STR("元"),         // 0x7C39 - 0x7C3C  92/25 - 92/28
		ARIB_STR("故"),         ARIB_STR("前"),         ARIB_STR("新"),         ARIB_STR("０,"),        // 0x7C3D - 0x7C40  92/29 - 92/32
		ARIB_STR("１,"),        ARIB_STR("２,"),        ARIB_STR("３,"),        ARIB_STR("４,"),        // 0x7C41 - 0x7C44  92/33 - 92/36
		ARIB_STR("５,"),        ARIB_STR("６,"),        ARIB_STR("７,"),        ARIB_STR("８,"),        // 0x7C45 - 0x7C48  92/37 - 92/40
		ARIB_STR("９,"),        ARIB_STR("(社)"),       ARIB_STR("(財)"),       ARIB_STR("(有)"),       // 0x7C49 - 0x7C4C  92/41 - 92/44
		ARIB_STR("(株)"),       ARIB_STR("(代)"),       ARIB_STR("(問)"),       ARIB_STR("＞"),         // 0x7C4D - 0x7C50  92/45 - 92/48
		ARIB_STR("＜"),         ARIB_STR("【"),         ARIB_STR("】"),         ARIB_STR("◇"),         // 0x7C51 - 0x7C54  92/49 - 92/52
		ARIB_STR("^2"),         ARIB_STR("^3"),         ARIB_STR("(CD)"),       ARIB_STR("(vn)"),       // 0x7C55 - 0x7C58  92/43 - 92/56
		ARIB_STR("(ob)"),       ARIB_STR("(cb)"),       ARIB_STR("(ce"),        ARIB_STR("mb)"),        // 0x7C59 - 0x7C5C  92/57 - 92/60
		ARIB_STR("(hp)"),       ARIB_STR("(br)"),       ARIB_STR("(p)"),        ARIB_STR("(s)"),        // 0x7C5D - 0x7C60  92/61 - 92/64
		ARIB_STR("(ms)"),       ARIB_STR("(t)"),        ARIB_STR("(bs)"),       ARIB_STR("(b)"),        // 0x7C61 - 0x7C64  92/65 - 92/68
		ARIB_STR("(tb)"),       ARIB_STR("(tp)"),       ARIB_STR("(ds)"),       ARIB_STR("(ag)"),       // 0x7C65 - 0x7C68  92/69 - 92/72
		ARIB_STR("(eg)"),       ARIB_STR("(vo)"),       ARIB_STR("(fl)"),       ARIB_STR("(ke"),        // 0x7C69 - 0x7C6C  92/73 - 92/76
		ARIB_STR("y)"),         ARIB_STR("(sa"),        ARIB_STR("x)"),         ARIB_STR("(sy"),        // 0x7C6D - 0x7C70  92/77 - 92/80
		ARIB_STR("n)"),         ARIB_STR("(or"),        ARIB_STR("g)"),         ARIB_STR("(pe"),        // 0x7C71 - 0x7C74  92/81 - 92/84
		ARIB_STR("r)"),         ARIB_STR("(R)"),        ARIB_STR("(C)"),        ARIB_STR("(箏)"),       // 0x7C75 - 0x7C78  92/85 - 92/88
		ARIB_STR("DJ"),         ARIB_STR("[演]"),       ARIB_STR("Fax"),                                // 0x7C79 - 0x7C7B  92/89 - 92/91
	};
	static const InternalChar * const SymbolsTable_92_U[] = {
		ARIB_STR("\u27a1"),     ARIB_STR("\u2b05"),     ARIB_STR("\u2b06"),     ARIB_STR("\u2b07"),     // 0x7C21 - 0x7C24  92/01 - 92/04
		ARIB_STR("\u2b2f"),     ARIB_STR("\u2b2e"),     ARIB_STR("年"),         ARIB_STR("月"),         // 0x7C25 - 0x7C28  92/05 - 92/08
		ARIB_STR("日"),         ARIB_STR("円"),         ARIB_STR("㎡"),         ARIB_STR("\u33a5"),     // 0x7C29 - 0x7C2C  92/09 - 92/12
		ARIB_STR("㎝"),         ARIB_STR("\u33a0"),     ARIB_STR("\u33a4"),     ARIB_STR("\U0001f100"), // 0x7C2D - 0x7C30  92/13 - 92/16
		ARIB_STR("\u2488"),     ARIB_STR("\u2489"),     ARIB_STR("\u248a"),     ARIB_STR("\u248b"),     // 0x7C31 - 0x7C34  92/17 - 92/20
		ARIB_STR("\u248c"),     ARIB_STR("\u248d"),     ARIB_STR("\u248e"),     ARIB_STR("\u248f"),     // 0x7C35 - 0x7C38  92/21 - 92/24
		ARIB_STR("\u2490"),     ARIB_STR("氏"),         ARIB_STR("副"),         ARIB_STR("元"),         // 0x7C39 - 0x7C3C  92/25 - 92/28
		ARIB_STR("故"),         ARIB_STR("前"),         ARIB_STR("新"),         ARIB_STR("\U0001f101"), // 0x7C3D - 0x7C40  92/29 - 92/32
		ARIB_STR("\U0001f102"), ARIB_STR("\U0001f103"), ARIB_STR("\U0001f104"), ARIB_STR("\U0001f105"), // 0x7C41 - 0x7C44  92/33 - 92/36
		ARIB_STR("\U0001f106"), ARIB_STR("\U0001f107"), ARIB_STR("\U0001f108"), ARIB_STR("\U0001f109"), // 0x7C45 - 0x7C48  92/37 - 92/40
		ARIB_STR("\U0001f10a"), ARIB_STR("\u3233"),     ARIB_STR("\u3236"),     ARIB_STR("\u3232"),     // 0x7C49 - 0x7C4C  92/41 - 92/44
		ARIB_STR("\u3231"),     ARIB_STR("\u3239"),     ARIB_STR("\u3244"),     ARIB_STR("\u25b6"),     // 0x7C4D - 0x7C50  92/45 - 92/48
		ARIB_STR("\u25c0"),     ARIB_STR("\u3016"),     ARIB_STR("\u3017"),     ARIB_STR("\u27d0"),     // 0x7C51 - 0x7C54  92/49 - 92/52
		ARIB_STR("\u00b2"),     ARIB_STR("\u00b3"),     ARIB_STR("\U0001f12d"), ARIB_STR("(vn)"),       // 0x7C55 - 0x7C58  92/53 - 92/56
		ARIB_STR("(ob)"),       ARIB_STR("(cb)"),       ARIB_STR("(ce"),        ARIB_STR("mb)"),        // 0x7C59 - 0x7C5C  92/57 - 92/60
		ARIB_STR("(hp)"),       ARIB_STR("(br)"),       ARIB_STR("(p)"),        ARIB_STR("(s)"),        // 0x7C5D - 0x7C60  92/61 - 92/64
		ARIB_STR("(ms)"),       ARIB_STR("(t)"),        ARIB_STR("(bs)"),       ARIB_STR("(b)"),        // 0x7C61 - 0x7C64  92/65 - 92/68
		ARIB_STR("(tb)"),       ARIB_STR("(tp)"),       ARIB_STR("(ds)"),       ARIB_STR("(ag)"),       // 0x7C65 - 0x7C68  92/69 - 92/72
		ARIB_STR("(eg)"),       ARIB_STR("(vo)"),       ARIB_STR("(fl)"),       ARIB_STR("(ke"),        // 0x7C69 - 0x7C6C  92/73 - 92/76
		ARIB_STR("y)"),         ARIB_STR("(sa"),        ARIB_STR("x)"),         ARIB_STR("(sy"),        // 0x7C6D - 0x7C70  92/77 - 92/80
		ARIB_STR("n)"),         ARIB_STR("(or"),        ARIB_STR("g)"),         ARIB_STR("(pe"),        // 0x7C71 - 0x7C74  92/81 - 92/84
		ARIB_STR("r)"),         ARIB_STR("\U0001f12c"), ARIB_STR("\U0001f12b"), ARIB_STR("\u3247"),     // 0x7C75 - 0x7C78  92/85 - 92/88
		ARIB_STR("\U0001f190"), ARIB_STR("\U0001f226"), ARIB_STR("\u213b"),                             // 0x7C79 - 0x7C7B  92/89 - 92/91
	};

	static const InternalChar * const SymbolsTable_93[] = {
		ARIB_STR("(月)"),       ARIB_STR("(火)"),       ARIB_STR("(水)"),       ARIB_STR("(木)"),       // 0x7D21 - 0x7D24  93/01 - 93/04
		ARIB_STR("(金)"),       ARIB_STR("(土)"),       ARIB_STR("(日)"),       ARIB_STR("(祝)"),       // 0x7D25 - 0x7D28  93/05 - 93/08
		ARIB_STR("㍾"),         ARIB_STR("㍽"),         ARIB_STR("㍼"),         ARIB_STR("㍻"),         // 0x7D29 - 0x7D2C  93/09 - 93/12
		ARIB_STR("№"),         ARIB_STR("℡"),         ARIB_STR("(〒)"),       ARIB_STR("○"),         // 0x7D2D - 0x7D30  93/13 - 93/16
		ARIB_STR("〔本〕"),     ARIB_STR("〔三〕"),     ARIB_STR("〔二〕"),     ARIB_STR("〔安〕"),     // 0x7D31 - 0x7D34  93/17 - 93/20
		ARIB_STR("〔点〕"),     ARIB_STR("〔打〕"),     ARIB_STR("〔盗〕"),     ARIB_STR("〔勝〕"),     // 0x7D35 - 0x7D38  93/21 - 93/24
		ARIB_STR("〔敗〕"),     ARIB_STR("〔Ｓ〕"),     ARIB_STR("［投］"),     ARIB_STR("［捕］"),     // 0x7D39 - 0x7D3C  93/25 - 93/28
		ARIB_STR("［一］"),     ARIB_STR("［二］"),     ARIB_STR("［三］"),     ARIB_STR("［遊］"),     // 0x7D3D - 0x7D40  93/29 - 93/32
		ARIB_STR("［左］"),     ARIB_STR("［中］"),     ARIB_STR("［右］"),     ARIB_STR("［指］"),     // 0x7D41 - 0x7D44  93/33 - 93/46
		ARIB_STR("［走］"),     ARIB_STR("［打］"),     ARIB_STR("㍑"),         ARIB_STR("㎏"),         // 0x7D45 - 0x7D48  93/37 - 93/40
		ARIB_STR("Hz"),         ARIB_STR("ha"),         ARIB_STR("km"),         ARIB_STR("平方km"),     // 0x7D49 - 0x7D4C  93/41 - 93/44
		ARIB_STR("hPa"),        nullptr,                nullptr,                ARIB_STR("1/2"),        // 0x7D4D - 0x7D50  93/45 - 93/48
		ARIB_STR("0/3"),        ARIB_STR("1/3"),        ARIB_STR("2/3"),        ARIB_STR("1/4"),        // 0x7D51 - 0x7D54  93/49 - 93/52
		ARIB_STR("3/4"),        ARIB_STR("1/5"),        ARIB_STR("2/5"),        ARIB_STR("3/5"),        // 0x7D55 - 0x7D58  93/53 - 93/56
		ARIB_STR("4/5"),        ARIB_STR("1/6"),        ARIB_STR("5/6"),        ARIB_STR("1/7"),        // 0x7D59 - 0x7D5C  93/57 - 93/60
		ARIB_STR("1/8"),        ARIB_STR("1/9"),        ARIB_STR("1/10"),       ARIB_STR("晴れ"),       // 0x7D5D - 0x7D60  93/61 - 93/64
		ARIB_STR("曇り"),       ARIB_STR("雨"),         ARIB_STR("雪"),         ARIB_STR("△"),         // 0x7D61 - 0x7D64  93/65 - 93/68
		ARIB_STR("▲"),         ARIB_STR("▽"),         ARIB_STR("▼"),         ARIB_STR("◆"),         // 0x7D65 - 0x7D68  93/69 - 93/72
		ARIB_STR("・"),         ARIB_STR("・"),         ARIB_STR("・"),         ARIB_STR("◇"),         // 0x7D69 - 0x7D6C  93/73 - 93/76
		ARIB_STR("◎"),         ARIB_STR("!!"),         ARIB_STR("!?"),         ARIB_STR("曇/晴"),      // 0x7D6D - 0x7D70  93/77 - 93/80
		ARIB_STR("雨"),         ARIB_STR("雨"),         ARIB_STR("雪"),         ARIB_STR("大雪"),       // 0x7D71 - 0x7D74  93/81 - 93/84
		ARIB_STR("雷"),         ARIB_STR("雷雨"),       ARIB_STR("　"),         ARIB_STR("・"),         // 0x7D75 - 0x7D78  93/85 - 93/88
		ARIB_STR("・"),         ARIB_STR("♪"),         ARIB_STR("℡"),                                 // 0x7D79 - 0x7D7B  93/89 - 93/91
	};
	static const InternalChar * const SymbolsTable_93_U[] = {
		ARIB_STR("\u322a"),     ARIB_STR("\u322b"),     ARIB_STR("\u322c"),     ARIB_STR("\u322d"),     // 0x7D21 - 0x7D24  93/01 - 93/04
		ARIB_STR("\u322e"),     ARIB_STR("\u322f"),     ARIB_STR("\u3230"),     ARIB_STR("\u3237"),     // 0x7D25 - 0x7D28  93/05 - 93/08
		ARIB_STR("㍾"),         ARIB_STR("㍽"),         ARIB_STR("㍼"),         ARIB_STR("㍻"),         // 0x7D29 - 0x7D2C  93/09 - 93/12
		ARIB_STR("№"),         ARIB_STR("℡"),         ARIB_STR("\u3036"),     ARIB_STR("\u26be"),     // 0x7D2D - 0x7D30  93/13 - 93/16
		ARIB_STR("\U0001f240"), ARIB_STR("\U0001f241"), ARIB_STR("\U0001f242"), ARIB_STR("\U0001f243"), // 0x7D31 - 0x7D34  93/17 - 93/20
		ARIB_STR("\U0001f244"), ARIB_STR("\U0001f245"), ARIB_STR("\U0001f246"), ARIB_STR("\U0001f247"), // 0x7D35 - 0x7D38  93/21 - 93/24
		ARIB_STR("\U0001f248"), ARIB_STR("\U0001f12a"), ARIB_STR("\U0001f227"), ARIB_STR("\U0001f228"), // 0x7D39 - 0x7D3C  93/25 - 93/28
		ARIB_STR("\U0001f229"), ARIB_STR("\U0001f214"), ARIB_STR("\U0001f22a"), ARIB_STR("\U0001f22b"), // 0x7D3D - 0x7D40  93/29 - 93/32
		ARIB_STR("\U0001f22c"), ARIB_STR("\U0001f22d"), ARIB_STR("\U0001f22e"), ARIB_STR("\U0001f22f"), // 0x7D41 - 0x7D44  93/33 - 93/36
		ARIB_STR("\U0001f230"), ARIB_STR("\U0001f231"), ARIB_STR("\u2113"),     ARIB_STR("㎏"),         // 0x7D45 - 0x7D48  93/37 - 93/40
		ARIB_STR("\u3390"),     ARIB_STR("\u33ca"),     ARIB_STR("\u339e"),     ARIB_STR("\u33a2"),     // 0x7D49 - 0x7D4C  93/41 - 93/44
		ARIB_STR("\u3371"),     nullptr,                nullptr,                ARIB_STR("\u00bd"),     // 0x7D4D - 0x7D50  93/45 - 93/48
		ARIB_STR("\u2189"),     ARIB_STR("\u2153"),     ARIB_STR("\u2154"),     ARIB_STR("\u00bc"),     // 0x7D51 - 0x7D54  93/49 - 93/52
		ARIB_STR("\u00be"),     ARIB_STR("\u2155"),     ARIB_STR("\u2156"),     ARIB_STR("\u2157"),     // 0x7D55 - 0x7D58  93/53 - 93/56
		ARIB_STR("\u2158"),     ARIB_STR("\u2159"),     ARIB_STR("\u215a"),     ARIB_STR("\u2150"),     // 0x7D59 - 0x7D5C  93/57 - 93/60
		ARIB_STR("\u215b"),     ARIB_STR("\u2151"),     ARIB_STR("\u2152"),     ARIB_STR("\u2600"),     // 0x7D5D - 0x7D60  93/61 - 93/64
		ARIB_STR("\u2601"),     ARIB_STR("\u2602"),     ARIB_STR("\u26c4"),     ARIB_STR("\u2616"),     // 0x7D61 - 0x7D64  93/65 - 93/68
		ARIB_STR("\u2617"),     ARIB_STR("\u26c9"),     ARIB_STR("\u26ca"),     ARIB_STR("\u2666"),     // 0x7D65 - 0x7D68  93/69 - 93/72
		ARIB_STR("\u2665"),     ARIB_STR("\u2663"),     ARIB_STR("\u2660"),     ARIB_STR("\u26cb"),     // 0x7D69 - 0x7D6C  93/73 - 93/76
		ARIB_STR("\u2a00"),     ARIB_STR("\u203c"),     ARIB_STR("\u2049"),     ARIB_STR("\u26c5"),     // 0x7D6D - 0x7D70  93/77 - 93/80
		ARIB_STR("\u2614"),     ARIB_STR("\u26c6"),     ARIB_STR("\u2603"),     ARIB_STR("\u26c7"),     // 0x7D71 - 0x7D74  93/81 - 93/84
		ARIB_STR("\u26a1"),     ARIB_STR("\u26c8"),     ARIB_STR("　"),         ARIB_STR("\u269e"),     // 0x7D75 - 0x7D78  93/85 - 93/88
		ARIB_STR("\u269f"),     ARIB_STR("\u266c"),     ARIB_STR("\u260e"),                             // 0x7D79 - 0x7D7B  93/89 - 93/91
	};

	static const InternalChar * const SymbolsTable_94[] = {
		ARIB_STR("Ⅰ"),         ARIB_STR("Ⅱ"),         ARIB_STR("Ⅲ"),         ARIB_STR("Ⅳ"),         // 0x7E21 - 0x7E24  94/01 - 94/04
		ARIB_STR("Ⅴ"),         ARIB_STR("Ⅵ"),         ARIB_STR("Ⅶ"),         ARIB_STR("Ⅷ"),         // 0x7E25 - 0x7E28  94/05 - 94/08
		ARIB_STR("Ⅸ"),         ARIB_STR("Ⅹ"),         ARIB_STR("XI"),         ARIB_STR("XⅡ"),        // 0x7E29 - 0x7E2C  94/09 - 94/12
		ARIB_STR("⑰"),         ARIB_STR("⑱"),         ARIB_STR("⑲"),         ARIB_STR("⑳"),         // 0x7E2D - 0x7E30  94/13 - 94/16
		ARIB_STR("(1)"),        ARIB_STR("(2)"),        ARIB_STR("(3)"),        ARIB_STR("(4)"),        // 0x7E31 - 0x7E34  94/17 - 94/20
		ARIB_STR("(5)"),        ARIB_STR("(6)"),        ARIB_STR("(7)"),        ARIB_STR("(8)"),        // 0x7E35 - 0x7E38  94/21 - 94/24
		ARIB_STR("(9)"),        ARIB_STR("(10)"),       ARIB_STR("(11)"),       ARIB_STR("(12)"),       // 0x7E39 - 0x7E3C  94/25 - 94/28
		ARIB_STR("(21)"),       ARIB_STR("(22)"),       ARIB_STR("(23)"),       ARIB_STR("(24)"),       // 0x7E3D - 0x7E40  94/29 - 94/32
		ARIB_STR("(A)"),        ARIB_STR("(B)"),        ARIB_STR("(C)"),        ARIB_STR("(D)"),        // 0x7E41 - 0x7E44  94/33 - 94/36
		ARIB_STR("(E)"),        ARIB_STR("(F)"),        ARIB_STR("(G)"),        ARIB_STR("(H)"),        // 0x7E45 - 0x7E48  94/37 - 94/40
		ARIB_STR("(I)"),        ARIB_STR("(J)"),        ARIB_STR("(K)"),        ARIB_STR("(L)"),        // 0x7E49 - 0x7E4C  94/41 - 94/44
		ARIB_STR("(M)"),        ARIB_STR("(N)"),        ARIB_STR("(O)"),        ARIB_STR("(P)"),        // 0x7E4D - 0x7E50  94/45 - 94/48
		ARIB_STR("(Q)"),        ARIB_STR("(R)"),        ARIB_STR("(S)"),        ARIB_STR("(T)"),        // 0x7E51 - 0x7E54  94/49 - 94/52
		ARIB_STR("(U)"),        ARIB_STR("(V)"),        ARIB_STR("(W)"),        ARIB_STR("(X)"),        // 0x7E55 - 0x7E58  94/53 - 94/56
		ARIB_STR("(Y)"),        ARIB_STR("(Z)"),        ARIB_STR("(25)"),       ARIB_STR("(26)"),       // 0x7E59 - 0x7E5C  94/57 - 94/60
		ARIB_STR("(27)"),       ARIB_STR("(28)"),       ARIB_STR("(29)"),       ARIB_STR("(30)"),       // 0x7E5D - 0x7E60  94/61 - 94/64
		ARIB_STR("①"),         ARIB_STR("②"),         ARIB_STR("③"),         ARIB_STR("④"),         // 0x7E61 - 0x7E64  94/65 - 94/68
		ARIB_STR("⑤"),         ARIB_STR("⑥"),         ARIB_STR("⑦"),         ARIB_STR("⑧"),         // 0x7E65 - 0x7E68  94/69 - 94/72
		ARIB_STR("⑨"),         ARIB_STR("⑩"),         ARIB_STR("⑪"),         ARIB_STR("⑫"),         // 0x7E69 - 0x7E6C  94/73 - 94/76
		ARIB_STR("⑬"),         ARIB_STR("⑭"),         ARIB_STR("⑮"),         ARIB_STR("⑯"),         // 0x7E6D - 0x7E70  94/77 - 94/80
		ARIB_STR("①"),         ARIB_STR("②"),         ARIB_STR("③"),         ARIB_STR("④"),         // 0x7E71 - 0x7E74  94/81 - 94/84
		ARIB_STR("⑤"),         ARIB_STR("⑥"),         ARIB_STR("⑦"),         ARIB_STR("⑧"),         // 0x7E75 - 0x7E78  94/85 - 94/88
		ARIB_STR("⑨"),         ARIB_STR("⑩"),         ARIB_STR("⑪"),         ARIB_STR("⑫"),         // 0x7E79 - 0x7E7C  94/89 - 94/92
		ARIB_STR("(31)"),                                                                               // 0x7E7D - 0x7E7D  94/93 - 94/93
	};
	static const InternalChar * const SymbolsTable_94_U[] = {
		ARIB_STR("Ⅰ"),         ARIB_STR("Ⅱ"),         ARIB_STR("Ⅲ"),         ARIB_STR("Ⅳ"),         // 0x7E21 - 0x7E24  94/01 - 94/04
		ARIB_STR("Ⅴ"),         ARIB_STR("Ⅵ"),         ARIB_STR("Ⅶ"),         ARIB_STR("Ⅷ"),         // 0x7E25 - 0x7E28  94/05 - 94/08
		ARIB_STR("Ⅸ"),         ARIB_STR("Ⅹ"),         ARIB_STR("\u216a"),     ARIB_STR("\u216b"),     // 0x7E29 - 0x7E2C  94/09 - 94/12
		ARIB_STR("⑰"),         ARIB_STR("⑱"),         ARIB_STR("⑲"),         ARIB_STR("⑳"),         // 0x7E2D - 0x7E30  94/13 - 94/16
		ARIB_STR("\u2474"),     ARIB_STR("\u2475"),     ARIB_STR("\u2476"),     ARIB_STR("\u2477"),     // 0x7E31 - 0x7E34  94/17 - 94/20
		ARIB_STR("\u2478"),     ARIB_STR("\u2479"),     ARIB_STR("\u247a"),     ARIB_STR("\u247b"),     // 0x7E35 - 0x7E38  94/21 - 94/24
		ARIB_STR("\u247c"),     ARIB_STR("\u247d"),     ARIB_STR("\u247e"),     ARIB_STR("\u247f"),     // 0x7E39 - 0x7E3C  94/25 - 94/28
		ARIB_STR("\u3251"),     ARIB_STR("\u3252"),     ARIB_STR("\u3253"),     ARIB_STR("\u3254"),     // 0x7E3D - 0x7E40  94/29 - 94/32
		ARIB_STR("\U0001f110"), ARIB_STR("\U0001f111"), ARIB_STR("\U0001f112"), ARIB_STR("\U0001f113"), // 0x7E41 - 0x7E44  94/33 - 94/36
		ARIB_STR("\U0001f114"), ARIB_STR("\U0001f115"), ARIB_STR("\U0001f116"), ARIB_STR("\U0001f117"), // 0x7E45 - 0x7E48  94/37 - 94/40
		ARIB_STR("\U0001f118"), ARIB_STR("\U0001f119"), ARIB_STR("\U0001f11a"), ARIB_STR("\U0001f11b"), // 0x7E49 - 0x7E4C  94/41 - 94/44
		ARIB_STR("\U0001f11c"), ARIB_STR("\U0001f11d"), ARIB_STR("\U0001f11e"), ARIB_STR("\U0001f11f"), // 0x7E4D - 0x7E50  94/45 - 94/48
		ARIB_STR("\U0001f120"), ARIB_STR("\U0001f121"), ARIB_STR("\U0001f122"), ARIB_STR("\U0001f123"), // 0x7E51 - 0x7E54  94/49 - 94/52
		ARIB_STR("\U0001f124"), ARIB_STR("\U0001f125"), ARIB_STR("\U0001f126"), ARIB_STR("\U0001f127"), // 0x7E55 - 0x7E58  94/53 - 94/56
		ARIB_STR("\U0001f128"), ARIB_STR("\U0001f129"), ARIB_STR("\u3255"),     ARIB_STR("\u3256"),     // 0x7E59 - 0x7E5C  94/57 - 94/60
		ARIB_STR("\u3257"),     ARIB_STR("\u3258"),     ARIB_STR("\u3259"),     ARIB_STR("\u325a"),     // 0x7E5D - 0x7E60  94/61 - 94/64
		ARIB_STR("①"),         ARIB_STR("②"),         ARIB_STR("③"),         ARIB_STR("④"),         // 0x7E61 - 0x7E64  94/65 - 94/68
		ARIB_STR("⑤"),         ARIB_STR("⑥"),         ARIB_STR("⑦"),         ARIB_STR("⑧"),         // 0x7E65 - 0x7E68  94/69 - 94/72
		ARIB_STR("⑨"),         ARIB_STR("⑩"),         ARIB_STR("⑪"),         ARIB_STR("⑫"),         // 0x7E69 - 0x7E6C  94/73 - 94/76
		ARIB_STR("⑬"),         ARIB_STR("⑭"),         ARIB_STR("⑮"),         ARIB_STR("⑯"),         // 0x7E6D - 0x7E70  94/77 - 94/80
		ARIB_STR("\u2776"),     ARIB_STR("\u2777"),     ARIB_STR("\u2778"),     ARIB_STR("\u2779"),     // 0x7E71 - 0x7E74  94/81 - 94/84
		ARIB_STR("\u277a"),     ARIB_STR("\u277b"),     ARIB_STR("\u277c"),     ARIB_STR("\u277d"),     // 0x7E75 - 0x7E78  94/85 - 94/88
		ARIB_STR("\u277e"),     ARIB_STR("\u277f"),     ARIB_STR("\u24eb"),     ARIB_STR("\u24ec"),     // 0x7E79 - 0x7E7C  94/89 - 94/92
		ARIB_STR("\u325b"),                                                                             // 0x7E7D - 0x7E7D  94/93 - 94/93
	};

	static const InternalChar * const KanjiTable1[] = {
		ARIB_STR("\u3402"),     ARIB_STR("\U00020158"), ARIB_STR("\u4efd"),     ARIB_STR("\u4eff"),     // 0x7521 - 0x7524
		ARIB_STR("\u4f9a"),     ARIB_STR("\u4fc9"),     ARIB_STR("\u509c"),     ARIB_STR("\u511e"),     // 0x7525 - 0x7528
		ARIB_STR("\u51bc"),     ARIB_STR("\u351f"),     ARIB_STR("\u5307"),     ARIB_STR("\u5361"),     // 0x7529 - 0x752C
		ARIB_STR("\u536c"),     ARIB_STR("\u8a79"),     ARIB_STR("\U00020bb7"), ARIB_STR("\u544d"),     // 0x752D - 0x7530
		ARIB_STR("\u5496"),     ARIB_STR("\u549c"),     ARIB_STR("\u54a9"),     ARIB_STR("\u550e"),     // 0x7531 - 0x7534
		ARIB_STR("\u554a"),     ARIB_STR("\u5672"),     ARIB_STR("\u56e4"),     ARIB_STR("\u5733"),     // 0x7535 - 0x7538
		ARIB_STR("\u5734"),     ARIB_STR("\ufa10"),     ARIB_STR("\u5880"),     ARIB_STR("\u59e4"),     // 0x7539 - 0x753C
		ARIB_STR("\u5a23"),     ARIB_STR("\u5a55"),     ARIB_STR("\u5bec"),     ARIB_STR("\ufa11"),     // 0x753D - 0x7540
		ARIB_STR("\u37e2"),     ARIB_STR("\u5eac"),     ARIB_STR("\u5f34"),     ARIB_STR("\u5f45"),     // 0x7541 - 0x7544
		ARIB_STR("\u5fb7"),     ARIB_STR("\u6017"),     ARIB_STR("\ufa6b"),     ARIB_STR("\u6130"),     // 0x7545 - 0x7548
		ARIB_STR("\u6624"),     ARIB_STR("\u66c8"),     ARIB_STR("\u66d9"),     ARIB_STR("\u66fa"),     // 0x7549 - 0x754C
		ARIB_STR("\u66fb"),     ARIB_STR("\u6852"),     ARIB_STR("\u9fc4"),     ARIB_STR("\u6911"),     // 0x754D - 0x7550
		ARIB_STR("\u693b"),     ARIB_STR("\u6a45"),     ARIB_STR("\u6a91"),     ARIB_STR("\u6adb"),     // 0x7551 - 0x7554
		ARIB_STR("\U000233cc"), ARIB_STR("\U000233fe"), ARIB_STR("\U000235c4"), ARIB_STR("\u6bf1"),     // 0x7555 - 0x7558
		ARIB_STR("\u6ce0"),     ARIB_STR("\u6d2e"),     ARIB_STR("\ufa45"),     ARIB_STR("\u6dbf"),     // 0x7559 - 0x755C
		ARIB_STR("\u6dca"),     ARIB_STR("\u6df8"),     ARIB_STR("\ufa46"),     ARIB_STR("\u6f5e"),     // 0x755D - 0x7560
		ARIB_STR("\u6ff9"),     ARIB_STR("\u7064"),     ARIB_STR("\ufa6c"),     ARIB_STR("\U000242ee"), // 0x7561 - 0x7564
		ARIB_STR("\u7147"),     ARIB_STR("\u71c1"),     ARIB_STR("\u7200"),     ARIB_STR("\u739f"),     // 0x7565 - 0x7568
		ARIB_STR("\u73a8"),     ARIB_STR("\u73c9"),     ARIB_STR("\u73d6"),     ARIB_STR("\u741b"),     // 0x7569 - 0x756C
		ARIB_STR("\u7421"),     ARIB_STR("\ufa4a"),     ARIB_STR("\u7426"),     ARIB_STR("\u742a"),     // 0x756D - 0x7570
		ARIB_STR("\u742c"),     ARIB_STR("\u7439"),     ARIB_STR("\u744b"),     ARIB_STR("\u3eda"),     // 0x7571 - 0x7574
		ARIB_STR("\u7575"),     ARIB_STR("\u7581"),     ARIB_STR("\u7772"),     ARIB_STR("\u4093"),     // 0x7575 - 0x7578
		ARIB_STR("\u78c8"),     ARIB_STR("\u78e0"),     ARIB_STR("\u7947"),     ARIB_STR("\u79ae"),     // 0x7579 - 0x757C
		ARIB_STR("\u9fc6"),     ARIB_STR("\u4103"),                                                     // 0x757D - 0x757E
	};
	static const InternalChar * const KanjiTable2[] = {
		ARIB_STR("\u9fc5"),     ARIB_STR("\u79da"),     ARIB_STR("\u7a1e"),     ARIB_STR("\u7b7f"),     // 0x7621 - 0x7624
		ARIB_STR("\u7c31"),     ARIB_STR("\u4264"),     ARIB_STR("\u7d8b"),     ARIB_STR("\u7fa1"),     // 0x7625 - 0x7628
		ARIB_STR("\u8118"),     ARIB_STR("\u813a"),     ARIB_STR("\ufa6d"),     ARIB_STR("\u82ae"),     // 0x7629 - 0x762C
		ARIB_STR("\u845b"),     ARIB_STR("\u84dc"),     ARIB_STR("\u84ec"),     ARIB_STR("\u8559"),     // 0x762D - 0x7630
		ARIB_STR("\u85ce"),     ARIB_STR("\u8755"),     ARIB_STR("\u87ec"),     ARIB_STR("\u880b"),     // 0x7631 - 0x7634
		ARIB_STR("\u88f5"),     ARIB_STR("\u89d2"),     ARIB_STR("\u8af6"),     ARIB_STR("\u8dce"),     // 0x7635 - 0x7638
		ARIB_STR("\u8fbb"),     ARIB_STR("\u8ff6"),     ARIB_STR("\u90dd"),     ARIB_STR("\u9127"),     // 0x7639 - 0x763C
		ARIB_STR("\u912d"),     ARIB_STR("\u91b2"),     ARIB_STR("\u9233"),     ARIB_STR("\u9288"),     // 0x763D - 0x7640
		ARIB_STR("\u9321"),     ARIB_STR("\u9348"),     ARIB_STR("\u9592"),     ARIB_STR("\u96de"),     // 0x7641 - 0x7644
		ARIB_STR("\u9903"),     ARIB_STR("\u9940"),     ARIB_STR("\u9ad9"),     ARIB_STR("\u9bd6"),     // 0x7645 - 0x7648
		ARIB_STR("\u9dd7"),     ARIB_STR("\u9eb4"),     ARIB_STR("\u9eb5"),                             // 0x7649 - 0x764B
	};

	static const struct {
		uint16_t First;
		uint16_t Last;
		const InternalChar * const *Table;
		const InternalChar * const *TableU;
	} SymbolTable[] = {
		{0x7521, 0x757E, nullptr,            KanjiTable1},
		{0x7621, 0x764B, nullptr,            KanjiTable2},
		{0x7A21, 0x7A48, nullptr,            SymbolsTable_90_01},
		{0x7A4D, 0x7A74, SymbolsTable_90_45, SymbolsTable_90_45_U},
		{0x7B21, 0x7B51, nullptr,            SymbolsTable_91},
		{0x7C21, 0x7C7B, SymbolsTable_92,    SymbolsTable_92_U},
		{0x7D21, 0x7D7B, SymbolsTable_93,    SymbolsTable_93_U},
		{0x7E21, 0x7E7D, SymbolsTable_94,    SymbolsTable_94_U},
	};

	// シンボルを変換する
	const InternalChar *pSrc = nullptr;

	for (auto &Table : SymbolTable) {
		if ((Code >= Table.First) && (Code <= Table.Last)) {
			if ((Table.Table == nullptr) || m_UnicodeSymbol)
				pSrc = Table.TableU[Code - Table.First];
			else
				pSrc = Table.Table[Code - Table.First];
			break;
		}
	}

	if (pSrc != nullptr)
		*pDstString += pSrc;
	else
		*pDstString += TOFU_STR;
}


void ARIBStringDecoder::PutMacroChar(uint16_t Code, InternalString *pDstString)
{
	static const uint8_t Macro[16][20] = {
		{0x1B, 0x24, 0x39, 0x1B, 0x29, 0x4A, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x24, 0x39, 0x1B, 0x29, 0x31, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x34, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x33, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x32, 0x1B, 0x29, 0x20, 0x41, 0x1B, 0x2A, 0x35, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x20, 0x41, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x20, 0x43, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x20, 0x44, 0x1B, 0x29, 0x20, 0x45, 0x1B, 0x2A, 0x20, 0x46, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x20, 0x47, 0x1B, 0x29, 0x20, 0x48, 0x1B, 0x2A, 0x20, 0x49, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x20, 0x4A, 0x1B, 0x29, 0x20, 0x4B, 0x1B, 0x2A, 0x20, 0x4C, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x20, 0x4D, 0x1B, 0x29, 0x20, 0x4E, 0x1B, 0x2A, 0x20, 0x4F, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x42, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x43, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x24, 0x39, 0x1B, 0x29, 0x20, 0x44, 0x1B, 0x2A, 0x30, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x31, 0x1B, 0x29, 0x30, 0x1B, 0x2A, 0x4A, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
		{0x1B, 0x28, 0x4A, 0x1B, 0x29, 0x32, 0x1B, 0x2A, 0x20, 0x41, 0x1B, 0x2B, 0x20, 0x70, 0x0F, 0x1B, 0x7D},
	};

	if ((Code & 0xF0) == 0x60)
		DecodeString(Macro[Code & 0x0F], StringLength(reinterpret_cast<const char *>(Macro[Code & 0x0F])), pDstString);
}


void ARIBStringDecoder::PutDRCSChar(uint16_t Code, InternalString *pDstString)
{
	if (m_pDRCSMap != nullptr) {
		const InternalChar *pSrc = m_pDRCSMap->GetString(Code);
		if (pSrc != nullptr) {
			*pDstString += pSrc;
			return;
		}
	}

	*pDstString += TOFU_STR;
}


void ARIBStringDecoder::ProcessEscapeSeq(uint8_t Code)
{
	// エスケープシーケンス処理
	switch (m_ESCSeqCount) {
	// 1バイト目
	case 1:
		switch (Code) {
		// Invocation of code elements
		case 0x6E: m_LockingGL = 2; m_ESCSeqCount = 0; return;	// LS2
		case 0x6F: m_LockingGL = 3; m_ESCSeqCount = 0; return;	// LS3
		case 0x7E: m_LockingGR = 1; m_ESCSeqCount = 0; return;	// LS1R
		case 0x7D: m_LockingGR = 2; m_ESCSeqCount = 0; return;	// LS2R
		case 0x7C: m_LockingGR = 3; m_ESCSeqCount = 0; return;	// LS3R

		// Designation of graphic sets
		case 0x24:
		case 0x28: m_ESCSeqIndex = 0; break;
		case 0x29: m_ESCSeqIndex = 1; break;
		case 0x2A: m_ESCSeqIndex = 2; break;
		case 0x2B: m_ESCSeqIndex = 3; break;
		default:   m_ESCSeqCount = 0; return;	// エラー
		}
		break;

	// 2バイト目
	case 2:
		if (DesignationGSET(m_ESCSeqIndex, Code)) {
			m_ESCSeqCount = 0;
			return;
		}

		switch (Code) {
		case 0x20: m_IsESCSeqDRCS = true;  break;
		case 0x28: m_IsESCSeqDRCS = true;  m_ESCSeqIndex = 0; break;
		case 0x29: m_IsESCSeqDRCS = false; m_ESCSeqIndex = 1; break;
		case 0x2A: m_IsESCSeqDRCS = false; m_ESCSeqIndex = 2; break;
		case 0x2B: m_IsESCSeqDRCS = false; m_ESCSeqIndex = 3; break;
		default:   m_ESCSeqCount = 0;      return;	// エラー
		}
		break;

	// 3バイト目
	case 3:
		if (!m_IsESCSeqDRCS) {
			if (DesignationGSET(m_ESCSeqIndex, Code)) {
				m_ESCSeqCount = 0;
				return;
			}
		} else {
			if (DesignationDRCS(m_ESCSeqIndex, Code)) {
				m_ESCSeqCount = 0;
				return;
			}
		}

		if (Code == 0x20) {
			m_IsESCSeqDRCS = true;
		} else {
			// エラー
			m_ESCSeqCount = 0;
			return;
		}
		break;

	// 4バイト目
	case 4:
		DesignationDRCS(m_ESCSeqIndex, Code);
		m_ESCSeqCount = 0;
		return;
	}

	m_ESCSeqCount++;
}


bool ARIBStringDecoder::DesignationGSET(uint8_t IndexG, uint8_t Code)
{
	// Gのグラフィックセットを割り当てる
	switch (Code) {
	case 0x42: m_CodeG[IndexG] = CodeSet::Kanji;                    return true;	// Kanji
	case 0x4A: m_CodeG[IndexG] = CodeSet::Alphanumeric;             return true;	// Alphanumeric
	case 0x30: m_CodeG[IndexG] = CodeSet::Hiragana;                 return true;	// Hiragana
	case 0x31: m_CodeG[IndexG] = CodeSet::Katakana;                 return true;	// Katakana
	case 0x32: m_CodeG[IndexG] = CodeSet::Mosaic_A;                 return true;	// Mosaic A
	case 0x33: m_CodeG[IndexG] = CodeSet::Mosaic_B;                 return true;	// Mosaic B
	case 0x34: m_CodeG[IndexG] = CodeSet::Mosaic_C;                 return true;	// Mosaic C
	case 0x35: m_CodeG[IndexG] = CodeSet::Mosaic_D;                 return true;	// Mosaic D
	case 0x36: m_CodeG[IndexG] = CodeSet::ProportionalAlphanumeric; return true;	// Proportional Alphanumeric
	case 0x37: m_CodeG[IndexG] = CodeSet::ProportionalHiragana;     return true;	// Proportional Hiragana
	case 0x38: m_CodeG[IndexG] = CodeSet::ProportionalKatakana;     return true;	// Proportional Katakana
	case 0x49: m_CodeG[IndexG] = CodeSet::JIS_X0201_Katakana;       return true;	// JIS X 0201 Katakana
	case 0x4B: m_CodeG[IndexG] = CodeSet::LatinExtension;           return true;	// Latin Extension
	case 0x4C: m_CodeG[IndexG] = CodeSet::LatinSpecial;             return true;	// Latin Special
	case 0x39: m_CodeG[IndexG] = CodeSet::JIS_KanjiPlane1;          return true;	// JIS compatible Kanji Plane 1
	case 0x3A: m_CodeG[IndexG] = CodeSet::JIS_KanjiPlane2;          return true;	// JIS compatible Kanji Plane 2
	case 0x3B: m_CodeG[IndexG] = CodeSet::AdditionalSymbols;        return true;	// Additional symbols
	}

	return false;
}


bool ARIBStringDecoder::DesignationDRCS(uint8_t IndexG, uint8_t Code)
{
	// DRCSのグラフィックセットを割り当てる
	if ((Code >= 0x40) && (Code <= 0x4F)) {
		// DRCS
		m_CodeG[IndexG] = static_cast<CodeSet>(static_cast<int>(CodeSet::DRCS_0) + (Code - 0x40));
	} else if (Code == 0x70) {
		// Macro
		m_CodeG[IndexG] = CodeSet::Macro;
	} else {
		return false;
	}
	return true;
}


bool ARIBStringDecoder::SetFormat(size_t Pos)
{
	if (!m_pFormatList)
		return false;

	FormatInfo Format;

	Format.Pos = Pos;
	Format.Size = m_CharSize;
	Format.CharColorIndex = m_CharColorIndex;
	Format.BackColorIndex = m_BackColorIndex;
	Format.RasterColorIndex = m_RasterColorIndex;

	if (!m_pFormatList->empty()) {
		if (m_pFormatList->back().Pos == Pos) {
			m_pFormatList->back() = Format;
			return true;
		}
	}

	m_pFormatList->push_back(Format);

	return true;
}


bool ARIBStringDecoder::IsDoubleByteCodeSet(CodeSet Set)
{
	switch (Set) {
	case CodeSet::Kanji:
	case CodeSet::JIS_KanjiPlane1:
	case CodeSet::JIS_KanjiPlane2:
	case CodeSet::AdditionalSymbols:
	case CodeSet::DRCS_0:
		return true;
	}

	return false;
}


size_t ARIBStringDecoder::UTF8ToCodePoint(const uint8_t *pData, size_t Length, uint32_t *pCodePoint)
{
	*pCodePoint = 0;
	if (Length == 0) {
		return 0;
	} else if ((pData[0] >= 0xC2) && (pData[0] < 0xE0) && (Length >= 2)
			&& (pData[1] >= 80) && (pData[1] < 0xC0)) {
		*pCodePoint = ((pData[0] & 0x1F) << 6) | (pData[1] & 0x3F);
		return 2;
	} else if ((pData[0] >= 0xE0) && (pData[0] < 0xF0) && (Length >= 3)
			&& (pData[1] >= 0x80) && (pData[1] < 0xC0)
			&& ((pData[0] & 0x0F) || (pData[1] & 0x20))
			&& (pData[2] >= 0x80) && (pData[2] < 0xC0)) {
		*pCodePoint = ((pData[0] & 0x0F) << 12) | ((pData[1] & 0x3F) << 6) | (pData[2] & 0x3F);
		if ((*pCodePoint >= 0xD800) && (*pCodePoint < 0xE000))
			*pCodePoint = 0;
		return 3;
	} else if ((pData[0] >= 0xF0) && (pData[0] < 0xF8) && (Length >= 4)
			&& (pData[1] >= 0x80) && (pData[1] < 0xC0)
			&& ((pData[0] & 0x07) || (pData[1] & 0x30))
			&& (pData[2] >= 0x80) && (pData[2] < 0xC0)
			&& (pData[3] >= 0x80) && (pData[3] < 0xC0)) {
		*pCodePoint = ((pData[0] & 0x07) << 18) | ((pData[1] & 0x3F) << 12)
			| ((pData[2] & 0x3F) << 6) | (pData[3] & 0x3F);
		if (*pCodePoint >= 0x110000)
			*pCodePoint = 0;
		return 4;
	} else if (pData[0] < 0x80) {
		*pCodePoint = pData[0];
	}
	return 1;
}


}	// namespace LibISDB
