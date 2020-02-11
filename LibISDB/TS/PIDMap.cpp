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
 @file   PIDMap.cpp
 @brief  PID マップ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "PIDMap.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


PIDMapManager::PIDMapManager()
	: m_MapCount(0)
{
	m_PIDMap.fill(nullptr);
}


PIDMapManager::~PIDMapManager()
{
	UnmapAllTargets();
}


bool PIDMapManager::StorePacket(const TSPacket *pPacket)
{
	const uint16_t PID = pPacket->GetPID();

	if (PID > PID_MAX)
		return false;

	PIDMapTarget *pTarget = m_PIDMap[PID];

	if (pTarget == nullptr)
		return false;

	return pTarget->StorePacket(pPacket);
}


bool PIDMapManager::StorePacketStream(DataStream *pPacketStream)
{
	const uint16_t PID = pPacketStream->Get<TSPacket>()->GetPID();

	if ((PID > PID_MAX) || (m_PIDMap[PID] == nullptr))
		return false;

	do {
		const TSPacket *pPacket = pPacketStream->Get<TSPacket>();

		LIBISDB_ASSERT(pPacket->GetPID() == PID);

		PIDMapTarget *pTarget = m_PIDMap[PID];

		if (pTarget == nullptr)
			break;

		pTarget->StorePacket(pPacket);
	} while (pPacketStream->Next());

	return true;
}


bool PIDMapManager::MapTarget(uint16_t PID, PIDMapTarget *pMapTarget)
{
	if ((PID > PID_MAX) || (pMapTarget == nullptr))
		return false;

	UnmapTarget(PID);

	m_PIDMap[PID] = pMapTarget;
	m_MapCount++;

	pMapTarget->OnPIDMapped(PID);

	return true;
}


bool PIDMapManager::UnmapTarget(uint16_t PID)
{
	if (PID > PID_MAX)
		return false;

	PIDMapTarget *pTarget = m_PIDMap[PID];

	if (pTarget == nullptr)
		return false;

	m_PIDMap[PID] = nullptr;
	m_MapCount--;

	pTarget->OnPIDUnmapped(PID);

	return true;
}


void PIDMapManager::UnmapAllTargets()
{
	for (uint16_t PID = 0x0000_u16; PID <= PID_MAX; PID++) {
		UnmapTarget(PID);
	}
}


PIDMapTarget * PIDMapManager::GetMapTarget(uint16_t PID) const
{
	if (PID > PID_MAX)
		return nullptr;

	return m_PIDMap[PID];
}


uint16_t PIDMapManager::GetMapCount() const
{
	return m_MapCount;
}


}	// namespace LibISDB
