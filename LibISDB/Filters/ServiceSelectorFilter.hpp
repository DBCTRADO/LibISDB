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
 @file   ServiceSelectorFilter.hpp
 @brief  サービス選択フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_SERVICE_SELECTOR_FILTER_H
#define LIBISDB_SERVICE_SELECTOR_FILTER_H


#include "FilterBase.hpp"
#include "../TS/StreamSelector.hpp"


namespace LibISDB
{

	/** サービス選択フィルタクラス */
	class ServiceSelectorFilter
		: public SingleIOFilter
	{
	public:
		ServiceSelectorFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("ServiceSelectorFilter"); }

	// FilterBase
		void Reset() override;
		void SetActiveServiceID(uint16_t ServiceID) override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	// ServiceSelectorFilter
		bool SetTargetServiceID(
			uint16_t ServiceID = SERVICE_ID_INVALID,
			StreamSelector::StreamFlag Stream = StreamSelector::StreamFlag::All);
		uint16_t GetTargetServiceID() const noexcept { return m_TargetServiceID; }
		StreamSelector::StreamFlag GetTargetStream() const noexcept { return m_TargetStream; }
		void SetFollowActiveService(bool Follow);
		bool GetFollowActiveService() const noexcept { return m_FollowActiveService; }

	protected:
		uint16_t m_TargetServiceID;
		StreamSelector::StreamFlag m_TargetStream;
		bool m_FollowActiveService;

		StreamSelector m_StreamSelector;
		DataStreamSequence<TSPacket> m_PacketSequence;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_SERVICE_SELECTOR_FILTER_H
