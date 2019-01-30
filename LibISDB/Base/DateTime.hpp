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
 @file   DateTime.hpp
 @brief  日時
 @author DBCTRADO
*/


#ifndef LIBISDB_DATE_TIME_H
#define LIBISDB_DATE_TIME_H


#include <ctime>
#include <chrono>
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#endif


namespace LibISDB
{

	/** 日時クラス */
	class DateTime
	{
	public:
		int Year;        /**< 年(A.D.) */
		int Month;       /**< 月(1 - 12) */
		int Day;         /**< 日(1 - 31) */
		int DayOfWeek;   /**< 曜日(0 - 6 = Sunday, Monday, ...) */
		int Hour;        /**< 時(0 - 23) */
		int Minute;      /**< 分(0 - 59) */
		int Second;      /**< 秒(0 - 60) */
		int Millisecond; /**< ミリ秒(0 - 999) */

		DateTime() noexcept;
		DateTime(const std::tm &Src) noexcept;

		bool operator == (const DateTime &rhs) const noexcept;
		bool operator != (const DateTime &rhs) const noexcept { return !(*this == rhs); }
		bool operator < (const DateTime &rhs) const noexcept;
		bool operator <= (const DateTime &rhs) const noexcept;
		bool operator > (const DateTime &rhs) const noexcept;
		bool operator >= (const DateTime &rhs) const noexcept;

		void FromTm(const std::tm &Src) noexcept;
		std::tm ToTm() const noexcept;
#ifdef LIBISDB_WINDOWS
		DateTime(const ::SYSTEMTIME &Src) noexcept;
		void FromSYSTEMTIME(const ::SYSTEMTIME &Src) noexcept;
		::SYSTEMTIME ToSYSTEMTIME() const noexcept;
#endif
		void Reset() noexcept;
		bool IsValid() const noexcept;
		int Compare(const DateTime &Time) const noexcept;

		long long DiffSeconds(const DateTime &Time) const noexcept;
		long long DiffMilliseconds(const DateTime &Time) const noexcept;
		std::chrono::milliseconds Diff(const DateTime &Time) const noexcept;

		bool OffsetSeconds(long Seconds) noexcept;
		bool OffsetMinutes(long Minutes) noexcept { return OffsetSeconds(Minutes * 60); }
		bool OffsetHours(long Hours) noexcept { return OffsetSeconds(Hours * (60 * 60)); }
		bool OffsetDays(long Days) noexcept { return OffsetSeconds(Days * (24 * 60 * 60)); }
		bool OffsetMilliseconds(long long Milliseconds) noexcept;
		bool Offset(const std::chrono::milliseconds &Milliseconds) noexcept;

		unsigned long long GetLinearSeconds() const noexcept;
		unsigned long long GetLinearMilliseconds() const noexcept;
		bool FromLinearSeconds(unsigned long long Seconds) noexcept;
		bool FromLinearMilliseconds(unsigned long long Milliseconds) noexcept;

		bool NowLocal() noexcept;
		bool NowUTC() noexcept;
		bool ToLocal() noexcept;
		void SetDayOfWeek() noexcept;

		void TruncateToSeconds() noexcept;
		void TruncateToMinutes() noexcept;
		void TruncateToHours() noexcept;
		void TruncateToDays() noexcept;
	};

	bool IsLeapYear(int Year);
	int GetDayOfYear(int Year, int Month, int Day);
	int GetDayOfWeek(int Year, int Month, int Day);

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DATE_TIME_H
