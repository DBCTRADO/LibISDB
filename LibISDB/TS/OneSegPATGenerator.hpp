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
 @file   OneSegPATGenerator.hpp
 @brief  ワンセグ PAT 生成
 @author DBCTRADO
*/


#ifndef LIBISDB_ONESEG_PAT_GENERATOR_H
#define LIBISDB_ONESEG_PAT_GENERATOR_H


#include "PIDMap.hpp"
#include "PSITable.hpp"


namespace LibISDB
{

	/** ワンセグ PAT 生成クラス */
	class OneSegPATGenerator
	{
	public:
		OneSegPATGenerator();

		void Reset();
		bool StorePacket(const TSPacket *pPacket);
		bool GetPATPacket(TSPacket *pPacket);
		bool SetTransportStreamID(uint16_t TransportStreamID);

	protected:
		void OnNITSection(const PSITableBase *pTable, const PSISection *pSection);

		uint16_t m_TransportStreamID;
		bool m_HasPAT;
		bool m_GeneratePAT;
		PIDMapManager m_PIDMapManager;
		uint8_t m_ContinuityCounter;
		uint8_t m_PMTCount[ONESEG_PMT_PID_COUNT];
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ONESEG_PAT_GENERATOR_H
