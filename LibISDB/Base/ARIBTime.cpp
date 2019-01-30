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
 @file   ARIBTime.cpp
 @brief  MJD/BCD形式の日時の変換を行う
 @author DBCTRADO
*/


#include "../LibISDBBase.hpp"
#include "ARIBTime.hpp"
#include "../Utilities/Utilities.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


// MJD+BCD の日時を DateTime に変換する
bool MJDBCDTimeToDateTime(const uint8_t *pData, ReturnArg<DateTime> Time) noexcept
{
	if (!Time)
		return false;

	Time->Reset();

	if (pData == nullptr)
		return false;

	// 全ビットが1の場合は未定義
	if ((pData[0] == 0xFF) && (pData[1] == 0xFF) && (pData[2] == 0xFF) && (pData[3] == 0xFF) && (pData[4] == 0xFF))
		return false;

	// MJD の日付を解析
	ParseMJDTime(Load16(pData), &Time->Year, &Time->Month, &Time->Day, &Time->DayOfWeek);

	// BCD の時刻を解析
	ParseBCDTime(&pData[2], &Time->Hour, &Time->Minute, &Time->Second);

	return true;
}


// MJD の日付を年/月/日/曜日に変換する
void ParseMJDTime(
	uint16_t MJD,
	ReturnArg<int> Year, ReturnArg<int> Month, ReturnArg<int> Day,
	OptionalReturnArg<int> DayOfWeek) noexcept
{
	const int Yd = static_cast<int>((static_cast<double>(MJD) - 15078.2) / 365.25);
	const int Md = static_cast<int>((static_cast<double>(MJD) - 14956.1 - static_cast<int>(static_cast<double>(Yd) * 365.25)) / 30.6001);
	const int K = ((Md == 14) || (Md == 15)) ? 1 : 0;

	Day = MJD - 14956 -
		static_cast<int>(static_cast<double>(Yd) * 365.25) -
		static_cast<int>(static_cast<double>(Md) * 30.6001);
	Year = Yd + K + 1900;
	Month = Md - 1 - K * 12;
	if (DayOfWeek)
		DayOfWeek = (MJD + 3) % 7;
}


// 年/月/日を MJD に変換する
uint16_t MakeMJDTime(int Year, int Month, int Day) noexcept
{
	int Y = Year, M = Month;
	if (M <= 2) {
		M += 12;
		Y--;
	}
	return static_cast<uint16_t>(
		static_cast<int>(static_cast<double>(Y) * 365.25) + (Y / 400) - (Y / 100) +
		static_cast<int>(static_cast<double>(M - 2) * 30.59) + Day - 678912);
}


// MJD を DateTime に変換する
void MJDTimeToDateTime(uint16_t MJD, ReturnArg<DateTime> Time) noexcept
{
	if (!Time)
		return;

	Time->Reset();

	ParseMJDTime(MJD, &Time->Year, &Time->Month, &Time->Day, &Time->DayOfWeek);
}


// DateTime を MJD に変換する
uint16_t DateTimeToMJDTime(const DateTime &Time) noexcept
{
	return MakeMJDTime(Time.Year, Time.Month, Time.Day);
}


// BCD の時刻を時/分/秒に変換する
void ParseBCDTime(const uint8_t *pBCD, ReturnArg<int> Hour, ReturnArg<int> Minute, ReturnArg<int> Second) noexcept
{
	if (pBCD == nullptr)
		return;

	Hour   = GetBCD(pBCD[0]);
	Minute = GetBCD(pBCD[1]);
	Second = GetBCD(pBCD[2]);
}


// 時/分/秒 を BCD の時刻に変換する
bool MakeBCDTime(int Hour, int Minute, int Second, uint8_t *pBCD) noexcept
{
	if (pBCD == nullptr)
		return false;

	pBCD[0] = MakeBCD(Hour);
	pBCD[1] = MakeBCD(Minute);
	pBCD[2] = MakeBCD(Second);

	return true;
}


// BCD の時刻を秒単位に変換する
uint32_t BCDTimeToSecond(const uint8_t *pBCD) noexcept
{
	if (pBCD == nullptr)
		return 0;

	// 全ビットが1の場合は未定義
	if ((pBCD[0] == 0xFF) && (pBCD[1] == 0xFF) && (pBCD[2] == 0xFF))
		return 0;

	return (static_cast<uint32_t>(GetBCD(pBCD[0])) * 3600) +
	       (static_cast<uint32_t>(GetBCD(pBCD[1])) * 60) +
	       (static_cast<uint32_t>(GetBCD(pBCD[2])));
}


// BCD の時/分を分単位に変換する
uint16_t BCDTimeHMToMinute(uint16_t BCD) noexcept
{
	return ((((BCD >> 12)         * 10) + ((BCD >> 8) & 0x0F)) * 60) +
	       ((((BCD >>  4) & 0x0F) * 10) + ( BCD       & 0x0F));
}


}	// namespace LibISDB
