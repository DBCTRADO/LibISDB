/*
  LibISDB
  Copyright(c) 2017-2018 DBCTRADO

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
 @file   TSPacketCounterFilter.hpp
 @brief  TSパケットカウンタフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_PACKET_COUNTER_FILTER_H
#define LIBISDB_TS_PACKET_COUNTER_FILTER_H


#include "FilterBase.hpp"
#include "../TS/PIDMap.hpp"
#include "../TS/PSITable.hpp"
#include "../Utilities/BitRateCalculator.hpp"
#include <atomic>
#include <vector>


namespace LibISDB
{

	/** TSパケットカウンタフィルタクラス */
	class TSPacketCounterFilter
		: public SingleIOFilter
	{
	public:
		TSPacketCounterFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("TSPacketCounterFilter"); }

	// FilterBase
		void Reset() override;
		void SetActiveServiceID(uint16_t ServiceID) override;
		void SetActiveVideoPID(uint16_t PID, bool ServiceChanged) override;
		void SetActiveAudioPID(uint16_t PID, bool ServiceChanged) override;

	// SingleIOFilter
		bool ProcessData(DataStream *pData) override;

	// TSPacketCounterFilter
		unsigned long long GetInputPacketCount() const;
		void ResetInputPacketCount();
		unsigned long long GetScrambledPacketCount() const;
		void ResetScrambledPacketCount();
		void SetVideoPID(uint16_t PID);
		void SetAudioPID(uint16_t PID);
		unsigned long GetVideoBitRate() const;
		unsigned long GetAudioBitRate() const;

	protected:
		int GetServiceIndexByID(uint16_t ServiceID) const;
		void MapServiceESs(size_t Index);
		void UnmapServiceESs(size_t Index);
		void OnPATSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnPMTSection(const PSITableBase *pTable, const PSISection *pSection);

		struct ServiceInfo {
			uint16_t ServiceID;
			uint16_t PMTPID;
			std::vector<uint16_t> ESPIDList;
		};

		class ESPIDMapTarget
			: public PIDMapTarget
		{
		public:
			ESPIDMapTarget(TSPacketCounterFilter *pTSPacketCounter);
			bool StorePacket(const TSPacket *pPacket) override;

		private:
			TSPacketCounterFilter *m_pTSPacketCounter;
		};

		PIDMapManager m_PIDMapManager;
		ESPIDMapTarget m_ESPIDMapTarget;

		std::vector<ServiceInfo> m_ServiceList;
		uint16_t m_TargetServiceID;

		std::atomic<unsigned long long> m_InputPacketCount;
		std::atomic<unsigned long long> m_ScrambledPacketCount;

		uint16_t m_VideoPID;
		uint16_t m_AudioPID;
		BitRateCalculator m_VideoBitRate;
		BitRateCalculator m_AudioBitRate;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TS_PACKET_COUNTER_FILTER_H
