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
 @file   tslogoextract.cpp
 @brief  TS から局ロゴを抽出

 TS ファイルから局ロゴを抽出する。

 tslogoextract <filename>

 @author DBCTRADO
*/


#include "../LibISDB/LibISDB.hpp"
#include "../LibISDB/Engine/StreamSourceEngine.hpp"
#include "../LibISDB/Filters/StreamSourceFilter.hpp"
#include "../LibISDB/Filters/TSPacketParserFilter.hpp"
#include "../LibISDB/Filters/LogoDownloaderFilter.hpp"
#include "../LibISDB/Base/StandardStream.hpp"
#include "../LibISDB/Base/FileStream.hpp"
#include "../LibISDB/Utilities/Utilities.hpp"
#include "../LibISDB/Utilities/StringFormat.hpp"
#include "../LibISDB/Utilities/StringUtilities.hpp"
#include "../LibISDB/Utilities/CRC.hpp"
#include <iostream>
#include <iomanip>
#include <map>


namespace
{


class LogoExtractEngine
	: public LibISDB::StreamSourceEngine
	, public LibISDB::LogoDownloaderFilter::LogoHandler
{
public:
	void SetSaveRaw(bool Raw) noexcept { m_SaveRaw = Raw; }
	int GetSavedCount() const noexcept { return m_SavedCount; }

private:
// LibISDB::LogoDownloaderFilter::LogoHandler
	void OnLogoDownloaded(const LibISDB::LogoDownloaderFilter::LogoData &Data) override;

	bool WritePNGWithPLTE(LibISDB::Stream &Stream, const std::uint8_t *pData, std::size_t DataSize);

	bool m_SaveRaw = false;
	int m_SavedCount = 0;
	std::multimap<LibISDB::String, LibISDB::DataBuffer> m_LogoMap;
};


void LogoExtractEngine::OnLogoDownloaded(const LibISDB::LogoDownloaderFilter::LogoData &Data)
{
	// 透明なロゴは除外
	if (Data.DataSize <= 93)
		return;

	LibISDB::CharType Format[32];

	LibISDB::StringFormat(
		Format, LIBISDB_STR("{:04X}_{:03X}_{:03X}_{:02X}"),
		Data.NetworkID, Data.LogoID, Data.LogoVersion, Data.LogoType);

	LibISDB::String FileName(Format);
	auto Range = m_LogoMap.equal_range(FileName);
	if (Range.first != Range.second) {
		int Count = 0;
		for (auto it = Range.first; it != Range.second; ++it) {
			if ((it->second.GetSize() == Data.DataSize)
					&& (std::memcmp(it->second.GetData(), Data.pData, Data.DataSize) == 0)) {
				return;
			}
			Count++;
		}

		LibISDB::StringFormat(Format, LIBISDB_STR("-{}"), Count + 1);
		FileName += Format;
	}

	auto it = m_LogoMap.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(FileName),
		std::forward_as_tuple());
	it->second.SetData(Data.pData, Data.DataSize);

	FileName += LIBISDB_STR(".png");

#if defined(LIBISDB_WCHAR)
	auto &Out = std::wcout;
	auto &ErrOut = std::wcerr;
#else
	auto &Out = std::cout;
	auto &ErrOut = std::cerr;
#endif

	LibISDB::FileStream Stream;

	if (!Stream.Open(
			FileName.c_str(),
			LibISDB::FileStream::OpenFlag::Write |
			LibISDB::FileStream::OpenFlag::Create |
			LibISDB::FileStream::OpenFlag::Truncate)) {
		ErrOut << LIBISDB_STR("Failed to create file : ") << FileName << std::endl;
		return;
	}

	if (m_SaveRaw) {
		Stream.Write(Data.pData, Data.DataSize);
	} else {
		if (!WritePNGWithPLTE(Stream, Data.pData, Data.DataSize))
			ErrOut << LIBISDB_STR("Invalid PNG format") << std::endl;
	}

	Out << LIBISDB_STR("Extracted ") << FileName << std::endl;

	m_SavedCount++;
}


bool LogoExtractEngine::WritePNGWithPLTE(LibISDB::Stream &Stream, const std::uint8_t *pData, std::size_t DataSize)
{
	static const struct {
		std::uint8_t R, G, B, A;
	} Colormap[128] = {
		{  0,   0,   0, 255},
		{255,   0,   0, 255},
		{  0, 255,   0, 255},
		{255, 255,   0, 255},
		{  0,   0,   0, 255},
		{255,   0, 255, 255},
		{  0, 255, 255, 255},
		{255, 255, 255, 255},
		{  0,   0,   0,   0},
		{170,   0,   0, 255},
		{  0, 170,   0, 255},
		{170, 170,   0, 255},
		{  0,   0, 170, 255},
		{170,   0, 170, 255},
		{  0, 170, 170, 255},
		{170, 170, 170, 255},
		{  0,   0,  85, 255},
		{  0,  85,   0, 255},
		{  0,  85,  85, 255},
		{  0,  85, 170, 255},
		{  0,  85, 255, 255},
		{  0, 170,  85, 255},
		{  0, 170, 255, 255},
		{  0, 255,  85, 255},
		{  0, 255, 170, 255},
		{ 85,   0,   0, 255},
		{ 85,   0,  85, 255},
		{ 85,   0, 170, 255},
		{ 85,   0, 255, 255},
		{ 85,  85,   0, 255},
		{ 85,  85,  85, 255},
		{ 85,  85, 170, 255},
		{ 85,  85, 255, 255},
		{ 85, 170,   0, 255},
		{ 85, 170,  85, 255},
		{ 85, 170, 170, 255},
		{ 85, 170, 255, 255},
		{ 85, 255,   0, 255},
		{ 85, 255,  85, 255},
		{ 85, 255, 170, 255},
		{ 85, 255, 255, 255},
		{170,   0,  85, 255},
		{170,   0, 255, 255},
		{170,  85,   0, 255},
		{170,  85,  85, 255},
		{170,  85, 170, 255},
		{170,  85, 255, 255},
		{170, 170,  85, 255},
		{170, 170, 255, 255},
		{170, 255,   0, 255},
		{170, 255,  85, 255},
		{170, 255, 170, 255},
		{170, 255, 255, 255},
		{255,   0,  85, 255},
		{255,   0, 170, 255},
		{255,  85,   0, 255},
		{255,  85,  85, 255},
		{255,  85, 170, 255},
		{255,  85, 255, 255},
		{255, 170,   0, 255},
		{255, 170,  85, 255},
		{255, 170, 170, 255},
		{255, 170, 255, 255},
		{255, 225,  85, 255},
		{225, 225, 170, 255},
		{  0,   0,   0, 128},
		{255,   0,   0, 128},
		{  0, 255,   0, 128},
		{255, 255,   0, 128},
		{  0,   0, 255, 128},
		{255,   0, 255, 128},
		{  0, 255, 255, 128},
		{255, 255, 255, 128},
		{170,   0,   0, 128},
		{  0, 170,   0, 128},
		{170, 170,   0, 128},
		{  0,   0, 170, 128},
		{170,   0, 170, 128},
		{  0, 170, 170, 128},
		{170, 170, 170, 128},
		{  0,   0,  85, 128},
		{  0,  85,   0, 128},
		{  0,  85,  85, 128},
		{  0,  85, 170, 128},
		{  0,  85, 255, 128},
		{  0, 170,  85, 128},
		{  0, 170, 255, 128},
		{  0, 255,  85, 128},
		{  0, 255, 170, 128},
		{ 85,   0,   0, 128},
		{ 85,   0,  85, 128},
		{ 85,   0, 170, 128},
		{ 85,   0, 255, 128},
		{ 85,  85,   0, 128},
		{ 85,  85,  85, 128},
		{ 85,  85, 170, 128},
		{ 85,  85, 255, 128},
		{ 85, 170,   0, 128},
		{ 85, 170,  85, 128},
		{ 85, 170, 170, 128},
		{ 85, 170, 255, 128},
		{ 85, 255,   0, 128},
		{ 85, 255,  85, 128},
		{ 85, 255, 170, 128},
		{ 85, 255, 255, 128},
		{170,   0,  85, 128},
		{170,   0, 255, 128},
		{170,  85,   0, 128},
		{170,  85,  85, 128},
		{170,  85, 170, 128},
		{170,  85, 255, 128},
		{170, 170,  85, 128},
		{170, 170, 255, 128},
		{170, 255,   0, 128},
		{170, 255,  85, 128},
		{170, 255, 170, 128},
		{170, 255, 255, 128},
		{255,   0,  85, 128},
		{255,   0, 170, 128},
		{255,  85,   0, 128},
		{255,  85,  85, 128},
		{255,  85, 170, 128},
		{255,  85, 255, 128},
		{255, 170,   0, 128},
		{255, 170,  85, 128},
		{255, 170, 170, 128},
		{255, 170, 255, 128},
		{255, 255,  85, 128},
	};

	std::size_t i = 0;

	if ((DataSize < 8) || (std::memcmp(pData, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) != 0)) {
		return false;
	}
	Stream.Write(pData, 8);
	i += 8;

	int BitDepth = 0, ColorType = -1;
	bool HasPalette = false;

	for (;;) {
		if (DataSize - i < 12)
			return false;

		const std::uint32_t ChunkSize = LibISDB::Load32(&pData[i]);
		if (ChunkSize > DataSize - i - 12)
			return false;

		if (std::memcmp(&pData[i + 4], u8"IHDR", 4) == 0) {
			if (ChunkSize < 13)
				return false;
			BitDepth = pData[i + 8 + 8];
			ColorType = pData[i + 8 + 9];
		} else if (std::memcmp(&pData[i + 4], u8"PLTE", 4) == 0) {
			HasPalette = true;
		} else if (std::memcmp(&pData[i + 4], u8"IDAT", 4) == 0) {
			if (!HasPalette && (BitDepth <= 8) && (ColorType == 3)) {
				const int ColorCount = 1 << BitDepth;
				std::uint8_t Buffer[8 + 256 * 3 + 4];

				// tRNS
				LibISDB::Store32(&Buffer[0], ColorCount);
				std::memcpy(&Buffer[4], u8"tRNS", 4);
				for (int i = 0; i < std::min(ColorCount, 128); i++)
					Buffer[8 + i] = Colormap[i].A;
				if (BitDepth == 8)
					std::memset(&Buffer[8 + 128], 0, 128);
				LibISDB::Store32(&Buffer[8 + ColorCount], LibISDB::CRC32::Calc(&Buffer[4], 4 + ColorCount));
				Stream.Write(Buffer, 8 + ColorCount + 4);

				// PLTE
				const std::uint32_t PaletteSize = ColorCount * 3;
				LibISDB::Store32(&Buffer[0], PaletteSize);
				std::memcpy(&Buffer[4], u8"PLTE", 4);
				for (int i = 0; i < std::min(ColorCount, 128); i++) {
					Buffer[8 + i * 3 + 0] = Colormap[i].R;
					Buffer[8 + i * 3 + 1] = Colormap[i].G;
					Buffer[8 + i * 3 + 2] = Colormap[i].B;
				}
				if (BitDepth == 8)
					std::memset(&Buffer[8 + 128 * 3], 0, 128 * 3);
				LibISDB::Store32(&Buffer[8 + PaletteSize], LibISDB::CRC32::Calc(&Buffer[4], 4 + PaletteSize));
				Stream.Write(Buffer, 8 + PaletteSize + 4);
			}
		}

		Stream.Write(&pData[i], 8 + ChunkSize + 4);

		if (std::memcmp(&pData[i + 4], u8"IEND", 4) == 0)
			break;

		i += 8 + ChunkSize + 4;
	}

	return true;
}


}


#if defined(LIBISDB_WCHAR)
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
#if defined(LIBISDB_WCHAR)
	auto &Out = std::wcout;
	auto &ErrOut = std::wcerr;
#else
	auto &Out = std::cout;
	auto &ErrOut = std::cerr;
#endif

	if (argc < 2) {
		ErrOut << LIBISDB_STR("Need filename.") << std::endl;
		return 1;
	}

	LibISDB::StreamSourceFilter *pSource = new LibISDB::StreamSourceFilter;
	LibISDB::TSPacketParserFilter *pParser = new LibISDB::TSPacketParserFilter;
	LibISDB::LogoDownloaderFilter *pLogoDownloader = new LibISDB::LogoDownloaderFilter;

	LogoExtractEngine Engine;

	Engine.BuildEngine({pSource, pParser, pLogoDownloader});
	Engine.SetStartStreamingOnSourceOpen(true);

	pLogoDownloader->SetLogoHandler(&Engine);

	const LibISDB::CharType *pFileName = argv[1];
	if (LibISDB::StringCompare(pFileName, LIBISDB_STR("-")) == 0)
		pFileName = LibISDB::StandardInputStream::Name;
	if (!Engine.OpenSource(pFileName)) {
		ErrOut << LIBISDB_STR("Failed to open file : ") << pFileName << std::endl;
		return 1;
	}

	Engine.WaitForEndOfStream();
	Engine.CloseSource();

	if (Engine.GetSavedCount() == 0) {
		ErrOut << LIBISDB_STR("Logo data not found.") << std::endl;
		return 1;
	}

	Out << Engine.GetSavedCount() << LIBISDB_STR(" logo files saved.") << std::endl;

	return 0;
}
