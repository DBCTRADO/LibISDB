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
 @file   BitRateCalculator.cpp
 @brief  ビットレート計算クラス
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "BitRateCalculator.hpp"


namespace LibISDB
{


BitRateCalculator::BitRateCalculator()
	: m_UpdateInterval(TickClock::ClocksPerSec)
{
	Reset();
}


void BitRateCalculator::Initialize()
{
	m_LastClock = m_Clock.Get();
	m_Bytes = 0;
	m_BitRate = 0;
}


void BitRateCalculator::Reset()
{
	m_LastClock = 0;
	m_Bytes = 0;
	m_BitRate = 0;
}


bool BitRateCalculator::Update(size_t Bytes)
{
	const TickClock::ClockType Now = m_Clock.Get();
	bool Updated = false;

	if (Now >= m_LastClock) {
		m_Bytes += Bytes;
		if (Now - m_LastClock >= m_UpdateInterval) {
			m_BitRate = static_cast<unsigned long>(
				(static_cast<unsigned long long>(m_Bytes) * (8 * TickClock::ClocksPerSec)) /
				 static_cast<unsigned long long>(Now - m_LastClock));
			m_LastClock = Now;
			m_Bytes = 0;
			Updated = true;
		}
	} else {
		m_LastClock = Now;
		m_Bytes = 0;
	}

	return Updated;
}


unsigned long BitRateCalculator::GetBitRate() const
{
	if (m_Clock.Get() - m_LastClock >= 2 * m_UpdateInterval)
		return 0;
	return m_BitRate;
}


bool BitRateCalculator::SetUpdateInterval(TickClock::ClockType Interval)
{
	if (Interval < 1)
		return false;

	m_UpdateInterval = Interval;

	return true;
}


}	// namespace LibISDB
