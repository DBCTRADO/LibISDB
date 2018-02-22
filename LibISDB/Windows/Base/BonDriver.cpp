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
 @file   BonDriver.cpp
 @brief  BonDriver
 @author DBCTRADO
*/


#include "../../LibISDBPrivate.hpp"
#include "../../LibISDBWindows.hpp"
#include "BonDriver.hpp"
#include "../../Base/DebugDef.hpp"


#ifdef LIBISDB_ENABLE_SEH
#define LIBISDB_SEH_TRACE LIBISDB_TRACE_ERROR
#else
#define LIBISDB_SEH_TRACE(...) ((void)0)
#endif


namespace LibISDB
{


BonDriver::BonDriver()
	: m_hLib(nullptr)
	, m_pIBonDriver(nullptr)
	, m_pIBonDriver2(nullptr)
	, m_IsOpen(false)
{
}


BonDriver::~BonDriver()
{
	Unload();
}


bool BonDriver::Load(const CStringView &FileName)
{
	Unload();

	m_hLib = ::LoadLibrary(FileName.c_str());
	if (m_hLib == nullptr)
		return false;

	return true;
}


void BonDriver::Unload()
{
	if (m_hLib != nullptr) {
		ReleaseIBonDriver();

		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
	}
}


bool BonDriver::IsLoaded() const
{
	return m_hLib != nullptr;
}


bool BonDriver::CreateIBonDriver()
{
	if (m_hLib == nullptr)
		return false;
	if (m_pIBonDriver != nullptr)
		return false;

	auto pCreateBonDriver =
		reinterpret_cast<IBonDriver * (*)()>(::GetProcAddress(m_hLib, "CreateBonDriver"));
	if (pCreateBonDriver == nullptr)
		return false;

	LIBISDB_SEH_TRY {
		m_pIBonDriver = pCreateBonDriver();
	} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in CreateBonDriver()\n"));
	}
	if (m_pIBonDriver == nullptr)
		return false;

	m_pIBonDriver2 = dynamic_cast<IBonDriver2 *>(m_pIBonDriver);

	return true;
}


void BonDriver::ReleaseIBonDriver()
{
	if (m_pIBonDriver != nullptr) {
		LIBISDB_SEH_TRY {
			m_pIBonDriver->Release();
		} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
			LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in IBonDriver::Release()\n"));
		}
		m_pIBonDriver = nullptr;
		m_pIBonDriver2 = nullptr;
		m_IsOpen = false;
	}
}


bool BonDriver::IsIBonDriverCreated() const
{
	return m_pIBonDriver != nullptr;
}


bool BonDriver::IsIBonDriver2() const
{
	return m_pIBonDriver2 != nullptr;
}


bool BonDriver::OpenTuner()
{
	if (m_pIBonDriver == nullptr)
		return false;

	LIBISDB_SEH_TRY {
		if (!m_pIBonDriver->OpenTuner())
			return false;
	} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in IBonDriver::OpenTuner()\n"));
		return false;
	}

	m_IsOpen = true;

	return true;
}


void BonDriver::CloseTuner()
{
	if (m_pIBonDriver != nullptr) {
		LIBISDB_SEH_TRY {
			m_pIBonDriver->CloseTuner();
		} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		}

		m_IsOpen = false;
	}
}


bool BonDriver::IsTunerOpen() const
{
	// IBonDriver2::IsTunerOpening は常に FALSE を返す実装がされているものがあるため利用できない
#if 0
	if (m_pIBonDriver2 != nullptr)
		return m_pIBonDriver2->IsTunerOpening() != FALSE;
#endif

	return m_IsOpen;
}


const IBonDriver2::CharType * BonDriver::GetTunerName() const
{
	if (m_pIBonDriver2 == nullptr)
		return nullptr;

	return m_pIBonDriver2->GetTunerName();
}


bool BonDriver::SetChannel(uint8_t Channel)
{
	if (m_pIBonDriver == nullptr)
		return false;

	LIBISDB_SEH_TRY {
		if (m_pIBonDriver->SetChannel(Channel))
			return true;
	} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in IBonDriver::SetChannel()\n"));
	}

	return false;
}


bool BonDriver::SetChannel(uint32_t Space, uint32_t Channel)
{
	if (m_pIBonDriver2 == nullptr)
		return false;

	LIBISDB_SEH_TRY {
		if (m_pIBonDriver2->SetChannel(Space, Channel))
			return true;
	} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in IBonDriver2::SetChannel()\n"));
	}

	return false;
}


uint32_t BonDriver::GetCurSpace() const
{
	if (m_pIBonDriver2 == nullptr)
		return SPACE_INVALID;

	return m_pIBonDriver2->GetCurSpace();
}


uint32_t BonDriver::GetCurChannel() const
{
	if (m_pIBonDriver2 == nullptr)
		return CHANNEL_INVALID;

	return m_pIBonDriver2->GetCurChannel();
}


const IBonDriver2::CharType * BonDriver::EnumTuningSpace(uint32_t Space) const
{
	if (m_pIBonDriver2 == nullptr)
		return nullptr;

	return m_pIBonDriver2->EnumTuningSpace(Space);
}


const IBonDriver2::CharType * BonDriver::EnumChannelName(uint32_t Space, uint32_t Channel) const
{
	if (m_pIBonDriver2 == nullptr)
		return nullptr;

	return m_pIBonDriver2->EnumChannelName(Space, Channel);
}


float BonDriver::GetSignalLevel() const
{
	if (m_pIBonDriver == nullptr)
		return 0.0f;

	return m_pIBonDriver->GetSignalLevel();
}


uint32_t BonDriver::WaitTsStream(uint32_t Timeout)
{
	if (m_pIBonDriver == nullptr)
		return 0;

	return m_pIBonDriver->WaitTsStream(Timeout);
}


uint32_t BonDriver::GetReadyCount() const
{
	if (m_pIBonDriver == nullptr)
		return 0;

	return m_pIBonDriver->GetReadyCount();
}


bool BonDriver::GetTsStream(uint8_t *pDst, uint32_t *pSize, uint32_t *pRemain)
{
	DWORD Size = *pSize, Remain = 0;

	*pSize = 0;
	if (pRemain != nullptr)
		*pRemain = 0;

	if (m_pIBonDriver == nullptr)
		return false;

	LIBISDB_SEH_TRY {
		if (!m_pIBonDriver->GetTsStream(pDst, &Size, &Remain))
			return false;
	} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in IBonDriver::GetTsStream()\n"));
		return false;
	}

	*pSize = Size;
	if (pRemain != nullptr)
		*pRemain = Remain;

	return true;
}


bool BonDriver::GetTsStream(uint8_t **ppDst, uint32_t *pSize, uint32_t *pRemain)
{
	*ppDst = nullptr;
	*pSize = 0;
	if (pRemain != nullptr)
		*pRemain = 0;

	if (m_pIBonDriver == nullptr)
		return false;

	BYTE *pDst = nullptr;
	DWORD Size = 0, Remain = 0;

	LIBISDB_SEH_TRY {
		if (!m_pIBonDriver->GetTsStream(&pDst, &Size, &Remain))
			return false;
	} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
		LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in IBonDriver::GetTsStream()\n"));
		return false;
	}

	*ppDst = reinterpret_cast<uint8_t *>(pDst);
	*pSize = Size;
	if (pRemain != nullptr)
		*pRemain = Remain;

	return true;
}


void BonDriver::PurgeTsStream()
{
	if (m_pIBonDriver != nullptr) {
		LIBISDB_SEH_TRY {
			m_pIBonDriver->PurgeTsStream();
		} LIBISDB_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
			LIBISDB_SEH_TRACE(LIBISDB_STR("SEH exception in IBonDriver::PurgeTsStream()\n"));
		}
	}
}


}	// namespace LibISDB
