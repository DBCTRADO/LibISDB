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
 @file   EventInfo.cpp
 @brief  番組情報
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "EventInfo.hpp"
#include "../Utilities/Sort.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


bool EventInfo::operator == (const EventInfo &rhs) const noexcept
{
	return IsEqual(rhs)
		&& (Type == rhs.Type)
		&& (UpdatedTime == rhs.UpdatedTime);
}


bool EventInfo::IsEqual(const EventInfo &Op) const noexcept
{
	return (NetworkID == Op.NetworkID)
		&& (TransportStreamID == Op.TransportStreamID)
		&& (ServiceID == Op.ServiceID)
		&& (EventID == Op.EventID)
		&& (StartTime.IsValid() == Op.StartTime.IsValid())
		&& ((!StartTime.IsValid() || (StartTime == Op.StartTime)))
		&& (Duration == Op.Duration)
		&& (RunningStatus == Op.RunningStatus)
		&& (FreeCAMode == Op.FreeCAMode)
		&& (EventName == Op.EventName)
		&& (EventText == Op.EventText)
		&& (ExtendedText == Op.ExtendedText)
		&& (VideoList == Op.VideoList)
		&& (AudioList == Op.AudioList)
		&& (ContentNibble == Op.ContentNibble)
		&& (EventGroupList == Op.EventGroupList)
		&& (IsCommonEvent == Op.IsCommonEvent)
		&& ((!IsCommonEvent || (CommonEvent == Op.CommonEvent)));
}


bool EventInfo::HasBasic() const noexcept
{
	return !!(Type & TypeFlag::Basic);
}


bool EventInfo::HasExtended() const noexcept
{
	return !!(Type & TypeFlag::Extended);
}


bool EventInfo::IsPresent() const noexcept
{
	return !!(Type & TypeFlag::Present);
}


bool EventInfo::IsFollowing() const noexcept
{
	return !!(Type & TypeFlag::Following);
}


bool EventInfo::IsPresentFollowing() const noexcept
{
	return !!(Type & (TypeFlag::Present | TypeFlag::Following));
}


bool EventInfo::IsDatabase() const noexcept
{
	return !!(Type & TypeFlag::Database);
}


bool EventInfo::GetStartTime(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	Time = StartTime;

	return Time->IsValid();
}


bool EventInfo::GetEndTime(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	if (StartTime.IsValid()) {
		Time = StartTime;
		if (Time->OffsetSeconds(Duration))
			return true;
	}

	Time->Reset();

	return false;
}


bool EventInfo::GetStartTimeUTC(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	if (StartTime.IsValid()) {
		if (EPGTimeToUTCTime(StartTime, Time))
			return true;
	}

	Time->Reset();

	return false;
}


bool EventInfo::GetEndTimeUTC(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	if (StartTime.IsValid()) {
		Time = StartTime;
		if (Time->OffsetSeconds((-9 * 60 * 60) + Duration))
			return true;
	}

	Time->Reset();

	return false;
}


bool EventInfo::GetStartTimeLocal(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	if (StartTime.IsValid()) {
		if (EPGTimeToLocalTime(StartTime, Time))
			return true;
	}

	Time->Reset();

	return false;
}


bool EventInfo::GetEndTimeLocal(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	DateTime EndTime;

	if (GetEndTime(&EndTime) && EPGTimeToLocalTime(EndTime, Time))
		return true;

	Time->Reset();

	return false;
}


bool EventInfo::GetConcatenatedExtendedText(ReturnArg<String> Text) const
{
	if (!Text)
		return false;

	Text->clear();

	for (auto it = ExtendedText.begin(); it != ExtendedText.end(); ++it) {
		if (!it->Description.empty()) {
			*Text += it->Description;
			*Text += LIBISDB_STR(LIBISDB_NEWLINE);
		}

		if (!it->Text.empty()) {
			*Text += it->Text;
			if (it + 1 != ExtendedText.end())
				*Text += LIBISDB_STR(LIBISDB_NEWLINE);
		}
	}

	return true;
}


size_t EventInfo::GetConcatenatedExtendedTextLength() const
{
	static const size_t NewLineLength = std::size(LIBISDB_STR(LIBISDB_NEWLINE)) - 1;
	size_t Length = 0;

	for (auto it = ExtendedText.begin(); it != ExtendedText.end(); ++it) {
		if (!it->Description.empty()) {
			Length += it->Description.length();
			Length += NewLineLength;
		}

		if (!it->Text.empty()) {
			Length += it->Text.length();
			if (it + 1 != ExtendedText.end())
				Length += NewLineLength;
		}
	}

	return Length;
}


int EventInfo::GetMainAudioIndex() const
{
	for (size_t i = 0; i < AudioList.size(); i++) {
		if (AudioList[i].MainComponentFlag)
			return (int)i;
	}
	return -1;
}


const EventInfo::AudioInfo * EventInfo::GetMainAudioInfo() const
{
	if (AudioList.empty())
		return nullptr;

	const int MainAudioIndex = GetMainAudioIndex();
	if (MainAudioIndex >= 0)
		return &AudioList[MainAudioIndex];

	return &AudioList[0];
}




// EPGの日時(UTC+9)からUTCに変換する
bool EPGTimeToUTCTime(const DateTime &EPGTime, ReturnArg<DateTime> UTCTime)
{
	if (!UTCTime)
		return false;

	UTCTime = EPGTime;

	if (UTCTime->OffsetSeconds(-9 * 60 * 60))
		return true;

	UTCTime->Reset();

	return false;
}


// UTCからEPGの日時(UTC+9)に変換する
bool UTCTimeToEPGTime(const DateTime &UTCTime, ReturnArg<DateTime> EPGTime)
{
	if (!EPGTime)
		return false;

	EPGTime = UTCTime;

	if (EPGTime->OffsetSeconds(9 * 60 * 60))
		return true;

	EPGTime->Reset();

	return false;
}


// EPGの日時(UTC+9)からローカル日時に変換する
bool EPGTimeToLocalTime(const DateTime &EPGTime, ReturnArg<DateTime> LocalTime)
{
	if (!LocalTime)
		return false;

	if (EPGTimeToUTCTime(EPGTime, LocalTime) && LocalTime->ToLocal())
		return true;

	LocalTime->Reset();

	return false;
}


// 現在の日時をEPGの日時(UTC+9)で取得する
bool GetCurrentEPGTime(ReturnArg<DateTime> Time)
{
	if (!Time)
		return false;

	Time->NowUTC();

	if (Time->OffsetSeconds(9 * 60 * 60))
		return true;

	Time->Reset();

	return false;
}


// 拡張テキストを取得する
bool GetEventExtendedTextList(const DescriptorBlock *pDescBlock, ReturnArg<EventExtendedTextList> List)
{
	if (!List)
		return false;

	List->clear();

	if (pDescBlock == nullptr)
		return false;

	std::vector<const ExtendedEventDescriptor *> DescList;
	pDescBlock->EnumDescriptors<ExtendedEventDescriptor>(
		[&](const ExtendedEventDescriptor *pDesc) {
			DescList.push_back(pDesc);
		});
	if (DescList.empty())
		return false;

	// descriptor_number 順にソートする
	InsertionSort(DescList,
		[](const ExtendedEventDescriptor *pDesc1, const ExtendedEventDescriptor *pDesc2) {
			return pDesc1->GetDescriptorNumber() < pDesc2->GetDescriptorNumber();
		});

	struct ItemInfo {
		uint8_t DescriptorNumber;
		const ARIBString *pDescription;
		const ARIBString *pData1;
		const ARIBString *pData2;
	};
	std::vector<ItemInfo> ItemList;
	for (auto e : DescList) {
		for (int j = 0; j < e->GetItemCount(); j++) {
			const ExtendedEventDescriptor::ItemInfo *pItem = e->GetItem(j);
			if (pItem == nullptr)
				continue;
			if (!pItem->Description.empty()) {
				// 新規項目
				ItemInfo Item;
				Item.DescriptorNumber = e->GetDescriptorNumber();
				Item.pDescription = &pItem->Description;
				Item.pData1 = &pItem->ItemChar;
				Item.pData2 = nullptr;
				ItemList.push_back(Item);
			} else if (!ItemList.empty()) {
				// 前の項目の続き
				ItemInfo &Item = ItemList[ItemList.size() - 1];
				if ((Item.DescriptorNumber == e->GetDescriptorNumber() - 1)
						&& (Item.pData2 == nullptr)) {
					Item.pData2 = &pItem->ItemChar;
				}
			}
		}
	}

	List->resize(ItemList.size());
	for (size_t i = 0; i < ItemList.size(); i++) {
		EventExtendedTextItem &Item = (*List)[i];
		Item.Description = *ItemList[i].pDescription;
		Item.Text = *ItemList[i].pData1;
		if (ItemList[i].pData2 != nullptr)
			Item.Text += *ItemList[i].pData2;
	}

	return true;
}


static void CanonicalizeExtendedText(const String &Src, ReturnArg<String> Dst)
{
	for (auto it = Src.begin(); it != Src.end();) {
		if (*it == LIBISDB_CHAR('\r')) {
			*Dst += LIBISDB_STR(LIBISDB_NEWLINE);
			++it;
			if (it == Src.end())
				break;
			if (*it == LIBISDB_CHAR('\n'))
				++it;
		} else {
			Dst->push_back(*it);
			++it;
		}
	}
}


bool GetEventExtendedTextList(
	const DescriptorBlock *pDescBlock,
	ARIBStringDecoder &StringDecoder, ARIBStringDecoder::DecodeFlag DecodeFlags,
	ReturnArg<EventInfo::ExtendedTextInfoList> List)
{
	EventExtendedTextList TextList;

	if (!GetEventExtendedTextList(pDescBlock, &TextList))
		return false;

	List->clear();
	List->reserve(TextList.size());

	String Buffer;

	for (auto const &e : TextList) {
		EventInfo::ExtendedTextInfo &Text = List->emplace_back();
		StringDecoder.Decode(e.Description, &Text.Description, DecodeFlags);
		if (StringDecoder.Decode(e.Text, &Buffer, DecodeFlags))
			CanonicalizeExtendedText(Buffer, &Text.Text);
	}

	return true;
}


bool GetConcatenatedEventExtendedText(
	const EventExtendedTextList &List, ARIBStringDecoder &StringDecoder,
	ARIBStringDecoder::DecodeFlag DecodeFlags, ReturnArg<String> Text)
{
	if (!Text)
		return false;

	Text->clear();

	String Buffer;

	for (auto &e : List) {
		if (StringDecoder.Decode(e.Description, &Buffer, DecodeFlags)) {
			*Text += Buffer;
			*Text += LIBISDB_STR(LIBISDB_NEWLINE);
		}

		if (StringDecoder.Decode(e.Text, &Buffer, DecodeFlags)) {
			CanonicalizeExtendedText(Buffer, Text);
			*Text += LIBISDB_STR(LIBISDB_NEWLINE);
		}
	}

	return true;
}


bool GetEventExtendedText(
	const DescriptorBlock *pDescBlock, ARIBStringDecoder &StringDecoder,
	ARIBStringDecoder::DecodeFlag DecodeFlags, ReturnArg<String> Text)
{
	EventExtendedTextList List;

	if (!GetEventExtendedTextList(pDescBlock, &List))
		return false;
	return GetConcatenatedEventExtendedText(List, StringDecoder, DecodeFlags, Text);
}


}	// namespace LibISDB
