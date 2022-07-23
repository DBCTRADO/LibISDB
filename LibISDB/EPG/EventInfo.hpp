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
 @file   EventInfo.hpp
 @brief  番組情報
 @author DBCTRADO
*/


#ifndef LIBISDB_EVENT_INFO_H
#define LIBISDB_EVENT_INFO_H


#include "../TS/Descriptors.hpp"
#include "../TS/DescriptorBlock.hpp"


namespace LibISDB
{

	/** 番組情報クラス */
	class EventInfo
	{
	public:
		struct ExtendedTextInfo {
			String Description;
			String Text;

			bool operator == (const ExtendedTextInfo &rhs) const noexcept
			{
				return (Description == rhs.Description)
					&& (Text == rhs.Text);
			}

			bool operator != (const ExtendedTextInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		struct VideoInfo {
			uint8_t StreamContent = STREAM_CONTENT_INVALID;
			uint8_t ComponentType = COMPONENT_TYPE_INVALID;
			uint8_t ComponentTag = COMPONENT_TAG_INVALID;
			uint32_t LanguageCode = LANGUAGE_CODE_INVALID;
			String Text;

			bool operator == (const VideoInfo &rhs) const noexcept
			{
				return (StreamContent == rhs.StreamContent)
					&& (ComponentType == rhs.ComponentType)
					&& (ComponentTag == rhs.ComponentTag)
					&& (LanguageCode == rhs.LanguageCode)
					&& (Text == rhs.Text);
			}

			bool operator != (const VideoInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		struct AudioInfo {
			uint8_t StreamContent;
			uint8_t ComponentType;
			uint8_t ComponentTag;
			uint8_t SimulcastGroupTag;
			bool ESMultiLingualFlag;
			bool MainComponentFlag;
			uint8_t QualityIndicator;
			uint8_t SamplingRate;
			uint32_t LanguageCode;
			uint32_t LanguageCode2;
			String Text;

			bool operator == (const AudioInfo &rhs) const noexcept
			{
				return (StreamContent == rhs.StreamContent)
					&& (ComponentType == rhs.ComponentType)
					&& (ComponentTag == rhs.ComponentTag)
					&& (SimulcastGroupTag == rhs.SimulcastGroupTag)
					&& (ESMultiLingualFlag == rhs.ESMultiLingualFlag)
					&& (MainComponentFlag == rhs.MainComponentFlag)
					&& (QualityIndicator == rhs.QualityIndicator)
					&& (SamplingRate == rhs.SamplingRate)
					&& (LanguageCode == rhs.LanguageCode)
					&& (LanguageCode2 == rhs.LanguageCode2)
					&& (Text == rhs.Text);
			}

			bool operator != (const AudioInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		struct ContentNibbleInfo {
			int NibbleCount = 0;
			ContentDescriptor::NibbleInfo NibbleList[7];

			bool operator == (const ContentNibbleInfo &rhs) const noexcept
			{
				if (NibbleCount == rhs.NibbleCount) {
					for (int i = 0; i < NibbleCount; i++) {
						if (NibbleList[i] != rhs.NibbleList[i])
							return false;
					}
					return true;
				}
				return false;
			}

			bool operator != (const ContentNibbleInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		struct EventGroupInfo {
			uint8_t GroupType = EventGroupDescriptor::GROUP_TYPE_UNDEFINED;
			std::vector<EventGroupDescriptor::EventInfo> EventList;

			bool operator == (const EventGroupInfo &rhs) const noexcept
			{
				return (GroupType == rhs.GroupType)
					&& (EventList == rhs.EventList);
			}

			bool operator != (const EventGroupInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		struct CommonEventInfo {
			uint16_t ServiceID = SERVICE_ID_INVALID;
			uint16_t EventID = EVENT_ID_INVALID;

			bool operator == (const CommonEventInfo &rhs) const noexcept
			{
				return (ServiceID == rhs.ServiceID)
					&& (EventID == rhs.EventID);
			}

			bool operator != (const CommonEventInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		struct SeriesInfo {
			uint16_t SeriesID;
			uint8_t RepeatLabel;
			uint8_t ProgramPattern;
			DateTime ExpireDate;
			uint16_t EpisodeNumber;
			uint16_t LastEpisodeNumber;
			String SeriesName;
		};

		typedef std::vector<ExtendedTextInfo> ExtendedTextInfoList;
		typedef std::vector<VideoInfo> VideoInfoList;
		typedef std::vector<AudioInfo> AudioInfoList;
		typedef std::vector<EventGroupInfo> EventGroupInfoList;

		enum class TypeFlag : unsigned int {
			None      = 0x0000U,
			Basic     = 0x0001U,
			Extended  = 0x0002U,
			Present   = 0x0004U,
			Following = 0x0008U,
			Database  = 0x0010U,
		};

		typedef unsigned int SourceIDType;

		uint16_t NetworkID;
		uint16_t TransportStreamID;
		uint16_t ServiceID;
		uint16_t EventID;
		DateTime StartTime;
		uint32_t Duration;
		uint8_t RunningStatus;
		bool FreeCAMode;
		String EventName;
		String EventText;
		ExtendedTextInfoList ExtendedText;
		VideoInfoList VideoList;
		AudioInfoList AudioList;
		ContentNibbleInfo ContentNibble;
		EventGroupInfoList EventGroupList;
		bool IsCommonEvent = false;
		CommonEventInfo CommonEvent;
		TypeFlag Type = TypeFlag::None;
		unsigned long long UpdatedTime = 0;
		SourceIDType SourceID = 0;

		bool operator == (const EventInfo &rhs) const noexcept;
		bool operator != (const EventInfo &rhs) const noexcept { return !(*this == rhs); }

		bool IsEqual(const EventInfo &Op) const noexcept;

		bool HasBasic() const noexcept;
		bool HasExtended() const noexcept;
		bool IsPresent() const noexcept;
		bool IsFollowing() const noexcept;
		bool IsPresentFollowing() const noexcept;
		bool IsDatabase() const noexcept;

		bool GetStartTime(ReturnArg<DateTime> Time) const;
		bool GetEndTime(ReturnArg<DateTime> Time) const;
		bool GetStartTimeUTC(ReturnArg<DateTime> Time) const;
		bool GetEndTimeUTC(ReturnArg<DateTime> Time) const;
		bool GetStartTimeLocal(ReturnArg<DateTime> Time) const;
		bool GetEndTimeLocal(ReturnArg<DateTime> Time) const;

		bool GetConcatenatedExtendedText(ReturnArg<String> Text) const;
		size_t GetConcatenatedExtendedTextLength() const;

		int GetMainAudioIndex() const;
		const AudioInfo * GetMainAudioInfo() const;
	};

	LIBISDB_ENUM_FLAGS(EventInfo::TypeFlag)

	bool EPGTimeToUTCTime(const DateTime &EPGTime, ReturnArg<DateTime> UTCTime);
	bool UTCTimeToEPGTime(const DateTime &UTCTime, ReturnArg<DateTime> EPGTime);
	bool EPGTimeToLocalTime(const DateTime &EPGTime, ReturnArg<DateTime> LocalTime);
	bool GetCurrentEPGTime(ReturnArg<DateTime> Time);

	struct EventExtendedTextItem {
		ARIBString Description;
		ARIBString Text;
	};
	typedef std::vector<EventExtendedTextItem> EventExtendedTextList;

	bool GetEventExtendedTextList(
		const DescriptorBlock *pDescBlock, ReturnArg<EventExtendedTextList> List);
	bool GetEventExtendedTextList(
		const DescriptorBlock *pDescBlock,
		ARIBStringDecoder &StringDecoder, ARIBStringDecoder::DecodeFlag DecodeFlags,
		ReturnArg<EventInfo::ExtendedTextInfoList> List);
	bool GetConcatenatedEventExtendedText(
		const EventExtendedTextList &List, ARIBStringDecoder &StringDecoder,
		ARIBStringDecoder::DecodeFlag DecodeFlags, ReturnArg<String> Text);
	bool GetEventExtendedText(
		const DescriptorBlock *pDescBlock, ARIBStringDecoder &StringDecoder,
		ARIBStringDecoder::DecodeFlag DecodeFlags, ReturnArg<String> Text);

}	// namespace LibISDB


#endif	// ifndef LIBISDB_EVENT_INFO_H
