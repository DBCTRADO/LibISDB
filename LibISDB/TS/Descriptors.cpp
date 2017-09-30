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
 @file   Descriptors.cpp
 @brief  各種記述子
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "Descriptors.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/StringUtilities.hpp"
#include "../Base/ARIBTime.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


CADescriptor::CADescriptor()
{
	Reset();
}


void CADescriptor::Reset()
{
	DescriptorBase::Reset();

	m_CASystemID = 0;
	m_CAPID = PID_INVALID;
	m_PrivateData.ClearSize();
}


bool CADescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 4)
		return false;
	if ((pPayload[2] & 0xE0) != 0xE0)
		return false;

	m_CASystemID = Load16(&pPayload[0]);
	m_CAPID      = Load16(&pPayload[2]) & 0x1FFF_u16;
	m_PrivateData.SetData(&pPayload[4], m_Length - 4);

	return true;
}




void NetworkNameDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_NetworkName.clear();
}


bool NetworkNameDescriptor::GetNetworkName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_NetworkName;

	return !Name->empty();
}


bool NetworkNameDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	if (m_Length > 0)
		m_NetworkName.assign(&pPayload[0], m_Length);
	else
		m_NetworkName.clear();

	return true;
}




void ServiceListDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_ServiceList.clear();
}


int ServiceListDescriptor::GetServiceCount() const
{
	return static_cast<int>(m_ServiceList.size());
}


int ServiceListDescriptor::GetServiceIndexByID(uint16_t ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return static_cast<int>(i);
	}
	return -1;
}


uint8_t ServiceListDescriptor::GetServiceTypeByID(uint16_t ServiceID) const
{
	int Index = GetServiceIndexByID(ServiceID);
	if (Index >= 0)
		return m_ServiceList[Index].ServiceType;
	return SERVICE_TYPE_INVALID;
}


bool ServiceListDescriptor::GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const
{
	if (!Info)
		return false;
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	*Info = m_ServiceList[Index];

	return true;
}


bool ServiceListDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	const int ServiceCount = m_Length / 3;

	m_ServiceList.resize(ServiceCount);

	int Pos = 0;
	for (int i = 0; i < ServiceCount; i++) {
		m_ServiceList[i].ServiceID   = Load16(&pPayload[Pos + 0]);
		m_ServiceList[i].ServiceType = pPayload[Pos + 2];
		Pos += 3;
	}

	return true;
}




SatelliteDeliverySystemDescriptor::SatelliteDeliverySystemDescriptor()
{
	Reset();
}


void SatelliteDeliverySystemDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_Frequency = 0;
	m_OrbitalPosition = 0;
	m_WestEastFlag = false;
	m_Polarization = 0xFF;
	m_Modulation = 0;
	m_SymbolRate = 0;
	m_FECInner = 0;
}


bool SatelliteDeliverySystemDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length != 11)
		return false;

	m_Frequency       = GetBCD(&pPayload[0], 8);
	m_OrbitalPosition = (uint16_t)GetBCD(&pPayload[4], 4);
	m_WestEastFlag    = (pPayload[6] & 0x80) != 0;
	m_Polarization    = (pPayload[6] >> 5) & 0x03;
	m_Modulation      = pPayload[6] & 0x1F;
	m_SymbolRate      = GetBCD(&pPayload[7], 7);
	m_FECInner        = pPayload[10] & 0x0F;

	return true;
}




ServiceDescriptor::ServiceDescriptor()
{
	Reset();
}


void ServiceDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_ServiceType = SERVICE_TYPE_INVALID;
	m_ProviderName.clear();
	m_ServiceName.clear();
}


bool ServiceDescriptor::GetProviderName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_ProviderName;

	return !Name->empty();
}


bool ServiceDescriptor::GetServiceName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_ServiceName;

	return !Name->empty();
}


bool ServiceDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 3)
		return false;

	m_ServiceType = pPayload[0];

	int Pos = 1, Length;

	// Provider Name
	Length = pPayload[Pos++];
	m_ProviderName.clear();
	if (Length > 0) {
		if (Pos + Length >= m_Length)
			return false;
		m_ProviderName.assign(&pPayload[Pos], Length);
		Pos += Length;
	}

	// Service Name
	Length = pPayload[Pos++];
	m_ServiceName.clear();
	if (Length > 0) {
		if (Pos + Length > m_Length)
			return false;
		m_ServiceName.assign(&pPayload[Pos], Length);
	}

	return true;
}




LinkageDescriptor::LinkageDescriptor()
{
	Reset();
}


void LinkageDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_ServiceID = SERVICE_ID_INVALID;
	m_LinkageType = 0;
	m_PrivateData.ClearSize();
}


bool LinkageDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 7)
		return false;

	m_TransportStreamID = Load16(&pPayload[0]);
	m_OriginalNetworkID = Load16(&pPayload[2]);
	m_ServiceID         = Load16(&pPayload[4]);
	m_LinkageType       = pPayload[6];
	m_PrivateData.SetData(&pPayload[7], m_Length - 7);

	return true;
}




ShortEventDescriptor::ShortEventDescriptor()
{
	Reset();
}


void ShortEventDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_EventName.clear();
	m_EventDescription.clear();
}


bool ShortEventDescriptor::GetEventName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_EventName;

	return !Name->empty();
}


bool ShortEventDescriptor::GetEventDescription(ReturnArg<ARIBString> Description) const
{
	if (!Description)
		return false;

	*Description = m_EventDescription;

	return !Description->empty();
}


bool ShortEventDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 5)
		return false;

	m_LanguageCode = Load24(&pPayload[0]);

	int Pos = 3, Length;

	// Event Name
	Length = pPayload[Pos++];
	m_EventName.clear();
	if (Length > 0) {
		if (Pos + Length >= m_Length)
			return false;
		m_EventName.assign(&pPayload[Pos], Length);
		Pos += Length;
	}

	// Event Description
	Length = pPayload[Pos++];
	m_EventDescription.clear();
	if (Length > 0) {
		if (Pos + Length > m_Length)
			return false;
		m_EventDescription.assign(&pPayload[Pos], Length);
	}

	return true;
}




ExtendedEventDescriptor::ExtendedEventDescriptor()
{
	Reset();
}


void ExtendedEventDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_DescriptorNumber = 0;
	m_LastDescriptorNumber = 0;
	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_ItemList.clear();
}


int ExtendedEventDescriptor::GetItemCount() const
{
	return static_cast<int>(m_ItemList.size());
}


const ExtendedEventDescriptor::ItemInfo * ExtendedEventDescriptor::GetItem(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ItemList.size())
		return nullptr;
	return &m_ItemList[Index];
}


bool ExtendedEventDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 5)
		return false;

	m_DescriptorNumber     = pPayload[0] >> 4;
	m_LastDescriptorNumber = pPayload[0] & 0x0F;
	m_LanguageCode         = Load24(&pPayload[1]);

	m_ItemList.clear();

	const uint8_t ItemLength = pPayload[4];
	const size_t EndPos = 5 + ItemLength;
	if (EndPos > m_Length)
		return false;
	size_t Pos = 5;

	while (Pos < EndPos) {
		ItemInfo Item;

		const uint8_t DescriptionLength = pPayload[Pos++];
		if (Pos + DescriptionLength > EndPos)
			break;
		if (DescriptionLength > 0) {
			Item.Description.assign(&pPayload[Pos], DescriptionLength);
			Pos += DescriptionLength;
		}

		const uint8_t CharLength = pPayload[Pos++];
		if (Pos + CharLength > EndPos)
			break;
		Item.ItemChar.assign(&pPayload[Pos], std::min(CharLength, 220_u8));
		Pos += CharLength;

		m_ItemList.push_back(std::move(Item));
	}

	return true;
}




ComponentDescriptor::ComponentDescriptor()
{
	Reset();
}


void ComponentDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_StreamContent = STREAM_CONTENT_INVALID;
	m_ComponentType = COMPONENT_TYPE_INVALID;
	m_ComponentTag = COMPONENT_TAG_INVALID;
	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_Text.clear();
}


bool ComponentDescriptor::GetText(ReturnArg<ARIBString> Text) const
{
	if (!Text)
		return false;

	*Text = m_Text;

	return !Text->empty();
}


bool ComponentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 6)
		return false;

	m_StreamContent = pPayload[0] & 0x0F;
	if (m_StreamContent != 0x01)
		return false;
	m_ComponentType = pPayload[1];
	m_ComponentTag  = pPayload[2];
	m_LanguageCode  = Load24(&pPayload[3]);
	m_Text.clear();
	if (m_Length > 6)
		m_Text.assign(&pPayload[6], std::min(m_Length - 6, 16));

	return true;
}




StreamIDDescriptor::StreamIDDescriptor()
{
	Reset();
}


void StreamIDDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_ComponentTag = COMPONENT_TAG_INVALID;
}


bool StreamIDDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length != 1)
		return false;

	m_ComponentTag = pPayload[0];

	return true;
}




ContentDescriptor::ContentDescriptor()
{
	Reset();
}


void ContentDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_NibbleCount = 0;
}


int ContentDescriptor::GetNibbleCount() const
{
	return m_NibbleCount;
}


bool ContentDescriptor::GetNibble(int Index, ReturnArg<NibbleInfo> Nibble) const
{
	if ((Index < 0) || (Index >= m_NibbleCount) || !Nibble)
		return false;
	*Nibble = m_NibbleList[Index];
	return true;
}


bool ContentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length > 14)
		return false;

	m_NibbleCount = m_Length / 2;

	for (int i = 0; i < m_NibbleCount; i++) {
		m_NibbleList[i].ContentNibbleLevel1 = pPayload[i * 2 + 0] >> 4;
		m_NibbleList[i].ContentNibbleLevel2 = pPayload[i * 2 + 0] & 0x0F;
		m_NibbleList[i].UserNibble1         = pPayload[i * 2 + 1] >> 4;
		m_NibbleList[i].UserNibble2         = pPayload[i * 2 + 1] & 0x0F;
	}

	return true;
}




void LocalTimeOffsetDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_TimeOffsetList.clear();
}


int LocalTimeOffsetDescriptor::GetTimeOffsetInfoCount() const
{
	return static_cast<int>(m_TimeOffsetList.size());
}


bool LocalTimeOffsetDescriptor::GetTimeOffsetInfo(int Index, ReturnArg<TimeOffsetInfo> Info) const
{
	if (!Info)
		return false;
	if (static_cast<unsigned int>(Index) >= m_TimeOffsetList.size())
		return false;

	*Info = m_TimeOffsetList[Index];

	return true;
}


bool LocalTimeOffsetDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 13)
		return false;

	m_TimeOffsetList.resize(m_Length / 13);

	size_t Pos = 0;

	for (TimeOffsetInfo &Info : m_TimeOffsetList) {
		Info.CountryCode             = Load24(&pPayload[Pos]);
		Info.CountryRegionID         = (pPayload[Pos + 3] & 0xFC) >> 2;
		Info.LocalTimeOffsetPolarity = (pPayload[Pos + 3] & 0x01) != 0;
		Info.LocalTimeOffset         = BCDTimeHMToMinute(Load16(&pPayload[Pos + 4]));
		MJDBCDTimeToDateTime(&pPayload[Pos + 6], &Info.TimeOfChange);
		Info.NextTimeOffset          = BCDTimeHMToMinute(Load16(&pPayload[Pos + 11]));

		Pos += 13;
	}

	return true;
}




HierarchicalTransmissionDescriptor::HierarchicalTransmissionDescriptor()
{
	Reset();
}


void HierarchicalTransmissionDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_QualityLevel = 0xFF_u8;
	m_ReferencePID = PID_INVALID;
}


bool HierarchicalTransmissionDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length != 3)
		return false;

	m_QualityLevel = pPayload[0] & 0x01;
	m_ReferencePID = Load16(&pPayload[1]) & 0x1FFF_u16;

	return true;
}




DigitalCopyControlDescriptor::DigitalCopyControlDescriptor()
{
	Reset();
}


void DigitalCopyControlDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_DigitalRecordingControlData = 0;
	m_MaximumBitRateFlag = false;
	m_ComponentControlFlag = false;
	m_CopyControlType = 0;
	m_APSControlData = 0;
	m_MaximumBitRate = 0;
	m_ComponentControlList.clear();
}


int DigitalCopyControlDescriptor::GetComponentControlCount() const
{
	return static_cast<int>(m_ComponentControlList.size());
}


bool DigitalCopyControlDescriptor::GetComponentControlInfo(int Index, ReturnArg<ComponentControlInfo> Info) const
{
	if (!Info)
		return false;
	if (static_cast<unsigned int>(Index) >= m_ComponentControlList.size())
		return false;

	*Info = m_ComponentControlList[Index];

	return true;
}


bool DigitalCopyControlDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_DigitalRecordingControlData = pPayload[0] >> 6;
	m_MaximumBitRateFlag          = (pPayload[0] & 0x20) != 0;
	m_ComponentControlFlag        = (pPayload[0] & 0x10) != 0;
	m_CopyControlType             = (pPayload[0] >> 2) & 0x03;
	if ((m_CopyControlType == 1) || (m_CopyControlType == 3))
		m_APSControlData          = pPayload[0] & 0x03;

	size_t Pos = 1;

	if (m_MaximumBitRateFlag) {
		if (m_Length < 2)
			return false;
		m_MaximumBitRate = pPayload[Pos++];
	}

	m_ComponentControlList.clear();

	if (m_ComponentControlFlag) {
		if (Pos + 1 > m_Length)
			return false;

		const uint8_t ComponentControlLength = pPayload[Pos++];
		const size_t EndPos = Pos + ComponentControlLength;
		if (EndPos > m_Length)
			return false;

		while (Pos + 2 <= EndPos) {
			ComponentControlInfo Info;

			Info.ComponentTag                = pPayload[Pos++];
			Info.DigitalRecordingControlData = pPayload[Pos] >> 6;
			Info.MaximumBitRateFlag          = (pPayload[Pos] & 0x20) != 0;
			Info.CopyControlType             = (pPayload[Pos] >> 2) & 0x03;
			if ((Info.CopyControlType == 1) || (Info.CopyControlType == 3))
				Info.APSControlData          = pPayload[Pos] & 0x03;
			Pos++;
			if (Info.MaximumBitRateFlag) {
				if (Pos >= EndPos)
					break;
				Info.MaximumBitRate          = pPayload[Pos++];
			}

			m_ComponentControlList.push_back(Info);
		}
	}

	return true;
}




AudioComponentDescriptor::AudioComponentDescriptor()
{
	Reset();
}


void AudioComponentDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_StreamContent = STREAM_CONTENT_INVALID;
	m_ComponentType = COMPONENT_TYPE_INVALID;
	m_ComponentTag = COMPONENT_TAG_INVALID;
	m_StreamType = STREAM_TYPE_INVALID;
	m_SimulcastGroupTag = 0;
	m_ESMultiLingualFlag = false;
	m_MainComponentFlag = false;
	m_QualityIndicator = 0;
	m_SamplingRate = 0;
	m_LanguageCode = LANGUAGE_CODE_INVALID;
	m_LanguageCode2 = LANGUAGE_CODE_INVALID;
	m_Text.clear();
}


bool AudioComponentDescriptor::GetText(ReturnArg<ARIBString> Text) const
{
	if (!Text)
		return false;

	*Text = m_Text;

	return !Text->empty();
}


bool AudioComponentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 9)
		return false;

	m_StreamContent      = pPayload[0] & 0x0F;
	if (m_StreamContent != 0x02)
		return false;
	m_ComponentType      = pPayload[1];
	m_ComponentTag       = pPayload[2];
	m_StreamType         = pPayload[3];
	m_SimulcastGroupTag  = pPayload[4];
	m_ESMultiLingualFlag = (pPayload[5] & 0x80) != 0;
	m_MainComponentFlag  = (pPayload[5] & 0x40) != 0;
	m_QualityIndicator   = (pPayload[5] & 0x30) >> 4;
	m_SamplingRate       = (pPayload[5] & 0x0E) >> 1;
	m_LanguageCode       = Load24(&pPayload[6]);
	size_t Pos = 9;
	if (m_ESMultiLingualFlag) {
		if (Pos + 3 > m_Length)
			return false;
		m_LanguageCode2 = Load24(&pPayload[Pos]);
		Pos += 3;
	}
	if (Pos < m_Length)
		m_Text.assign(&pPayload[Pos], std::min(static_cast<size_t>(m_Length) - Pos, 33_z));
	else
		m_Text.clear();

	return true;
}




HyperLinkDescriptor::HyperLinkDescriptor()
{
	Reset();
}


void HyperLinkDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_HyperLinkageType = 0;
	m_LinkDestinationType = 0;
	m_SelectorInfo = SelectorInfo();
}


bool HyperLinkDescriptor::GetSelectorInfo(ReturnArg<SelectorInfo> Info) const
{
	if (!Info)
		return false;
	if ((m_LinkDestinationType < 0x01) || (m_LinkDestinationType > 0x07))
		return false;

	*Info = m_SelectorInfo;

	return true;
}


bool HyperLinkDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 3)
		return false;

	m_HyperLinkageType            = pPayload[0];
	m_LinkDestinationType         = pPayload[1];
	m_SelectorInfo.SelectorLength = pPayload[2];

	if (3 + m_SelectorInfo.SelectorLength > m_Length)
		return false;

	switch (m_LinkDestinationType) {
	case LINK_DESTINATION_TYPE_LINK_TO_SERVICE:
		if (m_SelectorInfo.SelectorLength != 6)
			return false;
		m_SelectorInfo.LinkServiceInfo.OriginalNetworkID = Load16(&pPayload[3]);
		m_SelectorInfo.LinkServiceInfo.TransportStreamID = Load16(&pPayload[5]);
		m_SelectorInfo.LinkServiceInfo.ServiceID         = Load16(&pPayload[7]);
		break;

	case LINK_DESTINATION_TYPE_LINK_TO_EVENT:
		if (m_SelectorInfo.SelectorLength != 8)
			return false;
		m_SelectorInfo.LinkEventInfo.OriginalNetworkID = Load16(&pPayload[3]);
		m_SelectorInfo.LinkEventInfo.TransportStreamID = Load16(&pPayload[5]);
		m_SelectorInfo.LinkEventInfo.ServiceID         = Load16(&pPayload[7]);
		m_SelectorInfo.LinkEventInfo.EventID           = Load16(&pPayload[9]);
		break;

	case LINK_DESTINATION_TYPE_LINK_TO_MODULE:
		if (m_SelectorInfo.SelectorLength != 11)
			return false;
		m_SelectorInfo.LinkModuleInfo.OriginalNetworkID = Load16(&pPayload[3]);
		m_SelectorInfo.LinkModuleInfo.TransportStreamID = Load16(&pPayload[5]);
		m_SelectorInfo.LinkModuleInfo.ServiceID         = Load16(&pPayload[7]);
		m_SelectorInfo.LinkModuleInfo.EventID           = Load16(&pPayload[9]);
		m_SelectorInfo.LinkModuleInfo.ComponentTag      = pPayload[11];
		m_SelectorInfo.LinkModuleInfo.ModuleID          = Load16(&pPayload[12]);
		break;

	case LINK_DESTINATION_TYPE_LINK_TO_CONTENT:
		if (m_SelectorInfo.SelectorLength != 10)
			return false;
		m_SelectorInfo.LinkContentInfo.OriginalNetworkID = Load16(&pPayload[3]);
		m_SelectorInfo.LinkContentInfo.TransportStreamID = Load16(&pPayload[5]);
		m_SelectorInfo.LinkContentInfo.ServiceID         = Load16(&pPayload[7]);
		m_SelectorInfo.LinkContentInfo.ContentID         = Load32(&pPayload[9]);
		break;

	case LINK_DESTINATION_TYPE_LINK_TO_CONTENT_MODULE:
		if (m_SelectorInfo.SelectorLength != 13)
			return false;
		m_SelectorInfo.LinkContentModuleInfo.OriginalNetworkID = Load16(&pPayload[3]);
		m_SelectorInfo.LinkContentModuleInfo.TransportStreamID = Load16(&pPayload[5]);
		m_SelectorInfo.LinkContentModuleInfo.ServiceID         = Load16(&pPayload[7]);
		m_SelectorInfo.LinkContentModuleInfo.ContentID         = Load32(&pPayload[9]);
		m_SelectorInfo.LinkContentModuleInfo.ComponentTag      = pPayload[13];
		m_SelectorInfo.LinkContentModuleInfo.ModuleID          = Load16(&pPayload[14]);
		break;

	case LINK_DESTINATION_TYPE_LINK_TO_ERT_MODE:
		if (m_SelectorInfo.SelectorLength != 6)
			return false;
		m_SelectorInfo.LinkERTNodeInfo.InformationProviderID = Load16(&pPayload[3]);
		m_SelectorInfo.LinkERTNodeInfo.TransportStreamID     = Load16(&pPayload[5]);
		m_SelectorInfo.LinkERTNodeInfo.NodeID                = Load16(&pPayload[7]);
		break;

	case LINK_DESTINATION_TYPE_LINK_TO_STORED_CONTENT:
		std::memcpy(m_SelectorInfo.LinkStoredContentInfo.URIChar, &pPayload[3], m_SelectorInfo.SelectorLength);
		break;
	}

	return true;
}




TargetRegionDescriptor::TargetRegionDescriptor()
{
	Reset();
}


void TargetRegionDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_RegionSpecType = 0;
	m_TargetRegionSpec = TargetRegionSpec();
}


bool TargetRegionDescriptor::GetTargetRegionSpec(ReturnArg<TargetRegionSpec> Spec) const
{
	if (!Spec)
		return false;

	*Spec = m_TargetRegionSpec;

	return true;
}


bool TargetRegionDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_RegionSpecType = pPayload[0];

	if (m_RegionSpecType == REGION_SPEC_TYPE_BS) {
		if (m_Length != 1 + 7)
			return false;
		std::memcpy(m_TargetRegionSpec.BS.PrefectureBitmap, &pPayload[1], 7);
	}

	return true;
}




VideoDecodeControlDescriptor::VideoDecodeControlDescriptor()
{
	Reset();
}


void VideoDecodeControlDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_StillPictureFlag = false;
	m_SequenceEndCodeFlag = false;
	m_VideoEncodeFormat = 0xFF_u8;
}


bool VideoDecodeControlDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length != 1)
		return false;

	const uint8_t Data = pPayload[0];

	m_StillPictureFlag    = (Data & 0x80) != 0;
	m_SequenceEndCodeFlag = (Data & 0x40) != 0;
	m_VideoEncodeFormat   = (Data >> 2) & 0x0F;

	return true;
}




DownloadContentDescriptor::DownloadContentDescriptor()
	: m_Info()
{
}


void DownloadContentDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_Info = DownloadContentInfo();
}


bool DownloadContentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 18)
		return false;

	m_Info.Reboot            = (pPayload[0] & 0x80) != 0;
	m_Info.AddOn             = (pPayload[0] & 0x40) != 0;
	m_Info.CompatibilityFlag = (pPayload[0] & 0x20) != 0;
	m_Info.ModuleInfoFlag    = (pPayload[0] & 0x10) != 0;
	m_Info.TextInfoFlag      = (pPayload[0] & 0x80) != 0;
	m_Info.ComponentSize     = Load32(&pPayload[1]);
	m_Info.DownloadID        = Load32(&pPayload[5]);
	m_Info.TimeOutValueDII   = Load32(&pPayload[9]);
	m_Info.LeakRate          = Load24(&pPayload[13]) >> 2;
	m_Info.ComponentTag      = pPayload[16];

	size_t Pos = 17;

	if (m_Info.CompatibilityFlag) {
		if (Pos + 4 > m_Length)
			return false;
		const uint16_t CompatibilityDescriptorLength = Load16(&pPayload[Pos + 0]);
		const uint16_t DescriptorCount               = Load16(&pPayload[Pos + 2]);
		Pos += 4;
		if (Pos + CompatibilityDescriptorLength > m_Length)
			return false;
		m_Info.CompatibilityDescriptor.DescriptorList.resize(DescriptorCount);
		for (auto &Descriptor : m_Info.CompatibilityDescriptor.DescriptorList) {
			if (Pos + 11 > m_Length)
				return false;
			Descriptor.DescriptorType = pPayload[Pos + 0];
			const uint8_t DescriptorLength = pPayload[Pos + 1];
			Descriptor.SpecifierType = pPayload[Pos + 2];
			Descriptor.SpecifierData = Load24(&pPayload[Pos + 3]);
			Descriptor.Model         = Load16(&pPayload[Pos + 6]);
			Descriptor.Version       = Load16(&pPayload[Pos + 8]);

			const uint8_t SubDescriptorCount = pPayload[Pos + 10];
			Pos += 11;
			Descriptor.SubDescriptorList.resize(SubDescriptorCount);
			for (auto &SubDescriptor : Descriptor.SubDescriptorList) {
				if (Pos + 2 > m_Length)
					return false;
				SubDescriptor.SubDescriptorType = pPayload[Pos + 0];
				const uint8_t SubDescriptorLength = pPayload[Pos + 1];
				Pos += 2;
				if (Pos + SubDescriptorLength > m_Length)
					return false;
				SubDescriptor.AdditionalInformation.SetData(&pPayload[Pos], SubDescriptorLength);
				Pos += SubDescriptorLength;
			}
		}
	}

	m_Info.ModuleList.clear();
	if (m_Info.ModuleInfoFlag) {
		const uint16_t NumOfModules = Load16(&pPayload[Pos]);

		m_Info.ModuleList.resize(NumOfModules);

		for (ModuleInfo &Info : m_Info.ModuleList) {
			if (Pos + 7 > m_Length)
				return false;
			Info.ModuleID   = Load16(&pPayload[Pos + 0]);
			Info.ModuleSize = Load32(&pPayload[Pos + 2]);
			const uint8_t ModuleInfoLength = pPayload[Pos + 6];
			Pos += 7;
			if (Pos + ModuleInfoLength > m_Length)
				return false;
			Info.ModuleInfoByte.SetData(&pPayload[Pos], ModuleInfoLength);
			Pos += ModuleInfoLength;
		}
	}

	if (Pos >= m_Length)
		return false;
	const uint8_t PrivateDataLength = pPayload[Pos];
	Pos++;
	if (Pos + PrivateDataLength > m_Length)
		return false;
	m_Info.PrivateData.SetData(&pPayload[Pos], PrivateDataLength);
	Pos += PrivateDataLength;

	m_Info.Text.clear();
	if (m_Info.TextInfoFlag) {
		if (Pos + 4 > m_Length)
			return false;
		m_Info.LanguageCode = Load24(&pPayload[Pos]);
		const uint8_t TextLength = pPayload[Pos + 3];
		if ((TextLength > 0) && (Pos + 4 + TextLength <= m_Length))
			m_Info.Text.assign(&pPayload[Pos + 4], TextLength);
	}

	return true;
}




CAEMMTSDescriptor::CAEMMTSDescriptor()
{
	Reset();
}


void CAEMMTSDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_CASystemID = 0;
	m_TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_PowerSupplyPeriod = 0;
}


bool CAEMMTSDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length != 7)
		return false;

	m_CASystemID        = Load16(&pPayload[0]);
	m_TransportStreamID = Load16(&pPayload[2]);
	m_OriginalNetworkID = Load16(&pPayload[4]);
	m_PowerSupplyPeriod = pPayload[6];

	return true;
}




CAContractInfoDescriptor::CAContractInfoDescriptor()
{
	Reset();
}


void CAContractInfoDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_CASystemID = 0x0000;
	m_CAUnitID = 0x0;
	m_NumOfComponent = 0;
	m_ContractVerificationInfoLength = 0;
	m_FeeName.clear();
}


uint8_t CAContractInfoDescriptor::GetComponentTag(uint8_t Index) const noexcept
{
	if (Index >= m_NumOfComponent)
		return COMPONENT_TAG_INVALID;
	return m_ComponentTag[Index];
}


uint8_t CAContractInfoDescriptor::GetContractVerificationInfo(uint8_t *pInfo, uint8_t MaxLength) const
{
	if ((pInfo == nullptr) || (MaxLength < m_ContractVerificationInfoLength))
		return 0;

	std::memcpy(pInfo, m_ContractVerificationInfo, m_ContractVerificationInfoLength);

	return m_ContractVerificationInfoLength;
}


bool CAContractInfoDescriptor::GetFeeName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_FeeName;

	return !Name->empty();
}


bool CAContractInfoDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 7)
		return false;

	m_CASystemID = Load16(&pPayload[0]);
	m_CAUnitID   = pPayload[2] >> 4;
	if (m_CAUnitID == 0x0)
		return false;

	// Component Tag
	m_NumOfComponent = pPayload[2] & 0x0F;
	if ((m_NumOfComponent == 0)
			|| (m_NumOfComponent > MAX_NUM_OF_COMPONENT)
			|| (m_Length < 7 + m_NumOfComponent))
		return false;
	size_t Pos = 3;
	std::memcpy(m_ComponentTag, &pPayload[Pos], m_NumOfComponent);
	Pos += m_NumOfComponent;

	// Contract Verification Info
	m_ContractVerificationInfoLength = pPayload[Pos++];
	if ((m_ContractVerificationInfoLength > MAX_VERIFICATION_INFO_LENGTH)
			|| (m_Length < Pos + m_ContractVerificationInfoLength + 1))
		return false;
	std::memcpy(m_ContractVerificationInfo, &pPayload[Pos], m_ContractVerificationInfoLength);
	Pos += m_ContractVerificationInfoLength;

	// Fee Name
	const uint8_t FeeNameLength = pPayload[Pos++];
	if (FeeNameLength > 0) {
		if (m_Length < Pos + FeeNameLength)
			return false;
		m_FeeName.assign(&pPayload[Pos], FeeNameLength);
	} else {
		m_FeeName.clear();
	}

	return true;
}




CAServiceDescriptor::CAServiceDescriptor()
{
	Reset();
}


void CAServiceDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_CASystemID = 0;
	m_CABroadcasterGroupID = 0;
	m_MessageControl = 0xFF;
	m_ServiceIDList.clear();
}


int CAServiceDescriptor::GetServiceIDCount() const
{
	return static_cast<int>(m_ServiceIDList.size());
}


uint16_t CAServiceDescriptor::GetServiceID(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_ServiceIDList.size())
		return SERVICE_ID_INVALID;
	return m_ServiceIDList[Index];
}


bool CAServiceDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 4)
		return false;

	m_CASystemID           = Load16(&pPayload[0]);
	m_CABroadcasterGroupID = pPayload[2];
	m_MessageControl       = pPayload[3];

	const int ServiceIDCount = (m_Length - 4) / 2;
	m_ServiceIDList.resize(ServiceIDCount);
	for (int i = 0; i < ServiceIDCount; i++)
		m_ServiceIDList[i] = Load16(&pPayload[4 + 2 * i]);

	return true;
}




TSInformationDescriptor::TSInformationDescriptor()
{
	Reset();
}


void TSInformationDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_RemoteControlKeyID = 0;
	m_TSName.clear();
}


bool TSInformationDescriptor::GetTSName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_TSName;

	return !Name->empty();
}


bool TSInformationDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 2)
		return false;

	m_RemoteControlKeyID = pPayload[0];

	m_TSName.clear();
	const uint8_t Length = pPayload[1] >> 2;
	if (2 + Length > m_Length)
		return false;
	if (Length > 0)
		m_TSName.assign(&pPayload[2], Length);

	return true;
}




ExtendedBroadcasterDescriptor::ExtendedBroadcasterDescriptor()
{
	Reset();
}


void ExtendedBroadcasterDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_BroadcasterType = 0;
}


bool ExtendedBroadcasterDescriptor::GetTerrestrialBroadcasterInfo(ReturnArg<TerrestrialBroadcasterInfo> Info) const
{
	if (!Info)
		return false;
	if (m_BroadcasterType != BROADCASTER_TYPE_TERRESTRIAL)
		return false;

	*Info = m_TerrestrialBroadcasterInfo;

	return true;
}


bool ExtendedBroadcasterDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_BroadcasterType = pPayload[0] >> 4;

	if (m_BroadcasterType == BROADCASTER_TYPE_TERRESTRIAL) {
		if (m_Length < 4)
			return false;

		m_TerrestrialBroadcasterInfo.TerrestrialBroadcasterID  = Load16(&pPayload[1]);
		m_TerrestrialBroadcasterInfo.NumberOfAffiliationIDLoop = pPayload[3] >> 4;
		m_TerrestrialBroadcasterInfo.NumberOfBroadcasterIDLoop = pPayload[3] & 0x0F;

		if (m_Length < 4 +
				m_TerrestrialBroadcasterInfo.NumberOfAffiliationIDLoop +
				m_TerrestrialBroadcasterInfo.NumberOfBroadcasterIDLoop * 3)
			return false;

		std::memcpy(
			m_TerrestrialBroadcasterInfo.AffiliationIDList, &pPayload[4],
			m_TerrestrialBroadcasterInfo.NumberOfAffiliationIDLoop);

		size_t Pos = 4 + m_TerrestrialBroadcasterInfo.NumberOfAffiliationIDLoop;

		for (uint8_t i = 0; i < m_TerrestrialBroadcasterInfo.NumberOfBroadcasterIDLoop; i++) {
			m_TerrestrialBroadcasterInfo.BroadcasterIDList[i].OriginalNetworkID = Load16(&pPayload[Pos + 0]);
			m_TerrestrialBroadcasterInfo.BroadcasterIDList[i].BroadcasterID     = pPayload[Pos + 2];
			Pos += 3;
		}
	}

	return true;
}



LogoTransmissionDescriptor::LogoTransmissionDescriptor()
{
	Reset();
}


void LogoTransmissionDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_LogoTransmissionType = TRANSMISSION_UNDEFINED;
	m_LogoID = LOGO_ID_INVALID;
	m_LogoVersion = LOGO_VERSION_INVALID;
	m_DownloadDataID = DATA_ID_INVALID;
	m_LogoChar[0] = '\0';
}


size_t LogoTransmissionDescriptor::GetLogoChar(char *pChar, size_t MaxLength) const
{
	if ((pChar == nullptr) || (MaxLength < 1))
		return 0;
	StringCopy(pChar, m_LogoChar, std::min(MaxLength, MAX_LOGO_CHAR));
	return StringLength(m_LogoChar, MAX_LOGO_CHAR - 1);
}


bool LogoTransmissionDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_LogoTransmissionType = pPayload[0];
	m_LogoID = LOGO_ID_INVALID;
	m_LogoVersion = LOGO_VERSION_INVALID;
	m_DownloadDataID = DATA_ID_INVALID;
	m_LogoChar[0] = '\0';
	if (m_LogoTransmissionType == TRANSMISSION_CDT1) {
		// CDT伝送方式1
		if (m_Length < 7)
			return false;
		m_LogoID         = Load16(&pPayload[1]) & 0x01FF;
		m_LogoVersion    = Load16(&pPayload[3]) & 0x0FFF;
		m_DownloadDataID = Load16(&pPayload[5]);
	} else if (m_LogoTransmissionType == TRANSMISSION_CDT2) {
		// CDT伝送方式2
		if (m_Length < 3)
			return false;
		m_LogoID         = Load16(&pPayload[1]) & 0x01FF;
	} else if (m_LogoTransmissionType == TRANSMISSION_CHAR) {
		// 簡易ロゴ方式
		size_t i;
		for (i = 0; (i < (size_t)m_Length - 1) && (i < MAX_LOGO_CHAR - 1); i++) {
			m_LogoChar[i] = pPayload[1 + i];
		}
		m_LogoChar[i] = '\0';
	}

	return true;
}




SeriesDescriptor::SeriesDescriptor()
{
	Reset();
}


void SeriesDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_SeriesID = SERIES_ID_INVALID;
	m_RepeatLabel = 0x00;
	m_ProgramPattern = PROGRAM_PATTERN_INVALID;
	m_ExpireDateValidFlag = false;
	m_EpisodeNumber = 0;
	m_LastEpisodeNumber = 0;
	m_SeriesName.clear();
}


bool SeriesDescriptor::GetExpireDate(ReturnArg<DateTime> Date) const
{
	if (!Date || !m_ExpireDateValidFlag)
		return false;
	*Date = m_ExpireDate;
	return true;
}


bool SeriesDescriptor::GetSeriesName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_SeriesName;

	return !Name->empty();
}


bool SeriesDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 8)
		return false;

	m_SeriesID            = Load16(&pPayload[0]);
	m_RepeatLabel         = pPayload[2] >> 4;
	m_ProgramPattern      = (pPayload[2] & 0x0E) >> 1;
	m_ExpireDateValidFlag = (pPayload[2] & 0x01) != 0;
	if (m_ExpireDateValidFlag)
		MJDTimeToDateTime(Load16(&pPayload[3]), &m_ExpireDate);
	m_EpisodeNumber       = (uint16_t)((pPayload[5] << 4) | (pPayload[6] >> 4));
	m_LastEpisodeNumber   = (uint16_t)(((pPayload[6] & 0x0F) << 8) | pPayload[7]);
	if (m_Length > 8)
		m_SeriesName.assign(&pPayload[8], m_Length - 8);
	else
		m_SeriesName.clear();

	return true;
}




EventGroupDescriptor::EventGroupDescriptor()
{
	Reset();
}


void EventGroupDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_GroupType = GROUP_TYPE_UNDEFINED;
	m_EventList.clear();
}


int EventGroupDescriptor::GetEventCount() const
{
	return static_cast<int>(m_EventList.size());
}


bool EventGroupDescriptor::GetEventInfo(int Index, ReturnArg<EventInfo> Info) const
{
	if ((static_cast<unsigned int>(Index) >= m_EventList.size()) || !Info)
		return false;
	*Info = m_EventList[Index];
	return true;
}


bool EventGroupDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_GroupType = pPayload[0] >> 4;
	const int EventCount = pPayload[0] & 0x0F;
	m_EventList.clear();
	if ((m_GroupType != 0x04) && (m_GroupType != 0x05)) {
		int Pos = 1;
		if (Pos + EventCount * 4 > m_Length)
			return false;
		for (int i = 0; i < EventCount; i++) {
			EventInfo Info;
			Info.ServiceID         = Load16(&pPayload[Pos + 0]);
			Info.EventID           = Load16(&pPayload[Pos + 2]);
			Info.NetworkID         = NETWORK_ID_INVALID;
			Info.TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
			m_EventList.push_back(Info);
			Pos += 4;
		}
	} else {
		if (EventCount != 0)
			return false;
		int Pos = 1;
		while (Pos + 8 <= m_Length) {
			EventInfo Info;
			Info.NetworkID         = Load16(&pPayload[Pos + 0]);
			Info.TransportStreamID = Load16(&pPayload[Pos + 2]);
			Info.ServiceID         = Load16(&pPayload[Pos + 4]);
			Info.EventID           = Load16(&pPayload[Pos + 6]);
			m_EventList.push_back(Info);
			Pos += 8;
		}
	}

	return true;
}




SIParameterDescriptor::SIParameterDescriptor()
{
	Reset();
}


void SIParameterDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_ParameterVersion = 0xFF;
	m_UpdateTime.Reset();
	m_TableList.clear();
}


bool SIParameterDescriptor::GetUpdateTime(ReturnArg<DateTime> Time) const
{
	if (!Time)
		return false;

	*Time = m_UpdateTime;

	return Time->IsValid();
}


int SIParameterDescriptor::GetTableCount() const
{
	return static_cast<int>(m_TableList.size());
}


bool SIParameterDescriptor::GetTableInfo(int Index, ReturnArg<TableInfo> Info) const
{
	if (!Info)
		return false;
	if (static_cast<unsigned int>(Index) >= m_TableList.size())
		return false;

	*Info = m_TableList[Index];

	return true;
}


bool SIParameterDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 3)
		return false;

	m_ParameterVersion = pPayload[0];
	MJDTimeToDateTime(Load16(&pPayload[1]), &m_UpdateTime);

	m_TableList.clear();

	for (size_t Pos = 3; Pos + 3 <= m_Length;) {
		TableInfo Info;

		Info.TableID = pPayload[Pos + 0];

		const uint8_t DescriptionLength = pPayload[Pos + 1];
		Pos += 2;
		if (Pos + DescriptionLength > m_Length)
			break;

		bool OK = false;

		switch (Info.TableID) {
		case TABLE_ID_NIT:
		case TABLE_ID_SDT_ACTUAL:
		case TABLE_ID_SDT_OTHER:
		case TABLE_ID_BIT:
		case TABLE_ID_NBIT_MSG:
		case TABLE_ID_NBIT_REF:
			if (DescriptionLength == 1) {
				Info.NIT.TableCycle = pPayload[Pos];
				OK = true;
			}
			break;

		case TABLE_ID_SDTT:
		case TABLE_ID_LDT:
		case TABLE_ID_CDT:
			if (DescriptionLength == 2) {
				Info.LDT.TableCycle = Load16(&pPayload[Pos]);
				OK = true;
			}
			break;

		case TABLE_ID_EIT_PF_ACTUAL:
			if (DescriptionLength == 4) {
				// Terrestrial (H-EIT[p/f], M-EIT, L-EIT)
				Info.HMLEIT.HEITTableCycle = pPayload[Pos + 0];
				Info.HMLEIT.MEITTableCycle = pPayload[Pos + 1];
				Info.HMLEIT.LEITTableCycle = pPayload[Pos + 2];
				Info.HMLEIT.NumOfMEITEvent = pPayload[Pos + 3] >> 4;
				Info.HMLEIT.NumOfLEITEvent = pPayload[Pos + 3] & 0x0F;
				OK = true;
				break;
			}
			[[fallthrough]];
		case TABLE_ID_EIT_PF_OTHER:
			if (DescriptionLength == 1) {
				Info.EIT_PF.TableCycle = pPayload[Pos];
				OK = true;
				break;
			}
			break;

		case TABLE_ID_EIT_SCHEDULE_ACTUAL:
		case TABLE_ID_EIT_SCHEDULE_EXTENDED:
		case TABLE_ID_EIT_SCHEDULE_OTHER:
			Info.HEIT_Schedule.MediaTypeCount = 0;

			if (DescriptionLength >= 4) {
				const size_t EndPos = Pos + DescriptionLength;

				for (int i = 0; Pos + 4 <= EndPos; i++) {
					Info.HEIT_Schedule.MediaTypeList[i].MediaType       = pPayload[Pos + 0] >> 6;
					Info.HEIT_Schedule.MediaTypeList[i].Pattern         = (pPayload[Pos + 0] >> 4) & 0x03;
					Info.HEIT_Schedule.MediaTypeList[i].EITOtherFlag    = (pPayload[Pos + 0] & 0x08) != 0;
					Info.HEIT_Schedule.MediaTypeList[i].ScheduleRange   = GetBCD(pPayload[Pos + 1]);
					Info.HEIT_Schedule.MediaTypeList[i].BaseCycle       = GetBCD(&pPayload[Pos + 2], 3);
					Info.HEIT_Schedule.MediaTypeList[i].CycleGroupCount = pPayload[Pos + 3] & 0x03;

					Pos += 4;
					if (Pos + Info.HEIT_Schedule.MediaTypeList[i].CycleGroupCount * 2 > EndPos)
						break;

					for (int j = 0; j < Info.HEIT_Schedule.MediaTypeList[i].CycleGroupCount; j++) {
						Info.HEIT_Schedule.MediaTypeList[i].CycleGroup[j].NumOfSegment = GetBCD(pPayload[Pos + 0]);
						Info.HEIT_Schedule.MediaTypeList[i].CycleGroup[j].Cycle        = GetBCD(pPayload[Pos + 1]);
						Pos += 2;
					}

					Info.HEIT_Schedule.MediaTypeCount++;
				}

				OK = true;
			}
			break;
		}

		if (OK)
			m_TableList.push_back(Info);

		Pos += DescriptionLength;
	}

	return true;
}




void BroadcasterNameDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_BroadcasterName.clear();
}


bool BroadcasterNameDescriptor::GetBroadcasterName(ReturnArg<ARIBString> Name) const
{
	if (!Name)
		return false;

	*Name = m_BroadcasterName;

	return !Name->empty();
}


bool BroadcasterNameDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	if (m_Length > 0)
		m_BroadcasterName.assign(&pPayload[0], m_Length);
	else
		m_BroadcasterName.clear();

	return true;
}




ComponentGroupDescriptor::ComponentGroupDescriptor()
{
	Reset();
}


void ComponentGroupDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_ComponentGroupType = 0;
	m_TotalBitRateFlag = false;
	m_GroupList.clear();
}


uint8_t ComponentGroupDescriptor::GetGroupCount() const
{
	return static_cast<uint8_t>(m_GroupList.size());
}


const ComponentGroupDescriptor::GroupInfo * ComponentGroupDescriptor::GetGroupInfo(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_GroupList.size())
		return nullptr;

	return &m_GroupList[Index];
}


bool ComponentGroupDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_ComponentGroupType = pPayload[0] >> 5;
	m_TotalBitRateFlag   = (pPayload[0] & 0x10) != 0;

	const int NumOfGroup = pPayload[0] & 0x0F;

	m_GroupList.clear();
	m_GroupList.reserve(NumOfGroup);

	int Pos = 1;

	for (int i = 0; (i < NumOfGroup) && (Pos + 2 <= m_Length); i++) {
		GroupInfo Group;

		Group.ComponentGroupID = pPayload[Pos] >> 4;
		Group.NumOfCAUnit      = pPayload[Pos] & 0x0F;
		Pos++;

		for (int j = 0; j < Group.NumOfCAUnit; j++) {
			CAUnitInfo &CAUnit = Group.CAUnitList[j];

			if (Pos >= m_Length)
				return false;
			CAUnit.CAUnitID       = pPayload[Pos] >> 4;
			CAUnit.NumOfComponent = pPayload[Pos] & 0x0F;
			Pos++;
			if (Pos + CAUnit.NumOfComponent > m_Length)
				return false;
			std::memcpy(CAUnit.ComponentTag, &pPayload[Pos], CAUnit.NumOfComponent);
			Pos += CAUnit.NumOfComponent;
		}

		if (m_TotalBitRateFlag) {
			if (Pos >= m_Length)
				return false;
			Group.TotalBitRate = pPayload[Pos++];
		} else {
			Group.TotalBitRate = 0;
		}

		if (Pos >= m_Length)
			return false;
		const uint8_t TextLength = pPayload[Pos++];
		if (TextLength > 0) {
			if (Pos + TextLength > m_Length)
				return false;
			Group.Text.assign(&pPayload[Pos], TextLength);
			Pos += TextLength;
		}

		m_GroupList.push_back(std::move(Group));
	}

	return true;
}




LDTLinkageDescriptor::LDTLinkageDescriptor()
{
	Reset();
}


void LDTLinkageDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_OriginalServiceID = SERVICE_ID_INVALID;
	m_TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_OriginalNetworkID = NETWORK_ID_INVALID;
	m_DescriptionList.clear();
}


int LDTLinkageDescriptor::GetDescriptionInfoCount() const
{
	return static_cast<int>(m_DescriptionList.size());
}


bool LDTLinkageDescriptor::GetDescriptionInfo(int Index, ReturnArg<DescriptionInfo> Info) const
{
	if (!Info)
		return false;
	if (static_cast<unsigned int>(Index) >= m_DescriptionList.size())
		return false;

	*Info = m_DescriptionList[Index];

	return true;
}


bool LDTLinkageDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 6)
		return false;

	m_OriginalServiceID = Load16(&pPayload[0]);
	m_TransportStreamID = Load16(&pPayload[2]);
	m_OriginalNetworkID = Load16(&pPayload[4]);

	m_DescriptionList.resize((m_Length - 6) / 4);

	size_t Pos = 6;
	for (DescriptionInfo &Info : m_DescriptionList) {
		Info.DescriptionID   = Load16(&pPayload[Pos + 0]);
		Info.DescriptionType = pPayload[Pos + 2] & 0x0F;
		Pos += 4;
	}

	return true;
}




AccessControlDescriptor::AccessControlDescriptor()
{
	Reset();
}


void AccessControlDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_CASystemID = 0;
	m_PID = PID_INVALID;
	m_TransmissionType = 0;
	m_PrivateData.ClearSize();
}


bool AccessControlDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 4)
		return false;

	m_CASystemID       = Load16(&pPayload[0]);
	m_TransmissionType = pPayload[2] >> 5;
	m_PID              = Load16(&pPayload[2]) & 0x1FFF_u16;
	m_PrivateData.SetData(&pPayload[4], m_Length - 4);

	return true;
}




TerrestrialDeliverySystemDescriptor::TerrestrialDeliverySystemDescriptor()
{
	Reset();
}


void TerrestrialDeliverySystemDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_AreaCode = 0;
	m_GuardInterval = 0xFF;
	m_TransmissionMode = 0xFF;
	m_Frequency.clear();
}


int TerrestrialDeliverySystemDescriptor::GetFrequencyCount() const
{
	return static_cast<int>(m_Frequency.size());
}


uint16_t TerrestrialDeliverySystemDescriptor::GetFrequency(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_Frequency.size())
		return 0;
	return m_Frequency[Index];
}


bool TerrestrialDeliverySystemDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 4)
		return false;

	m_AreaCode         = (uint16_t)((pPayload[0] << 4) | (pPayload[1] >> 4));
	m_GuardInterval    = (pPayload[1] & 0x0C) >> 2;
	m_TransmissionMode = pPayload[1] & 0x03;
	const int FrequencyCount = (m_Length - 2) / 2;
	m_Frequency.resize(FrequencyCount);
	int Pos = 2;
	for (int i = 0; i < FrequencyCount; i++) {
		m_Frequency[i] = Load16(&pPayload[Pos + 0]);
		Pos += 2;
	}

	return true;
}




PartialReceptionDescriptor::PartialReceptionDescriptor()
{
	Reset();
}


void PartialReceptionDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_ServiceCount = 0;
}


int PartialReceptionDescriptor::GetServiceCount() const
{
	return m_ServiceCount;
}


uint16_t PartialReceptionDescriptor::GetServiceID(int Index) const
{
	if ((unsigned int)Index >= m_ServiceCount)
		return SERVICE_ID_INVALID;
	return m_ServiceList[Index];
}


bool PartialReceptionDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	uint8_t ServiceCount = m_Length / 2;
	if (ServiceCount > 3)
		ServiceCount = 3;

	m_ServiceCount = ServiceCount;

	for (uint8_t i = 0; i < ServiceCount; i++)
		m_ServiceList[i] = Load16(&pPayload[i * 2]);

	return true;
}




void EmergencyInformationDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_ServiceList.clear();
}


int EmergencyInformationDescriptor::GetServiceCount() const
{
	return static_cast<int>(m_ServiceList.size());
}


bool EmergencyInformationDescriptor::GetServiceInfo(int Index, ServiceInfo *pInfo) const
{
	if (pInfo == nullptr)
		return false;
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	*pInfo = m_ServiceList[Index];

	return true;
}


bool EmergencyInformationDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;

	m_ServiceList.clear();

	size_t Pos = 0;

	while (Pos + 4 <= m_Length) {
		m_ServiceList.emplace_back();
		ServiceInfo &Info = m_ServiceList.back();

		Info.ServiceID    = Load16(&pPayload[Pos + 0]);
		Info.StartEndFlag = (pPayload[Pos + 2] & 0x80) != 0;
		Info.SignalLevel  = (pPayload[Pos + 2] & 0x40) != 0;

		const uint8_t AreaCodeLength = pPayload[Pos + 3];
		Pos += 4;
		if ((AreaCodeLength % 2 != 0) || (Pos + AreaCodeLength > m_Length)) {
			m_ServiceList.pop_back();
			break;
		}

		Info.AreaCodeList.resize(AreaCodeLength / 2);
		for (uint16_t &AreaCode : Info.AreaCodeList) {
			AreaCode = Load16(&pPayload[Pos]) >> 4;
			Pos += 2;
		}
	}

	return true;
}




DataComponentDescriptor::DataComponentDescriptor()
{
	Reset();
}


void DataComponentDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_DataComponentID = 0;
	m_AdditionalDataComponentInfo.ClearSize();
}


bool DataComponentDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length < 1)
		return false;

	m_DataComponentID = pPayload[0];
	m_AdditionalDataComponentInfo.SetData(&pPayload[1], m_Length - 1);

	return true;
}




SystemManagementDescriptor::SystemManagementDescriptor()
{
	Reset();
}


void SystemManagementDescriptor::Reset()
{
	DescriptorBase::Reset();

	m_BroadcastingFlag = 0;
	m_BroadcastingID = 0;
	m_AdditionalBroadcastingID = 0;
}


bool SystemManagementDescriptor::StoreContents(const uint8_t *pPayload)
{
	if (m_Tag != TAG)
		return false;
	if (m_Length != 2)
		return false;

	m_BroadcastingFlag         = (pPayload[0] & 0xC0) >> 6;
	m_BroadcastingID           = (pPayload[0] & 0x3F);
	m_AdditionalBroadcastingID = pPayload[1];

	return true;
}


}	// namespace LibISDB
