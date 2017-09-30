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
 @file   ARIBTime.hpp
 @brief  MJD/BCD 形式の日時の変換を行う
 @author DBCTRADO
*/


#ifndef LIBISDB_ARIB_TIME_H
#define LIBISDB_ARIB_TIME_H


#include "DateTime.hpp"


namespace LibISDB
{

	/**
	 @brief MJD+BCD の日時を DateTime に変換する

	 @param[in]  pData 変換元データ(5オクテット)
	 @param[out] Time  変換先 DateTime

	 @retval true  変換が行われた
	 @retval false 変換が行われなかった
	*/
	bool MJDBCDTimeToDateTime(const uint8_t *pData, ReturnArg<DateTime> Time) noexcept;

	/**
	 @brief MJD の日付を年/月/日/曜日に変換する

	 @param[in]  MJD       MJD の値
	 @param[out] Year      年(A.D.)
	 @param[out] Month     月(1 - 12)
	 @param[out] Day       日(1 - 31)
	 @param[out] DayOfWeek 曜日(0 - 6 = Sun. - Sat.)
	*/
	void ParseMJDTime(
		uint16_t MJD,
		ReturnArg<int> Year, ReturnArg<int> Month, ReturnArg<int> Day,
		OptionalReturnArg<int> DayOfWeek = std::nullopt) noexcept;

	/**
	 @brief 年/月/日を MJD に変換する

	 @param[in] Year  年(A.D.)
	 @param[in] Month 月(1 - 12)
	 @param[in] Day   日(1 - 31)

	 @return 変換した MJD の値
	*/
	uint16_t MakeMJDTime(int Year, int Month, int Day) noexcept;

	/**
	 @brief MJD を DateTime に変換する

	 @param[in]  MJD  MJD の値
	 @param[out] Time 変換先 DateTime
	*/
	void MJDTimeToDateTime(uint16_t MJD, ReturnArg<DateTime> Time) noexcept;

	/**
	 @brief DateTime を MJD に変換する

	 @param[in] Time 変換元 DateTime

	 @return 変換した MJD の値
	*/
	uint16_t DateTimeToMJDTime(const DateTime &Time) noexcept;

	/**
	 @brief BCD の時刻を時/分/秒に変換する

	 @param[in]  pBCD   変換元 BCD(3オクテット)
	 @param[out] Hour   時
	 @param[out] Minute 分
	 @param[out] Second 秒
	*/
	void ParseBCDTime(const uint8_t *pBCD, ReturnArg<int> Hour, ReturnArg<int> Minute, ReturnArg<int> Second) noexcept;

	/**
	 @brief 時/分/秒 を BCD の時刻に変換する

	 @param[in]  Hour   時
	 @param[in]  Minute 分
	 @param[in]  Second 秒
	 @param[out] pBCD   変換先 BCD(3オクテット)
	*/
	bool MakeBCDTime(int Hour, int Minute, int Second, uint8_t *pBCD) noexcept;

	/**
	 @brief BCD の時刻を秒単位に変換する

	 @param[in] pBCD 変換元 BCD(3オクテット)

	 @return 変換した秒の値。変換できなかった場合は0
	*/
	uint32_t BCDTimeToSecond(const uint8_t *pBCD) noexcept;

	/**
	 @brief BCD の時/分を分単位に変換する

	 @param[in] BCD 変換元 BCD

	 @return 変換した分の値
	*/
	uint16_t BCDTimeHMToMinute(uint16_t BCD) noexcept;

}	// namespace LibISDB


#endif	// ifndef LIBISDB_ARIB_TIME_H
