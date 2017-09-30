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
 @file   DescriptorBlock.cpp
 @brief  記述子ブロック
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "DescriptorBlock.hpp"
#include "Descriptors.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


DescriptorBlock::DescriptorBlock(const DescriptorBlock &Src)
{
	*this = Src;
}


DescriptorBlock::DescriptorBlock(DescriptorBlock &&Src)
{
	*this = std::move(Src);
}


DescriptorBlock & DescriptorBlock::operator = (const DescriptorBlock &Src)
{
	if (&Src != this) {
		Reset();
		m_DescriptorList.reserve(Src.m_DescriptorList.size());

		for (auto &e : Src.m_DescriptorList) {
			m_DescriptorList.push_back(std::unique_ptr<DescriptorBase>(e->Clone()));
		}
	}

	return *this;
}


DescriptorBlock & DescriptorBlock::operator = (DescriptorBlock &&Src)
{
	if (&Src != this) {
		Reset();
		m_DescriptorList.swap(Src.m_DescriptorList);
	}

	return *this;
}


int DescriptorBlock::ParseBlock(const uint8_t *pData, size_t DataLength)
{
	Reset();

	if ((pData == nullptr) || (DataLength < 2) || (DataLength > 0xFFFF))
		return 0;

	size_t Pos = 0;

	do {
		std::unique_ptr<DescriptorBase> Descriptor(ParseDescriptor(&pData[Pos], static_cast<uint16_t>(DataLength - Pos)));

		if (!Descriptor)
			break;

		Pos += Descriptor->GetLength() + 2;

		m_DescriptorList.push_back(std::move(Descriptor));
	} while (Pos + 2 <= DataLength);

	return static_cast<int>(m_DescriptorList.size());
}


const DescriptorBase * DescriptorBlock::ParseBlock(const uint8_t *pData, size_t DataLength, uint8_t Tag)
{
	if (ParseBlock(pData, DataLength) == 0)
		return nullptr;

	return GetDescriptorByTag(Tag);
}


void DescriptorBlock::Reset()
{
	m_DescriptorList.clear();
}


int DescriptorBlock::GetDescriptorCount() const
{
	return static_cast<int>(m_DescriptorList.size());
}


const DescriptorBase * DescriptorBlock::GetDescriptorByIndex(int Index) const
{
	if (static_cast<unsigned int>(Index) >= m_DescriptorList.size())
		return nullptr;
	return m_DescriptorList[Index].get();
}


const DescriptorBase * DescriptorBlock::GetDescriptorByTag(uint8_t Tag) const
{
	for (auto &e : m_DescriptorList){
		if (e->GetTag() == Tag)
			return e.get();
	}

	return nullptr;
}


std::unique_ptr<DescriptorBase> DescriptorBlock::ParseDescriptor(const uint8_t *pData, uint16_t DataLength)
{
	if ((pData == nullptr) || (DataLength < 2))
		return nullptr;

	std::unique_ptr<DescriptorBase> Descriptor(CreateDescriptorInstance(pData[0]));

	if (!Descriptor->Parse(pData, DataLength))
		return nullptr;

	return Descriptor;
}


DescriptorBase * DescriptorBlock::CreateDescriptorInstance(uint8_t Tag)
{
	switch (Tag) {
	case CADescriptor::TAG                        : return new CADescriptor;
	case NetworkNameDescriptor::TAG               : return new NetworkNameDescriptor;
	case ServiceListDescriptor::TAG               : return new ServiceListDescriptor;
	case SatelliteDeliverySystemDescriptor::TAG   : return new SatelliteDeliverySystemDescriptor;
	case ServiceDescriptor::TAG                   : return new ServiceDescriptor;
	case LinkageDescriptor::TAG                   : return new LinkageDescriptor;
	case ShortEventDescriptor::TAG                : return new ShortEventDescriptor;
	case ExtendedEventDescriptor::TAG             : return new ExtendedEventDescriptor;
	case ComponentDescriptor::TAG                 : return new ComponentDescriptor;
	case StreamIDDescriptor::TAG                  : return new StreamIDDescriptor;
	case ContentDescriptor::TAG                   : return new ContentDescriptor;
	case LocalTimeOffsetDescriptor::TAG           : return new LocalTimeOffsetDescriptor;
	case HierarchicalTransmissionDescriptor::TAG  : return new HierarchicalTransmissionDescriptor;
	case DigitalCopyControlDescriptor::TAG        : return new DigitalCopyControlDescriptor;
	case AudioComponentDescriptor::TAG            : return new AudioComponentDescriptor;
	case HyperLinkDescriptor::TAG                 : return new HyperLinkDescriptor;
	case TargetRegionDescriptor::TAG              : return new TargetRegionDescriptor;
	case VideoDecodeControlDescriptor::TAG        : return new VideoDecodeControlDescriptor;
	case DownloadContentDescriptor::TAG           : return new DownloadContentDescriptor;
	case CAEMMTSDescriptor::TAG                   : return new CAEMMTSDescriptor;
	case CAContractInfoDescriptor::TAG            : return new CAContractInfoDescriptor;
	case CAServiceDescriptor::TAG                 : return new CAServiceDescriptor;
	case TSInformationDescriptor::TAG             : return new TSInformationDescriptor;
	case ExtendedBroadcasterDescriptor::TAG       : return new ExtendedBroadcasterDescriptor;
	case LogoTransmissionDescriptor::TAG          : return new LogoTransmissionDescriptor;
	case SeriesDescriptor::TAG                    : return new SeriesDescriptor;
	case EventGroupDescriptor::TAG                : return new EventGroupDescriptor;
	case SIParameterDescriptor::TAG               : return new SIParameterDescriptor;
	case BroadcasterNameDescriptor::TAG           : return new BroadcasterNameDescriptor;
	case ComponentGroupDescriptor::TAG            : return new ComponentGroupDescriptor;
	case LDTLinkageDescriptor::TAG                : return new LDTLinkageDescriptor;
	case AccessControlDescriptor::TAG             : return new AccessControlDescriptor;
	case TerrestrialDeliverySystemDescriptor::TAG : return new TerrestrialDeliverySystemDescriptor;
	case PartialReceptionDescriptor::TAG          : return new PartialReceptionDescriptor;
	case EmergencyInformationDescriptor::TAG      : return new EmergencyInformationDescriptor;
	case DataComponentDescriptor::TAG             : return new DataComponentDescriptor;
	case SystemManagementDescriptor::TAG          : return new SystemManagementDescriptor;
	}

	return new DescriptorBase;
}


}	// namespace LibISDB
