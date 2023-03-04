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
 @file   SIMD.cpp
 @brief  SIMD
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "SIMD.hpp"
#include <atomic>

#if defined(LIBISDB_X86) || defined(LIBISDB_X64)
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif
#endif


namespace LibISDB
{

namespace
{


#if defined(LIBISDB_X86) || defined(LIBISDB_X64)


enum {
	INSTRUCTION_MMX    = 0x00000001U,
	INSTRUCTION_SSE    = 0x00000002U,
	INSTRUCTION_SSE2   = 0x00000004U,
	INSTRUCTION_SSE3   = 0x00000008U,
	INSTRUCTION_SSSE3  = 0x00000010U,
	INSTRUCTION_SSE4_1 = 0x00000020U,
	INSTRUCTION_SSE4_2 = 0x00000040U,
};


static unsigned int GetSupportedInstructions() noexcept
{
	int CPUInfo[4];

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	::__cpuid(CPUInfo, 1);
#elif defined(__GNUC__) || defined(__clang__)
	__cpuid(1, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#else
	static_assert(false);
#endif

	unsigned int Supported = 0;

	if (CPUInfo[3] & 0x00800000)
		Supported |= INSTRUCTION_MMX;
	if (CPUInfo[3] & 0x02000000)
		Supported |= INSTRUCTION_SSE;
	if (CPUInfo[3] & 0x04000000)
		Supported |= INSTRUCTION_SSE2;
	if (CPUInfo[2] & 0x00000001)
		Supported |= INSTRUCTION_SSE3;
	if (CPUInfo[2] & 0x00000200)
		Supported |= INSTRUCTION_SSSE3;
	if (CPUInfo[2] & 0x00080000)
		Supported |= INSTRUCTION_SSE4_1;
	if (CPUInfo[2] & 0x00100000)
		Supported |= INSTRUCTION_SSE4_2;

	return Supported;
}


class CPUIdentify
{
public:
	CPUIdentify() noexcept
	{
		m_Available = GetSupportedInstructions();
		m_Enabled = m_Available;

		LIBISDB_TRACE(
			LIBISDB_STR("Detected CPU features : ")
			LIBISDB_STR("MMX {} ")
			LIBISDB_STR("SSE {} ")
			LIBISDB_STR("SSE2 {} ")
			LIBISDB_STR("SSE3 {} ")
			LIBISDB_STR("SSSE3 {} ")
			LIBISDB_STR("SSE4.1 {} ")
			LIBISDB_STR("SSE4.2 {}\n"),
			(m_Available & INSTRUCTION_MMX)    ? LIBISDB_STR("avail") : LIBISDB_STR("n/a"),
			(m_Available & INSTRUCTION_SSE)    ? LIBISDB_STR("avail") : LIBISDB_STR("n/a"),
			(m_Available & INSTRUCTION_SSE2)   ? LIBISDB_STR("avail") : LIBISDB_STR("n/a"),
			(m_Available & INSTRUCTION_SSE3)   ? LIBISDB_STR("avail") : LIBISDB_STR("n/a"),
			(m_Available & INSTRUCTION_SSSE3)  ? LIBISDB_STR("avail") : LIBISDB_STR("n/a"),
			(m_Available & INSTRUCTION_SSE4_1) ? LIBISDB_STR("avail") : LIBISDB_STR("n/a"),
			(m_Available & INSTRUCTION_SSE4_2) ? LIBISDB_STR("avail") : LIBISDB_STR("n/a"));
	}

	bool IsAvailable(unsigned int Instruction) const noexcept
	{
		return (m_Available & Instruction) == Instruction;
	}

	bool IsEnabled(unsigned int Instruction) const noexcept
	{
		return (m_Enabled & Instruction) == Instruction;
	}

	void SetEnabled(unsigned int Instruction, bool Enabled) noexcept
	{
		if (Enabled)
			m_Enabled |= (Instruction & m_Available);
		else
			m_Enabled &= ~Instruction;
	}

private:
	unsigned int m_Available;
	std::atomic<unsigned int> m_Enabled;
};

static CPUIdentify g_CPUIdentify;


#endif	// defined(LIBISDB_X86) || defined(LIBISDB_X64)


}	// namespace




#if defined(LIBISDB_X86) || defined(LIBISDB_X64)


#ifndef LIBISDB_NATIVE_SSE2
bool IsSSE2Available() noexcept
{
	return g_CPUIdentify.IsAvailable(INSTRUCTION_SSE2);
}


bool IsSSE2Enabled() noexcept
{
	return g_CPUIdentify.IsEnabled(INSTRUCTION_SSE2);
}
#endif


void SetSSE2Enabled(bool Enabled) noexcept
{
	g_CPUIdentify.SetEnabled(INSTRUCTION_SSE2, Enabled);
}


#endif	// defined(LIBISDB_X86) || defined(LIBISDB_X64)


}	// namespace LibISDB
