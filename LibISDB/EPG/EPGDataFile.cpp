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
 @file   EPGDataFile.cpp
 @brief  EPG データファイル
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "EPGDataFile.hpp"
#include "../Base/FileStream.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "../Utilities/CRC.hpp"
#include <new>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{

namespace
{


/*
	EPG データファイルの構造
	┌─────────────────────┐
	│FileHeader                                │
	├─────────────────────┤
	│┌───────────────────┐│
	││ServiceInfo                           ││
	│├───────────────────┤│
	││┌─────────────────┐││
	│││EventInfo                         │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││EventAudioHeader              ││││
	│││├───────────────┤│││
	││││┌─────────────┐││││
	│││││EventAudioInfo            │││││
	││││└─────────────┘││││
	││││ ...                          ││││
	│││└───────────────┘│││
	│││┌───────────────┐│││
	││││EventVideoHeader              ││││
	│││├───────────────┤│││
	││││┌─────────────┐││││
	│││││EventVideoInfo            │││││
	││││└─────────────┘││││
	││││ ...                          ││││
	│││└───────────────┘│││
	│││┌───────────────┐│││
	││││EventGenreInfo                ││││
	│││└───────────────┘│││
	│││┌───────────────┐│││
	││││イベント名                    ││││
	│││└───────────────┘│││
	│││┌───────────────┐│││
	││││イベントテキスト              ││││
	│││└───────────────┘│││
	│││┌───────────────┐│││
	││││イベント拡張テキスト          ││││
	│││└───────────────┘│││
	│││┌───────────────┐│││
	││││EventGroupHeader              ││││
	│││├───────────────┤│││
	││││┌─────────────┐││││
	│││││EventGroupInfoHeader      │││││
	││││├─────────────┤││││
	│││││┌───────────┐│││││
	││││││EventGroupInfo        ││││││
	│││││└───────────┘│││││
	│││││ ...                      │││││
	││││└─────────────┘││││
	││││ ...                          ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││└─────────────────┘││
	││ ...                                  ││
	│└───────────────────┘│
	│ ...                                      │
	└─────────────────────┘
*/


namespace EPGData
{


namespace Tag {
	constexpr uint8_t Null              = 0x00_u8;
	constexpr uint8_t End               = 0x01_u8;
	constexpr uint8_t Service           = 0x02_u8;
	constexpr uint8_t ServiceEnd        = 0x03_u8;
	constexpr uint8_t Event             = 0x04_u8;
	constexpr uint8_t EventEnd          = 0x05_u8;
	constexpr uint8_t EventAudio        = 0x06_u8;
	constexpr uint8_t EventVideo        = 0x07_u8;
	constexpr uint8_t EventGenre        = 0x08_u8;
	constexpr uint8_t EventName         = 0x09_u8;
	constexpr uint8_t EventText         = 0x0A_u8;
	constexpr uint8_t EventExtendedText = 0x0B_u8;
	constexpr uint8_t EventGroup        = 0x0C_u8;
}

struct ChunkHeader {
	uint8_t Tag;
	uint16_t Size;
};

constexpr size_t CHUNK_HEADER_SIZE = 1 + 2;


LIBISDB_PRAGMA_PACK_PUSH_1


struct FileHeader {
	char Type[8];
	uint32_t Version;
	uint32_t ServiceCount;
	uint64_t UpdateCount;
} LIBISDB_ATTRIBUTE_PACKED;

const char FileHeader_Type[8] = {'E', 'P', 'G', '-', 'D', 'A', 'T', 'A'};
const uint32_t FileHeader_Version = 0;

struct EPGDateTime {
	uint16_t Year;
	uint8_t Month;
	uint8_t DayOfWeek;
	uint8_t Day;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Second;

	EPGDateTime & operator = (const EPGDateTime &) = default;
	EPGDateTime & operator = (const DateTime &rhs)
	{
		Year = rhs.Year;
		Month = rhs.Month;
		DayOfWeek = rhs.DayOfWeek;
		Day = rhs.Day;
		Hour = rhs.Hour;
		Minute = rhs.Minute;
		Second = rhs.Second;

		return *this;
	}

	operator DateTime() const
	{
		DateTime Dst;

		Dst.Year = Year;
		Dst.Month = Month;
		Dst.Day = Day;
		Dst.DayOfWeek = DayOfWeek;
		Dst.Hour = Hour;
		Dst.Minute = Minute;
		Dst.Second = Second;

		return Dst;
	}
} LIBISDB_ATTRIBUTE_PACKED;

struct ServiceInfo {
	uint16_t NetworkID;
	uint16_t TransportStreamID;
	uint16_t ServiceID;
	uint16_t EventCount;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventInfo {
	static constexpr uint16_t Flag_RunningStatus = 0x0007_u16;
	static constexpr uint16_t Flag_FreeCAMode    = 0x0008_u16;
	static constexpr uint16_t Flag_Basic         = 0x0010_u16;
	static constexpr uint16_t Flag_Extended      = 0x0020_u16;
	static constexpr uint16_t Flag_Present       = 0x0040_u16;
	static constexpr uint16_t Flag_Following     = 0x0080_u16;

	uint16_t EventID;
	uint16_t Flags;
	EPGDateTime StartTime;
	uint32_t Duration;
	uint64_t UpdatedTime;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventAudioHeader {
	uint8_t AudioCount;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventAudioInfo {
	static constexpr uint8_t Flag_MultiLingual  = 0x01_u8;
	static constexpr uint8_t Flag_MainComponent = 0x02_u8;

	uint8_t Flags;
	uint8_t StreamContent;
	uint8_t ComponentType;
	uint8_t ComponentTag;
	uint8_t SimulcastGroupTag;
	uint8_t QualityIndicator;
	uint8_t SamplingRate;
	uint8_t Reserved;
	uint32_t LanguageCode;
	uint32_t LanguageCode2;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventVideoHeader {
	uint8_t VideoCount;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventVideoInfo {
	uint8_t StreamContent;
	uint8_t ComponentType;
	uint8_t ComponentTag;
	uint8_t Reserved;
	uint32_t LanguageCode;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventGenreInfo {
	uint8_t NibbleCount;
	struct {
		uint8_t ContentNibble;
		uint8_t UserNibble;
	} NibbleList[7];
} LIBISDB_ATTRIBUTE_PACKED;

struct EventExtendedTextHeader {
	uint8_t TextCount;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventGroupHeader {
	uint8_t GroupCount;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventGroupInfoHeader {
	uint8_t GroupType;
	uint8_t EventCount;
} LIBISDB_ATTRIBUTE_PACKED;

struct EventGroupInfo {
	uint16_t ServiceID;
	uint16_t EventID;
	uint16_t NetworkID;
	uint16_t TransportStreamID;
} LIBISDB_ATTRIBUTE_PACKED;


LIBISDB_PRAGMA_PACK_POP


}	// namespace EPGData


constexpr uint16_t MAX_EPG_TEXT_LENGTH = 4096;


void ReadData(FileStream &File, void *pData, size_t DataSize, size_t *pSizeLimit)
{
	if (DataSize > *pSizeLimit)
		throw EPGDataFile::Exception::FormatError;
	if (File.Read(pData, DataSize) != DataSize)
		throw EPGDataFile::Exception::Read;
	*pSizeLimit -= DataSize;
}


template<typename T> void ReadData(FileStream &File, T &Data, size_t *pSizeLimit)
{
	ReadData(File, &Data, sizeof(Data), pSizeLimit);
}


void ReadChunkHeader(FileStream &File, EPGData::ChunkHeader *pHeader, size_t *pSizeLimit)
{
	ReadData(File, pHeader->Tag, pSizeLimit);
	ReadData(File, pHeader->Size, pSizeLimit);
}


void ReadString(FileStream &File, String *pString, size_t *pSizeLimit)
{
	uint16_t Length;

	pString->clear();

	ReadData(File, Length, pSizeLimit);
	if (Length > MAX_EPG_TEXT_LENGTH)
		throw EPGDataFile::Exception::FormatError;

	if (Length > 0) {
		pString->resize(Length);
		ReadData(File, pString->data(), Length * sizeof(CharType), pSizeLimit);
	}
}


void WriteData(FileStream &File, const void *pData, size_t DataSize)
{
	if (File.Write(pData, DataSize) != DataSize)
		throw EPGDataFile::Exception::Write;
}


template<typename T> void WriteData(FileStream &File, const T &Data)
{
	WriteData(File, &Data, sizeof(Data));
}


void WriteChunkHeader(FileStream &File, uint8_t Tag, size_t Size = 0)
{
	LIBISDB_ASSERT(Size <= 0xFFFF);
	if (Size > 0xFFFF)
		throw EPGDataFile::Exception::Internal;

	WriteData(File, Tag);
	WriteData(File, static_cast<uint16_t>(Size));
}


void WriteChunk(FileStream &File, uint8_t Tag, const void *pData, size_t Size)
{
	WriteChunkHeader(File, Tag, Size);
	WriteData(File, pData, Size);
}


template<typename T> void WriteChunk(FileStream &File, uint8_t Tag, const T &Data)
{
	static_assert(sizeof(Data) <= 0xFFFF);

	WriteChunk(File, Tag, &Data, sizeof(Data));
}


void WriteString(FileStream &File, const String &Str)
{
	const uint16_t Length = static_cast<uint16_t>(Str.length());
	if (Length > MAX_EPG_TEXT_LENGTH)
		throw EPGDataFile::Exception::Internal;

	WriteData(File, Length);

	if (Length > 0)
		WriteData(File, Str.data(), Length * sizeof(CharType));
}


void WriteChunkString(FileStream &File, uint8_t Tag, const String &Str)
{
	if (Str.length() > MAX_EPG_TEXT_LENGTH)
		throw EPGDataFile::Exception::FormatError;

	WriteChunkHeader(File, Tag, sizeof(uint16_t) + Str.length() * sizeof(CharType));
	WriteString(File, Str);
}


}	// namespace




EPGDataFile::EPGDataFile()
	: m_pEPGDatabase(nullptr)
	, m_OpenFlags(OpenFlag::None)
	, m_UpdateCount(0)
{
}


bool EPGDataFile::Open(EPGDatabase *pEPGDatabase, const CStringView &FileName, OpenFlag Flags)
{
	Close();

	if (LIBISDB_TRACE_ERROR_IF(pEPGDatabase == nullptr))
		return false;
	if (LIBISDB_TRACE_ERROR_IF(FileName.empty()))
		return false;

	m_pEPGDatabase = pEPGDatabase;
	m_FileName = FileName;
	m_OpenFlags = Flags;
	m_UpdateCount = 0;

	return true;
}


void EPGDataFile::Close()
{
	m_pEPGDatabase = nullptr;
	m_FileName.clear();
	m_OpenFlags = OpenFlag::None;
}


bool EPGDataFile::IsOpen() const
{
	return m_pEPGDatabase != nullptr;
}


bool EPGDataFile::Load()
{
	if (LIBISDB_TRACE_ERROR_IF(
			(m_pEPGDatabase == nullptr) || m_FileName.empty() || !(m_OpenFlags & OpenFlag::Read)))
		return false;

	FileStream File;

	FileStream::OpenFlag FileOpenFlags =
		FileStream::OpenFlag::Read |
		FileStream::OpenFlag::SequentialRead;
	if (!!(m_OpenFlags & OpenFlag::ShareRead))
		FileOpenFlags |= FileStream::OpenFlag::ShareRead;
	if (!!(m_OpenFlags & OpenFlag::PriorityLow))
		FileOpenFlags |= FileStream::OpenFlag::PriorityLow;
	else if (!!(m_OpenFlags & OpenFlag::PriorityIdle))
		FileOpenFlags |= FileStream::OpenFlag::PriorityIdle;

	if (!File.Open(m_FileName, FileOpenFlags)) {
		Log(Logger::LogType::Error, LIBISDB_STR("EPGファイルを開けません。"));
		return false;
	}

	EPGData::FileHeader FileHeader;

	if (File.Read(&FileHeader, sizeof(EPGData::FileHeader)) != sizeof(EPGData::FileHeader)) {
		Log(Logger::LogType::Error, LIBISDB_STR("EPGファイルのヘッダを読み込めません。"));
		return false;
	}
	if (std::memcmp(FileHeader.Type, EPGData::FileHeader_Type, sizeof(FileHeader.Type)) != 0) {
		Log(Logger::LogType::Error, LIBISDB_STR("EPGファイルが未知の形式のため読み込めません。"));
		return false;
	}
	if (FileHeader.Version > EPGData::FileHeader_Version) {
		Log(Logger::LogType::Error, LIBISDB_STR("EPGファイルが非対応のバージョンのため読み込めません。"));
		return false;
	}

	m_UpdateCount = FileHeader.UpdateCount;

	try {
		EPGData::ChunkHeader ChunkHeader;

		do {
			size_t Size = EPGData::CHUNK_HEADER_SIZE;
			ReadChunkHeader(File, &ChunkHeader, &Size);

			if ((ChunkHeader.Tag == EPGData::Tag::Service) && (ChunkHeader.Size == sizeof(EPGData::ServiceInfo))) {
				ServiceInfo Service;

				LoadService(File, &Service);

				if (!Service.EventList.empty())
					m_pEPGDatabase->SetServiceEventList(Service.Info, std::move(Service.EventList));
			} else {
				if (ChunkHeader.Size > 0) {
					if (!File.SetPos(ChunkHeader.Size, Stream::SetPosType::Current))
						throw Exception::Seek;
				}
			}
		} while (ChunkHeader.Tag != EPGData::Tag::End);
	} catch (Exception Code) {
		ExceptionLog(Code);
		return false;
	} catch (std::bad_alloc) {
		ExceptionLog(Exception::MemoryAllocate);
		return false;
	}

	return true;
}


bool EPGDataFile::LoadMerged()
{
	if (LIBISDB_TRACE_ERROR_IF(
			(m_pEPGDatabase == nullptr) || m_FileName.empty() || !(m_OpenFlags & OpenFlag::Read)))
		return false;

	EPGDatabase Database;
	EPGDataFile File;

	if (!File.Open(&Database, m_FileName, m_OpenFlags))
		return false;
	if (!File.Load())
		return false;
	File.Close();

	m_pEPGDatabase->Merge(&Database, EPGDatabase::MergeFlag::Database);

	return true;
}


bool EPGDataFile::LoadHeader()
{
	if (LIBISDB_TRACE_ERROR_IF(m_FileName.empty() || !(m_OpenFlags & OpenFlag::Read)))
		return false;

	FileStream File;
	FileStream::OpenFlag FileOpenFlags = FileStream::OpenFlag::Read | FileStream::OpenFlag::ShareRead;

	if (!File.Open(m_FileName, FileOpenFlags))
		return false;

	EPGData::FileHeader FileHeader;

	if (File.Read(&FileHeader, sizeof(EPGData::FileHeader)) != sizeof(EPGData::FileHeader))
		return false;
	if (std::memcmp(FileHeader.Type, EPGData::FileHeader_Type, sizeof(FileHeader.Type)) != 0)
		return false;
	if (FileHeader.Version > EPGData::FileHeader_Version)
		return false;

	m_UpdateCount = FileHeader.UpdateCount;

	return true;
}


bool EPGDataFile::Save()
{
	if (LIBISDB_TRACE_ERROR_IF(
			(m_pEPGDatabase == nullptr) || m_FileName.empty() || !(m_OpenFlags & OpenFlag::Write)))
		return false;

	FileStream File;

	if (!File.Open(
				m_FileName,
				FileStream::OpenFlag::Write |
				FileStream::OpenFlag::Create |
				FileStream::OpenFlag::Truncate)) {
		Log(Logger::LogType::Error, LIBISDB_STR("EPGファイルが開けません。"));
		return false;
	}

	BlockLock Lock(m_pEPGDatabase->GetLock());

	DateTime EarliestTime;
	if (!!(m_OpenFlags & OpenFlag::DiscardOld)) {
		GetCurrentEPGTime(&EarliestTime);
		EarliestTime.OffsetHours(-1);
	}

	EPGDatabase::ServiceList ServiceList;
	m_pEPGDatabase->GetServiceList(&ServiceList);

	std::vector<uint16_t> EventCountList;
	EventCountList.reserve(ServiceList.size());

	uint32_t ValidServiceCount = 0;

	for (auto &Service : ServiceList) {
		uint16_t EventCount = 0;

		if (!!(m_OpenFlags & OpenFlag::DiscardOld)) {
			m_pEPGDatabase->EnumEventsSortedByTime(
				Service.NetworkID, Service.TransportStreamID, Service.ServiceID,
				&EarliestTime, nullptr,
				[&EventCount](const EventInfo &Event) -> bool {
					EventCount++;
					return true;
				});
		} else {
			m_pEPGDatabase->EnumEventsUnsorted(
				Service.NetworkID, Service.TransportStreamID, Service.ServiceID,
				[&EventCount](const EventInfo &Event) -> bool {
					EventCount++;
					return true;
				});
		}

		EventCountList.push_back(EventCount);
		if (EventCount > 0)
			ValidServiceCount++;
	}

	auto ErrorCleanup = [&]() {
		File.Close();

#ifdef LIBISDB_WINDOWS
		::DeleteFile(m_FileName.c_str());
#else
		std::remove(m_FileName.c_str());
#endif
	};

	try {
		EPGData::FileHeader FileHeader;

		std::memcpy(FileHeader.Type, EPGData::FileHeader_Type, sizeof(FileHeader.Type));
		FileHeader.Version = EPGData::FileHeader_Version;
		FileHeader.ServiceCount = ValidServiceCount;
		FileHeader.UpdateCount = ++m_UpdateCount;

		WriteData(File, FileHeader);

		for (size_t ServiceIndex = 0; ServiceIndex < ServiceList.size(); ServiceIndex++) {
			if (EventCountList[ServiceIndex] > 0)
				SaveService(File, ServiceList[ServiceIndex], EventCountList[ServiceIndex], EarliestTime);
		}

		WriteChunkHeader(File, EPGData::Tag::End);
	} catch (Exception Code) {
		ExceptionLog(Code);
		ErrorCleanup();
		return false;
	} catch (std::bad_alloc) {
		ExceptionLog(Exception::MemoryAllocate);
		ErrorCleanup();
		return false;
	}

	return true;
}


void EPGDataFile::LoadService(FileStream &File, ServiceInfo *pServiceInfo)
{
	EPGData::ServiceInfo ServiceHeader;
	size_t Size;

	Size = sizeof(ServiceHeader);
	ReadData(File, ServiceHeader, &Size);

	pServiceInfo->Info.NetworkID = ServiceHeader.NetworkID;
	pServiceInfo->Info.TransportStreamID = ServiceHeader.TransportStreamID;
	pServiceInfo->Info.ServiceID = ServiceHeader.ServiceID;

	pServiceInfo->EventList.reserve(ServiceHeader.EventCount);

	EPGData::ChunkHeader ChunkHeader;

	do {
		Size = EPGData::CHUNK_HEADER_SIZE;
		ReadChunkHeader(File, &ChunkHeader, &Size);

		if ((ChunkHeader.Tag == EPGData::Tag::Event) && (ChunkHeader.Size == sizeof(EPGData::EventInfo))) {
			pServiceInfo->EventList.emplace_back();
			EventInfo &Event = pServiceInfo->EventList.back();

			LoadEvent(File, pServiceInfo, &Event);
		} else {
			if (ChunkHeader.Size > 0) {
				if (!File.SetPos(ChunkHeader.Size, Stream::SetPosType::Current)) {
					throw Exception::Seek;
				}
			}
		}
	} while (ChunkHeader.Tag != EPGData::Tag::ServiceEnd);
}


void EPGDataFile::LoadEvent(FileStream &File, const ServiceInfo *pServiceInfo, EventInfo *pEvent)
{
	EPGData::EventInfo EventHeader;
	size_t Size = sizeof(EPGData::EventInfo);

	ReadData(File, &EventHeader, sizeof(EPGData::EventInfo), &Size);

	pEvent->NetworkID = pServiceInfo->Info.NetworkID;
	pEvent->TransportStreamID = pServiceInfo->Info.TransportStreamID;
	pEvent->ServiceID = pServiceInfo->Info.ServiceID;
	pEvent->EventID = EventHeader.EventID;
	pEvent->StartTime = EventHeader.StartTime;
	pEvent->Duration = EventHeader.Duration;
	pEvent->RunningStatus = EventHeader.Flags & EPGData::EventInfo::Flag_RunningStatus;
	pEvent->FreeCAMode = !!(EventHeader.Flags & EPGData::EventInfo::Flag_FreeCAMode);
	pEvent->Type = EventInfo::TypeFlag::Database;
	if (!!(EventHeader.Flags & EPGData::EventInfo::Flag_Basic))
		pEvent->Type |= EventInfo::TypeFlag::Basic;
	if (!!(EventHeader.Flags & EPGData::EventInfo::Flag_Extended))
		pEvent->Type |= EventInfo::TypeFlag::Extended;
	if (!!(EventHeader.Flags & EPGData::EventInfo::Flag_Present))
		pEvent->Type |= EventInfo::TypeFlag::Present;
	if (!!(EventHeader.Flags & EPGData::EventInfo::Flag_Following))
		pEvent->Type |= EventInfo::TypeFlag::Following;

	EPGData::ChunkHeader ChunkHeader;

	do {
		Size = EPGData::CHUNK_HEADER_SIZE;
		ReadChunkHeader(File, &ChunkHeader, &Size);
		Size = ChunkHeader.Size;

		switch (ChunkHeader.Tag) {
		case EPGData::Tag::EventAudio:
			{
				EPGData::EventAudioHeader Header;

				ReadData(File, Header, &Size);

				pEvent->AudioList.resize(Header.AudioCount);

				for (auto &Audio : pEvent->AudioList) {
					EPGData::EventAudioInfo Info;

					ReadData(File, Info, &Size);

					Audio.StreamContent      = Info.StreamContent;
					Audio.ComponentType      = Info.ComponentType;
					Audio.ComponentTag       = Info.ComponentTag;
					Audio.SimulcastGroupTag  = Info.SimulcastGroupTag;
					Audio.ESMultiLingualFlag = !!(Info.Flags & EPGData::EventAudioInfo::Flag_MultiLingual);
					Audio.MainComponentFlag  = !!(Info.Flags & EPGData::EventAudioInfo::Flag_MainComponent);
					Audio.QualityIndicator   = Info.QualityIndicator;
					Audio.SamplingRate       = Info.SamplingRate;
					Audio.LanguageCode       = Info.LanguageCode;
					Audio.LanguageCode2      = Info.LanguageCode2;

					ReadString(File, &Audio.Text, &Size);
				}
			}
			break;

		case EPGData::Tag::EventVideo:
			{
				EPGData::EventVideoHeader Header;

				ReadData(File, Header, &Size);

				pEvent->VideoList.resize(Header.VideoCount);

				for (auto &Video : pEvent->VideoList) {
					EPGData::EventVideoInfo Info;

					ReadData(File, Info, &Size);

					Video.StreamContent = Info.StreamContent;
					Video.ComponentType = Info.ComponentType;
					Video.ComponentTag  = Info.ComponentTag;
					Video.LanguageCode  = Info.LanguageCode;

					ReadString(File, &Video.Text, &Size);
				}
			}
			break;

		case EPGData::Tag::EventGenre:
			{
				EPGData::EventGenreInfo Genre;

				ReadData(File, Genre.NibbleCount, &Size);
				if (Genre.NibbleCount > 7)
					throw Exception::FormatError;
				ReadData(File, &Genre.NibbleList[0], Genre.NibbleCount * 2, &Size);

				pEvent->ContentNibble.NibbleCount = Genre.NibbleCount;

				for (int i = 0; i < Genre.NibbleCount; i++) {
					pEvent->ContentNibble.NibbleList[i].ContentNibbleLevel1 = Genre.NibbleList[i].ContentNibble >> 4;
					pEvent->ContentNibble.NibbleList[i].ContentNibbleLevel2 = Genre.NibbleList[i].ContentNibble & 0x0F;
					pEvent->ContentNibble.NibbleList[i].UserNibble1 = Genre.NibbleList[i].UserNibble >> 4;
					pEvent->ContentNibble.NibbleList[i].UserNibble2 = Genre.NibbleList[i].UserNibble & 0x0F;
				}
			}
			break;

		case EPGData::Tag::EventName:
			ReadString(File, &pEvent->EventName, &Size);
			break;

		case EPGData::Tag::EventText:
			ReadString(File, &pEvent->EventText, &Size);
			break;

		case EPGData::Tag::EventExtendedText:
			{
				EPGData::EventExtendedTextHeader Header;

				ReadData(File, Header, &Size);

				pEvent->ExtendedText.resize(Header.TextCount);

				for (auto &Text : pEvent->ExtendedText) {
					ReadString(File, &Text.Description, &Size);
					ReadString(File, &Text.Text, &Size);
				}
			}
			break;

		case EPGData::Tag::EventGroup:
			{
				EPGData::EventGroupHeader Header;

				ReadData(File, Header, &Size);

				pEvent->EventGroupList.resize(Header.GroupCount);

				for (auto &Group : pEvent->EventGroupList) {
					EPGData::EventGroupInfoHeader GroupHeader;

					ReadData(File, GroupHeader, &Size);

					Group.GroupType = GroupHeader.GroupType;
					Group.EventList.resize(GroupHeader.EventCount);

					for (auto &Event : Group.EventList) {
						EPGData::EventGroupInfo GroupInfo;

						ReadData(File, GroupInfo, &Size);

						Event.ServiceID         = GroupInfo.ServiceID;
						Event.EventID           = GroupInfo.EventID;
						Event.NetworkID         = GroupInfo.NetworkID;
						Event.TransportStreamID = GroupInfo.TransportStreamID;
					}

					if ((Group.GroupType == EventGroupDescriptor::GROUP_TYPE_COMMON)
							&& (Group.EventList.size() == 1)) {
						auto &Event = Group.EventList.front();
						if (Event.ServiceID != pEvent->ServiceID) {
							pEvent->IsCommonEvent = true;
							pEvent->CommonEvent.ServiceID = Event.ServiceID;
							pEvent->CommonEvent.EventID   = Event.EventID;
						}
					}
				}
			}
			break;

		default:
			if (ChunkHeader.Size > 0) {
				if (!File.SetPos(ChunkHeader.Size, Stream::SetPosType::Current))
					throw Exception::Seek;
			}
			break;
		}
	} while (ChunkHeader.Tag != EPGData::Tag::EventEnd);
}


void EPGDataFile::SaveService(
	FileStream &File, const EPGDatabase::ServiceInfo &ServiceInfo,
	uint16_t EventCount, const DateTime &EarliestTime)
{
	EPGData::ServiceInfo ServiceHeader;
	ServiceHeader.NetworkID         = ServiceInfo.NetworkID;
	ServiceHeader.TransportStreamID = ServiceInfo.TransportStreamID;
	ServiceHeader.ServiceID         = ServiceInfo.ServiceID;
	ServiceHeader.EventCount        = EventCount;
	WriteChunk(File, EPGData::Tag::Service, ServiceHeader);

	m_pEPGDatabase->EnumEventsSortedByTime(
		ServiceInfo.NetworkID, ServiceInfo.TransportStreamID, ServiceInfo.ServiceID,
		!!(m_OpenFlags & OpenFlag::DiscardOld) ? &EarliestTime : nullptr, nullptr,
		[&](const EventInfo &Event) -> bool {
			SaveEvent(File, Event);
			return true;
		});

	WriteChunkHeader(File, EPGData::Tag::ServiceEnd);
}


void EPGDataFile::SaveEvent(FileStream &File, const EventInfo &Event)
{
	EPGData::EventInfo EventHeader;

	EventHeader.EventID     = Event.EventID;
	EventHeader.Flags       = Event.RunningStatus;
	if (Event.FreeCAMode)
		EventHeader.Flags |= EPGData::EventInfo::Flag_FreeCAMode;
	if (Event.HasBasic())
		EventHeader.Flags |= EPGData::EventInfo::Flag_Basic;
	if (Event.HasExtended())
		EventHeader.Flags |= EPGData::EventInfo::Flag_Extended;
	if (Event.IsPresent())
		EventHeader.Flags |= EPGData::EventInfo::Flag_Present;
	if (Event.IsFollowing())
		EventHeader.Flags |= EPGData::EventInfo::Flag_Following;
	EventHeader.StartTime   = Event.StartTime;
	EventHeader.Duration    = Event.Duration;
	EventHeader.UpdatedTime = Event.UpdatedTime;

	WriteChunk(File, EPGData::Tag::Event, EventHeader);

	if (!Event.AudioList.empty()) {
		size_t Size =
			sizeof(EPGData::EventAudioHeader) +
			(Event.AudioList.size() * (sizeof(EPGData::EventAudioInfo) + sizeof(uint16_t)));
		for (auto &e : Event.AudioList)
			Size += e.Text.length() * sizeof(CharType);

		WriteChunkHeader(File, EPGData::Tag::EventAudio, Size);

		EPGData::EventAudioHeader Header;
		Header.AudioCount = static_cast<uint8_t>(Event.AudioList.size());
		WriteData(File, Header);

		for (auto &Audio : Event.AudioList) {
			EPGData::EventAudioInfo AudioInfo;

			AudioInfo.Flags = 0;
			if (Audio.ESMultiLingualFlag)
				AudioInfo.Flags |= EPGData::EventAudioInfo::Flag_MultiLingual;
			if (Audio.MainComponentFlag)
				AudioInfo.Flags |= EPGData::EventAudioInfo::Flag_MainComponent;
			AudioInfo.StreamContent     = Audio.StreamContent;
			AudioInfo.ComponentType     = Audio.ComponentType;
			AudioInfo.ComponentTag      = Audio.ComponentTag;
			AudioInfo.SimulcastGroupTag = Audio.SimulcastGroupTag;
			AudioInfo.QualityIndicator  = Audio.QualityIndicator;
			AudioInfo.SamplingRate      = Audio.SamplingRate;
			AudioInfo.Reserved          = 0;
			AudioInfo.LanguageCode      = Audio.LanguageCode;
			AudioInfo.LanguageCode2     = Audio.LanguageCode2;

			WriteData(File, AudioInfo);
			WriteString(File, Audio.Text);
		}
	}

	if (!Event.VideoList.empty()) {
		size_t Size =
			sizeof(EPGData::EventVideoHeader) +
			(Event.VideoList.size() * (sizeof(EPGData::EventVideoInfo) + sizeof(uint16_t)));
		for (auto &e : Event.VideoList)
			Size += e.Text.length() * sizeof(CharType);

		WriteChunkHeader(File, EPGData::Tag::EventVideo, Size);

		EPGData::EventVideoHeader Header;
		Header.VideoCount = static_cast<uint8_t>(Event.VideoList.size());
		WriteData(File, Header);

		for (auto &Video : Event.VideoList) {
			EPGData::EventVideoInfo VideoInfo;

			VideoInfo.StreamContent = Video.StreamContent;
			VideoInfo.ComponentType = Video.ComponentType;
			VideoInfo.ComponentTag  = Video.ComponentTag;
			VideoInfo.Reserved      = 0;
			VideoInfo.LanguageCode  = Video.LanguageCode;

			WriteData(File, VideoInfo);
			WriteString(File, Video.Text);
		}
	}

	if (Event.ContentNibble.NibbleCount > 0) {
		EPGData::EventGenreInfo Genre;

		Genre.NibbleCount = Event.ContentNibble.NibbleCount;

		for (int i = 0; i < Event.ContentNibble.NibbleCount; i++) {
			Genre.NibbleList[i].ContentNibble =
				(Event.ContentNibble.NibbleList[i].ContentNibbleLevel1 << 4) |
				 Event.ContentNibble.NibbleList[i].ContentNibbleLevel2;
			Genre.NibbleList[i].UserNibble =
				(Event.ContentNibble.NibbleList[i].UserNibble1 << 4) |
				 Event.ContentNibble.NibbleList[i].UserNibble2;
		}

		WriteChunk(File, EPGData::Tag::EventGenre, &Genre, 1 + Genre.NibbleCount * 2);
	}

	if (!Event.EventName.empty()) {
		WriteChunkString(File, EPGData::Tag::EventName, Event.EventName);
	}

	if (!Event.EventText.empty()) {
		WriteChunkString(File, EPGData::Tag::EventText, Event.EventText);
	}

	if (!Event.ExtendedText.empty()) {
		size_t Size =
			sizeof(EPGData::EventExtendedTextHeader) +
			(2 * sizeof(uint16_t) * Event.ExtendedText.size());
		for (auto &e : Event.ExtendedText)
			Size += (e.Description.length() + e.Text.length()) * sizeof(CharType);

		WriteChunkHeader(File, EPGData::Tag::EventExtendedText, Size);

		EPGData::EventExtendedTextHeader Header;

		Header.TextCount = static_cast<uint8_t>(Event.ExtendedText.size());

		WriteData(File, Header);

		for (auto &e : Event.ExtendedText) {
			WriteString(File, e.Description);
			WriteString(File, e.Text);
		}
	}

	if (!Event.EventGroupList.empty()) {
		size_t Size = sizeof(EPGData::EventGroupHeader);
		for (auto &e : Event.EventGroupList)
			Size += sizeof(EPGData::EventGroupInfoHeader) + e.EventList.size() * sizeof(EPGData::EventGroupInfo);

		WriteChunkHeader(File, EPGData::Tag::EventGroup, Size);

		EPGData::EventGroupHeader Header;
		Header.GroupCount = static_cast<uint8_t>(Event.EventGroupList.size());
		WriteData(File, Header);

		for (auto &Group : Event.EventGroupList) {
			EPGData::EventGroupInfoHeader GroupHeader;

			GroupHeader.GroupType = Group.GroupType;
			GroupHeader.EventCount = static_cast<uint8_t>(Group.EventList.size());

			WriteData(File, GroupHeader);

			for (auto &Event : Group.EventList) {
				EPGData::EventGroupInfo GroupInfo;

				GroupInfo.ServiceID         = Event.ServiceID;
				GroupInfo.EventID           = Event.EventID;
				GroupInfo.NetworkID         = Event.NetworkID;
				GroupInfo.TransportStreamID = Event.TransportStreamID;

				WriteData(File, GroupInfo);
			}
		}
	}

	WriteChunkHeader(File, EPGData::Tag::EventEnd);
}


void EPGDataFile::ExceptionLog(Exception Code)
{
	const CharType *pText;

	switch (Code) {
	case Exception::Read:
		pText = LIBISDB_STR("EPGファイルの読み込みエラーが発生しました。");
		break;
	case Exception::Write:
		pText = LIBISDB_STR("EPGファイルの書き出しエラーが発生しました。");
		break;
	case Exception::Seek:
		pText = LIBISDB_STR("EPGファイルのシークエラーが発生しました。");
		break;
	case Exception::MemoryAllocate:
		pText = LIBISDB_STR("メモリが確保できません。");
		break;
	case Exception::FormatError:
		pText = LIBISDB_STR("EPGファイルにエラーがあります。");
		break;
	case Exception::Internal:
		pText = LIBISDB_STR("内部エラーが発生しました。");
		break;
	default:
		return;
	}

	Log(Logger::LogType::Error, pText);
}


}	// namespace LibISDB
