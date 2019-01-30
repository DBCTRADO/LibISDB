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
 @file   PIDMap.hpp
 @brief  PID マップ
 @author DBCTRADO
*/


#ifndef LIBISDB_PID_MAP_H
#define LIBISDB_PID_MAP_H


#include "TSPacket.hpp"
#include "../Base/DataStream.hpp"
#include <array>


namespace LibISDB
{

	/** PID マップ対象クラス */
	class PIDMapTarget
	{
	public:
		virtual ~PIDMapTarget() = default;

		virtual bool StorePacket(const TSPacket *pPacket) = 0;
		virtual void OnPIDMapped(uint16_t PID) {}
		virtual void OnPIDUnmapped(uint16_t PID) {}
	};

	/** PID マップ管理クラス */
	class PIDMapManager
	{
	public:
		PIDMapManager();
		~PIDMapManager();

		bool StorePacket(const TSPacket *pPacket);
		bool StorePacketStream(DataStream *pPacketStream);

		bool MapTarget(uint16_t PID, PIDMapTarget *pMapTarget);
		bool UnmapTarget(uint16_t PID);
		void UnmapAllTargets();

		PIDMapTarget * GetMapTarget(uint16_t PID) const;
		template<typename T> T * GetMapTarget(uint16_t PID) const { return dynamic_cast<T *>(GetMapTarget(PID)); }
		uint16_t GetMapCount() const;

	protected:
		std::array<PIDMapTarget *, PID_MAX + 1> m_PIDMap;
		uint16_t m_MapCount;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_PID_MAP_H
