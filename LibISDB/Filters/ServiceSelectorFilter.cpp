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
 @file   ServiceSelectorFilter.cpp
 @brief  サービス選択フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "ServiceSelectorFilter.hpp"
#include "../TS/Tables.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/CRC.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


ServiceSelectorFilter::ServiceSelectorFilter()
	: m_TargetServiceID(SERVICE_ID_INVALID)
	, m_TargetStream(StreamSelector::StreamFlag::All)
	, m_FollowActiveService(false)
{
}


void ServiceSelectorFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_StreamSelector.Reset();
}


void ServiceSelectorFilter::SetActiveServiceID(uint16_t ServiceID)
{
	BlockLock Lock(m_FilterLock);

	if (m_FollowActiveService)
		SetTargetServiceID(ServiceID, m_TargetStream);
}


bool ServiceSelectorFilter::ReceiveData(DataStream *pData)
{
	BlockLock Lock(m_FilterLock);

	if (pData->Is<TSPacket>()) {
		do {
			TSPacket *pSrcPacket = pData->GetData()->Cast<TSPacket>();
			TSPacket *pDstPacket = m_StreamSelector.InputPacket(pSrcPacket);
			if (pDstPacket != nullptr)
				m_PacketSequence.AddData(*pDstPacket);
		} while (pData->Next());
	}

	if (m_PacketSequence.GetDataCount() > 0) {
		OutputData(m_PacketSequence);
		m_PacketSequence.SetDataCount(0);
	}

	return true;
}


bool ServiceSelectorFilter::SetTargetServiceID(uint16_t ServiceID, StreamSelector::StreamFlag Stream)
{
	BlockLock Lock(m_FilterLock);

	if ((m_TargetServiceID != ServiceID) || (m_TargetStream != Stream)) {
		m_TargetServiceID = ServiceID;
		m_TargetStream = Stream;

		if (Stream == StreamSelector::StreamFlag::All) {
			m_StreamSelector.SetTarget(ServiceID);
		} else {
			StreamSelector::StreamTypeTable StreamTable(Stream);

			m_StreamSelector.SetTarget(ServiceID, &StreamTable);
		}
	}

	return true;
}


void ServiceSelectorFilter::SetFollowActiveService(bool Follow)
{
	BlockLock Lock(m_FilterLock);

	m_FollowActiveService = Follow;
}


}	// namespace LibISDB
