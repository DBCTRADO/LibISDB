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
 @file   BonDriver.hpp
 @brief  BonDriver
 @author DBCTRADO
*/


#ifndef LIBISDB_BON_DRIVER_H
#define LIBISDB_BON_DRIVER_H


/** IBonDriver インターフェース */
class IBonDriver
{
public:
#ifndef LIBISDB_WINDOWS
	typedef int BOOL;
	typedef uint8_t BYTE;
	typedef uint32_t DWORD;
#endif

	virtual const BOOL OpenTuner() = 0;
	virtual void CloseTuner() = 0;

	virtual const BOOL SetChannel(const BYTE bCh) = 0;
	virtual const float GetSignalLevel() = 0;

	virtual const DWORD WaitTsStream(const DWORD dwTimeOut = 0) = 0;
	virtual const DWORD GetReadyCount() = 0;

	virtual const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain) = 0;
	virtual const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain) = 0;

	virtual void PurgeTsStream() = 0;

	virtual void Release() = 0;
};

/** IBonDriver2 インターフェース */
class IBonDriver2 : public IBonDriver
{
public:
#ifdef LIBISDB_WINDOWS
	typedef WCHAR CharType;
#else
	typedef char16_t CharType;
	typedef const CharType * LPCTSTR;
#endif

	virtual LPCTSTR GetTunerName() = 0;

	virtual const BOOL IsTunerOpening() = 0;

	virtual const LPCTSTR EnumTuningSpace(const DWORD dwSpace) = 0;
	virtual const LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel) = 0;

	virtual const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel) = 0;

	virtual const DWORD GetCurSpace() = 0;
	virtual const DWORD GetCurChannel() = 0;

// IBonDriver
	virtual void Release() = 0;
};

/** IBonDriver3 インターフェース */
class IBonDriver3 : public IBonDriver2
{
public:
	virtual const DWORD GetTotalDeviceNum() = 0;
	virtual const DWORD GetActiveDeviceNum() = 0;
	virtual const BOOL SetLnbPower(const BOOL bEnable) = 0;

// IBonDriver
	virtual void Release() = 0;
};


namespace LibISDB
{

	/** BonDriver クラス */
	class BonDriver
	{
	public:
		static constexpr uint32_t SPACE_INVALID   = 0xFFFFFFFF_u32;
		static constexpr uint32_t CHANNEL_INVALID = 0xFFFFFFFF_u32;

		BonDriver();
		~BonDriver();

		bool Load(const CStringView &FileName);
		void Unload();
		bool IsLoaded() const;
		bool CreateIBonDriver();
		void ReleaseIBonDriver();
		bool IsIBonDriverCreated() const;
		bool IsIBonDriver2() const;

		bool OpenTuner();
		void CloseTuner();
		bool IsTunerOpen() const;
		const IBonDriver2::CharType * GetTunerName() const;
		bool SetChannel(uint8_t Channel);
		bool SetChannel(uint32_t Space, uint32_t Channel);
		uint32_t GetCurSpace() const;
		uint32_t GetCurChannel() const;
		const IBonDriver2::CharType * EnumTuningSpace(uint32_t Space) const;
		const IBonDriver2::CharType * EnumChannelName(uint32_t Space, uint32_t Channel) const;
		float GetSignalLevel() const;
		uint32_t WaitTsStream(uint32_t Timeout = 0);
		uint32_t GetReadyCount() const;
		bool GetTsStream(uint8_t *pDst, uint32_t *pSize, uint32_t *pRemain);
		bool GetTsStream(uint8_t **ppDst, uint32_t *pSize, uint32_t *pRemain);
		void PurgeTsStream();

	private:
		HMODULE m_hLib;
		IBonDriver *m_pIBonDriver;
		IBonDriver2 *m_pIBonDriver2;
		bool m_IsOpen;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_BON_DRIVER_H
