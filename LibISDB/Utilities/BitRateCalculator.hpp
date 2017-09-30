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
 @file   BitRateCalculator.hpp
 @brief  ビットレート計算
 @author DBCTRADO
*/


#ifndef LIBISDB_BIT_RATE_CALCULATOR_H
#define LIBISDB_BIT_RATE_CALCULATOR_H


#include "Clock.hpp"


namespace LibISDB
{

	/** ビットレート計算クラス */
	class BitRateCalculator
	{
	public:
		BitRateCalculator();
		void Initialize();
		void Reset();
		bool Update(size_t Bytes);
		unsigned long GetBitRate() const;
		bool SetUpdateInterval(TickClock::ClockType Interval);
		TickClock::ClockType GetUpdateInterval() const noexcept { return m_UpdateInterval; }

	private:
		TickClock m_Clock;
		TickClock::ClockType m_LastClock;
		TickClock::ClockType m_UpdateInterval;
		size_t m_Bytes;
		unsigned long m_BitRate;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_BIT_RATE_CALCULATOR_H
