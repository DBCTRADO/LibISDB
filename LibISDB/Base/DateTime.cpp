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
 @file   DateTime.cpp
 @brief  日時
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "DateTime.hpp"
#include "DebugDef.hpp"


namespace LibISDB
{


bool IsLeapYear(int Year)
{
	return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0));
}


int GetDayOfYear(int Year, int Month, int Day)
{
	static const int MonthDays[11] = {
		31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};

	if (LIBISDB_TRACE_ERROR_IF((Month < 1) || (Month > 12)))
		return -1;

	int YearDay = Day - 1;

	if (Month >= 2) {
		YearDay += MonthDays[Month - 2];
		if (Month >= 3 && IsLeapYear(Year))
			YearDay++;
	}

	return YearDay;
}


int GetDayOfWeek(int Year, int Month, int Day)
{
	LIBISDB_ASSERT((Year >= 1583) && (Year <= 3999) && (Month >= 1) && (Month <= 12) && (Day >= 1) && (Day <= 31));

	// Zeller's congruence
	if (Month <= 2) {
		Year--;
		Month += 12;
	}
	return (Year * 365 + Year / 4 - Year / 100 + Year / 400 + 306 * (Month + 1) / 10 + Day - 428) % 7;
}




DateTime::DateTime() noexcept
	: Year(0)
	, Month(0)
	, Day(0)
	, DayOfWeek(0)
	, Minute(0)
	, Hour(0)
	, Second(0)
	, Millisecond(0)
{
}


DateTime::DateTime(const std::tm &Src) noexcept
{
	FromTm(Src);
}


bool DateTime::operator == (const DateTime &rhs) const noexcept
{
	return (Year == rhs.Year)
		&& (Month == rhs.Month)
		&& (Day == rhs.Day)
		&& (DayOfWeek == rhs.DayOfWeek)
		&& (Hour == rhs.Hour)
		&& (Minute == rhs.Minute)
		&& (Second == rhs.Second)
		&& (Millisecond == rhs.Millisecond);
}


bool DateTime::operator < (const DateTime &rhs) const noexcept
{
	return Compare(rhs) < 0;
}


bool DateTime::operator <= (const DateTime &rhs) const noexcept
{
	return Compare(rhs) <= 0;
}


bool DateTime::operator > (const DateTime &rhs) const noexcept
{
	return Compare(rhs) > 0;
}


bool DateTime::operator >= (const DateTime &rhs) const noexcept
{
	return Compare(rhs) >= 0;
}


void DateTime::FromTm(const std::tm &Src) noexcept
{
	Year        = Src.tm_year + 1900;
	Month       = Src.tm_mon + 1;
	Day         = Src.tm_mday;
	DayOfWeek   = Src.tm_wday;
	Hour        = Src.tm_hour;
	Minute      = Src.tm_min;
	Second      = Src.tm_sec;
	Millisecond = 0;
}


std::tm DateTime::ToTm() const noexcept
{
	std::tm To = {};

	To.tm_sec   = Second;
	To.tm_min   = Minute;
	To.tm_hour  = Hour;
	To.tm_mday  = Day;
	To.tm_mon   = Month - 1;
	To.tm_year  = Year - 1900;
	To.tm_wday  = DayOfWeek;
	To.tm_yday  = GetDayOfYear(Year, Month, Day);
	To.tm_isdst = -1;

	return To;
}


#ifdef LIBISDB_WINDOWS


DateTime::DateTime(const ::SYSTEMTIME &Src) noexcept
{
	FromSYSTEMTIME(Src);
}


void DateTime::FromSYSTEMTIME(const ::SYSTEMTIME &Src) noexcept
{
	Year        = Src.wYear;
	Month       = Src.wMonth;
	Day         = Src.wDay;
	DayOfWeek   = Src.wDayOfWeek;
	Hour        = Src.wHour;
	Minute      = Src.wMinute;
	Second      = Src.wSecond;
	Millisecond = Src.wMilliseconds;
}


::SYSTEMTIME DateTime::ToSYSTEMTIME() const noexcept
{
	::SYSTEMTIME To;

	To.wYear         = Year;
	To.wMonth        = Month;
	To.wDayOfWeek    = DayOfWeek;
	To.wDay          = Day;
	To.wHour         = Hour;
	To.wMinute       = Minute;
	To.wSecond       = Second;
	To.wMilliseconds = Millisecond;

	return To;
}


#endif	// LIBISDB_WINDOWS


void DateTime::Reset() noexcept
{
	*this = DateTime();
}


bool DateTime::IsValid() const noexcept
{
	return (Year >= 1)
		&& (Month >= 1) && (Month <= 12)
		&& (Day >= 1) && (Day <= 31)
		&& (DayOfWeek >= 0) && (DayOfWeek <= 6)
		&& (Hour >= 0) && (Hour <= 23)
		&& (Minute >= 0) && (Minute <= 59)
		&& (Second >= 0) && (Second <= 60)
		&& (Millisecond >= 0) && (Millisecond <= 999);
}


int DateTime::Compare(const DateTime &Time) const noexcept
{
	const long long Diff = DiffMilliseconds(Time);

	return (Diff < 0LL) ? -1 : (Diff > 0LL) ? 1 : 0;
}


long long DateTime::DiffSeconds(const DateTime &Time) const noexcept
{
	return static_cast<long long>(GetLinearSeconds()) - static_cast<long long>(Time.GetLinearSeconds());
}


long long DateTime::DiffMilliseconds(const DateTime &Time) const noexcept
{
	return static_cast<long long>(GetLinearMilliseconds()) - static_cast<long long>(Time.GetLinearMilliseconds());
}


std::chrono::milliseconds DateTime::Diff(const DateTime &Time) const noexcept
{
	return std::chrono::milliseconds(DiffMilliseconds(Time));
}


bool DateTime::OffsetSeconds(long Seconds) noexcept
{
#ifdef LIBISDB_WINDOWS

	return OffsetMilliseconds(static_cast<long long>(Seconds) * 1000LL);

#else	// LIBISDB_WINDOWS

	std::tm Tm = ToTm();
	std::time_t Time = ::timegm(&Tm);
	if (Time == (time_t)-1)
		return false;

	// TODO: Validate argument
	Time += Seconds;

	if (::gmtime_r(&Time, &Tm) == nullptr)
		return false;

	FromTm(Tm);

	return true;

#endif	// !def LIBISDB_WINDOWS
}


bool DateTime::OffsetMilliseconds(long long Milliseconds) noexcept
{
#ifdef LIBISDB_WINDOWS

	::SYSTEMTIME st = ToSYSTEMTIME();
	::FILETIME ft;
	::ULARGE_INTEGER Time;

	if (!::SystemTimeToFileTime(&st, &ft))
		return false;

	Time.LowPart = ft.dwLowDateTime;
	Time.HighPart = ft.dwHighDateTime;
	// TODO: Validate argument
	Time.QuadPart += Milliseconds * 10000LL;
	ft.dwLowDateTime = Time.LowPart;
	ft.dwHighDateTime = Time.HighPart;

	if (!::FileTimeToSystemTime(&ft, &st))
		return false;

	FromSYSTEMTIME(st);

	return true;

#else	// LIBISDB_WINDOWS

	const long long Seconds = Milliseconds / 1000LL;

	if ((Seconds < static_cast<long long>(std::numeric_limits<long>::min()))
			|| (Seconds > static_cast<long long>(std::numeric_limits<long>::max())))
		return false;

	return OffsetSeconds(static_cast<long>(Seconds));

#endif	// !def LIBISDB_WINDOWS
}


bool DateTime::Offset(const std::chrono::milliseconds &Milliseconds) noexcept
{
	return OffsetMilliseconds(Milliseconds.count());
}


unsigned long long DateTime::GetLinearSeconds() const noexcept
{
#ifdef LIBISDB_WINDOWS

	return GetLinearMilliseconds() / 1000ULL;

#else	// LIBISDB_WINDOWS

	std::tm Tm = ToTm();
	std::time_t Time = ::timegm(&Tm);

	if (Time == static_cast<std::time_t>(-1))
		return 0;

	return Time;

#endif	// !def LIBISDB_WINDOWS
}


unsigned long long DateTime::GetLinearMilliseconds() const noexcept
{
#ifdef LIBISDB_WINDOWS

	::SYSTEMTIME st = ToSYSTEMTIME();
	::FILETIME ft;

	if (!::SystemTimeToFileTime(&st, &ft))
		return 0;

	return ((static_cast<unsigned long long>(ft.dwHighDateTime) << 32) |
	         static_cast<unsigned long long>(ft.dwLowDateTime)) / 10000ULL;

#else	// LIBISDB_WINDOWS

	return GetLinearSeconds() * 1000ULL + Millisecond;

#endif	// !def LIBISDB_WINDOWS
}


bool DateTime::FromLinearSeconds(unsigned long long Seconds) noexcept
{
#ifdef LIBISDB_WINDOWS

	return FromLinearMilliseconds(Seconds * 1000ULL);

#else	// LIBISDB_WINDOWS

	std::time_t Time = Seconds;
	std::tm Tm;

	if (::gmtime_r(&Time, &Tm) == nullptr)
		return false;

	FromTm(Tm);

	return true;

#endif	// !def LIBISDB_WINDOWS
}


bool DateTime::FromLinearMilliseconds(unsigned long long Milliseconds) noexcept
{
#ifdef LIBISDB_WINDOWS

	const unsigned long long Time = Milliseconds * 10000ULL;
	::FILETIME ft;
	::SYSTEMTIME st;

	ft.dwLowDateTime = static_cast<DWORD>(Time & 0xFFFFFFFFULL);
	ft.dwHighDateTime = static_cast<DWORD>(Time >> 32);

	if (!::FileTimeToSystemTime(&ft, &st))
		return false;

	FromSYSTEMTIME(st);

	return true;

#else	// LIBISDB_WINDOWS

	if (!FromLinearSeconds(Milliseconds / 1000ULL))
		return false;

	Millisecond = static_cast<int>(Milliseconds % 1000ULL);

	return true;

#endif	// !def LIBISDB_WINDOWS
}


bool DateTime::NowLocal() noexcept
{
#ifdef LIBISDB_WINDOWS

	::SYSTEMTIME st;

	::GetLocalTime(&st);
	FromSYSTEMTIME(st);

	return true;

#else	// LIBISDB_WINDOWS

	std::time_t Time = std::time(nullptr);
	if (Time == static_cast<std::time_t>(-1))
		return false;

	std::tm Local;
	if (::localtime_r(&Time, &Local) == nullptr)
		return false;

	FromTm(Local);

	return true;

#endif	// !def LIBISDB_WINDOWS
}


bool DateTime::NowUTC() noexcept
{
#ifdef LIBISDB_WINDOWS

	::SYSTEMTIME st;

	::GetSystemTime(&st);
	FromSYSTEMTIME(st);

	return true;

#else	// LIBISDB_WINDOWS

	std::time_t Time = std::time(nullptr);
	if (Time == static_cast<std::time_t>(-1))
		return false;

	std::tm UTC;
	if (::gmtime_r(&Time, &UTC) == nullptr)
		return false;

	FromTm(UTC);

	return true;

#endif	// !def LIBISDB_WINDOWS
}


bool DateTime::ToLocal() noexcept
{
#ifdef LIBISDB_WINDOWS

	::SYSTEMTIME UTC = ToSYSTEMTIME(), Local;

	if (!::SystemTimeToTzSpecificLocalTime(nullptr, &UTC, &Local))
		return false;

	FromSYSTEMTIME(Local);

	return true;

#else	// LIBISDB_WINDOWS

	std::tm Tm = ToTm();
	std::time_t Time = ::timegm(&Tm);
	if (Time == static_cast<std::time_t>(-1))
		return false;

	if (::localtime_r(&Time, &Tm) == nullptr)
		return false;

	FromTm(Tm);

	return true;

#endif	// !def LIBISDB_WINDOWS
}


void DateTime::SetDayOfWeek() noexcept
{
	if (IsValid())
		DayOfWeek = GetDayOfWeek(Year, Month, Day);
}


void DateTime::TruncateToSeconds() noexcept
{
	Millisecond = 0;
}


void DateTime::TruncateToMinutes() noexcept
{
	Second = 0;
	Millisecond = 0;
}


void DateTime::TruncateToHours() noexcept
{
	Minute = 0;
	Second = 0;
	Millisecond = 0;
}


void DateTime::TruncateToDays() noexcept
{
	Hour = 0;
	Minute = 0;
	Second = 0;
	Millisecond = 0;
}


}	// namespace LibISDB
