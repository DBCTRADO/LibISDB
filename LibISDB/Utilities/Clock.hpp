/*
  LibISDB
  Copyright(c) 2017-2020 DBCTRADO

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
 @file   Clock.hpp
 @brief  クロック
 @author DBCTRADO
*/


#ifndef LIBISDB_CLOCK_H
#define LIBISDB_CLOCK_H


#include <chrono>
#ifdef LIBISDB_WINDOWS
#include "../LibISDBWindows.hpp"
#else
#include <ctime>
#endif


namespace LibISDB
{

	/** クロッククラス */
	class TickClock
	{
	public:
#ifdef LIBISDB_WINDOWS

		typedef uint64_t ClockType;
		static constexpr ClockType ClocksPerSec = 1000_u64;
		typedef std::chrono::milliseconds DurationType;

		ClockType Get() const noexcept
		{
			return ::GetTickCount64();
		}

#else	// LIBISDB_WINDOWS

		typedef uint64_t ClockType;
		static constexpr ClockType ClocksPerSec = 1000000000_u64;
		typedef std::chrono::nanoseconds DurationType;

		ClockType Get() const noexcept
		{
#ifdef CLOCK_MONOTONIC
			struct ::timespec Time;

			if (::clock_gettime(
#ifdef CLOCK_MONOTONIC_COARSE
					CLOCK_MONOTONIC_COARSE,
#elif defined(CLOCK_MONOTONIC_FAST)
					CLOCK_MONOTONIC_FAST,
#else
					CLOCK_MONOTONIC,
#endif
					&Time) == 0) {
				return (static_cast<uint64_t>(Time.tv_sec) * 1000000000_u64) +
				       static_cast<uint64_t>(Time.tv_nsec);
			}
#endif

			if constexpr ((ClocksPerSec > CLOCKS_PER_SEC) && (ClocksPerSec % CLOCKS_PER_SEC == 0)) {
				return static_cast<uint64_t>(std::clock()) * (ClocksPerSec / CLOCKS_PER_SEC);
			} else {
				return static_cast<uint64_t>(std::clock()) * ClocksPerSec / CLOCKS_PER_SEC;
			}
		}

#endif
	};

	/** 高精度クロッククラス */
	class HighPrecisionTickClock
	{
	public:
#ifdef LIBISDB_WINDOWS

		typedef uint64_t ClockType;
		static constexpr ClockType ClocksPerSec = 1000000_u64;
		typedef std::chrono::microseconds DurationType;

		HighPrecisionTickClock() noexcept
		{
			if (!::QueryPerformanceFrequency(&m_Frequency))
				m_Frequency.QuadPart = 0;
		}

		ClockType Get() const noexcept
		{
			if (m_Frequency.QuadPart != 0) {
				::LARGE_INTEGER Count;
				if (::QueryPerformanceCounter(&Count))
					return Count.QuadPart * 1000000_i64 / m_Frequency.QuadPart;
			}

			// Fallback
			return ::GetTickCount64() * 1000_u64;
		}

	private:
		::LARGE_INTEGER m_Frequency;

#else	// LIBISDB_WINDOWS

		typedef uint64_t ClockType;
		static constexpr ClockType ClocksPerSec = 1000000000_u64;
		typedef std::chrono::nanoseconds DurationType;

		ClockType Get() const noexcept
		{
#ifdef CLOCK_MONOTONIC
			struct ::timespec Time;

			if (::clock_gettime(
#ifdef CLOCK_MONOTONIC_PRECISE
					CLOCK_MONOTONIC_PRECISE,
#else
					CLOCK_MONOTONIC,
#endif
					&Time) == 0) {
				return (static_cast<uint64_t>(Time.tv_sec) * 1000000000_u64) +
				       static_cast<uint64_t>(Time.tv_nsec);
			}
#endif

			if constexpr ((ClocksPerSec > CLOCKS_PER_SEC) && (ClocksPerSec % CLOCKS_PER_SEC == 0)) {
				return static_cast<uint64_t>(std::clock()) * (ClocksPerSec / CLOCKS_PER_SEC);
			} else {
				return static_cast<uint64_t>(std::clock()) * ClocksPerSec / CLOCKS_PER_SEC;
			}
		}

#endif
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_CLOCK_H
