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
 @file   DirectShowUtilities.hpp
 @brief  DirectShow 用ユーティリティ
 @author DBCTRADO
*/


#ifndef LIBISDB_DIRECTSHOW_UTILITIES_H
#define LIBISDB_DIRECTSHOW_UTILITIES_H


#include <vector>
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>
#include <wmcodecdsp.h>
#include "../../Utilities/COMUtilities.hpp"


#ifndef WAVE_FORMAT_AAC
#define WAVE_FORMAT_AAC 0x00FF
#endif

DEFINE_GUID(MEDIASUBTYPE_AAC,
	WAVE_FORMAT_AAC, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_H264_bis,
	0x8D2D71CB, 0x243F, 0x45E3, 0xB2, 0xD8, 0x5F, 0xD7, 0x96, 0x7E, 0xC0, 0x9B);

DEFINE_GUID(MEDIASUBTYPE_avc1,
	0x31637661, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

DEFINE_GUID(MEDIASUBTYPE_HEVC,
	0x43564548, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);


namespace LibISDB::DirectShow
{

	/** DirectShow フィルタ検索クラス */
	class FilterFinder
	{
	public:
		struct FilterInfo {
			CLSID clsid;
			std::wstring FriendlyName;

			FilterInfo(CLSID id, LPCWSTR pszFriendlyName)
				: clsid(id)
				, FriendlyName(pszFriendlyName)
			{
			}
		};

		typedef std::vector<FilterInfo> FilterList;

		void Clear();
		int GetFilterCount() const;
		bool GetFilterInfo(int Index, CLSID *pClass, std::wstring *pFriendlyName = nullptr) const;
		bool GetFilterList(FilterList *pList) const;
		bool FindFilters(
			const GUID *pInTypes, int InTypeCount,
			const GUID *pOutTypes = nullptr, int OutTypeCount = 0,
			DWORD Merit = MERIT_DO_NOT_USE + 1);
		bool FindFilters(
			const GUID *pidInType, const GUID *pidInSubType,
			const GUID *pidOutType = nullptr, const GUID *pidOutubType = nullptr,
			DWORD Merit = MERIT_DO_NOT_USE + 1);
		bool SetPreferredFilter(const CLSID &idFilter);

	protected:
		FilterList m_FilterList;
	};

	/** DirectShow デバイス列挙クラス */
	class DeviceEnumerator
	{
	public:
		void Clear();
		int GetDeviceCount() const;
		bool EnumDevice(const CLSID &clsidDeviceClass);
		bool CreateFilter(const CLSID &clsidDeviceClass, LPCWSTR pszFriendlyName, IBaseFilter **ppFilter);
		LPCWSTR GetDeviceFriendlyName(int Index) const;

	protected:
		struct DeviceInfo {
			std::wstring FriendlyName;

			DeviceInfo(LPCWSTR pszFriendlyName)
				: FriendlyName(pszFriendlyName)
			{
			}
		};

		std::vector<DeviceInfo> m_DeviceList;
	};


	HRESULT AddToROT(IUnknown *pUnkGraph, DWORD *pRegister);
	void RemoveFromROT(DWORD Register);

	IPin * GetFilterPin(IBaseFilter *pFilter, PIN_DIRECTION dir, const AM_MEDIA_TYPE *pMediaType = nullptr);

	bool ShowPropertyPage(IBaseFilter *pFilter, HWND hwndOwner);
	bool HasPropertyPage(IBaseFilter *pFilter);

	HRESULT AppendFilterAndConnect(
		IGraphBuilder *pGraphBuilder,
		IBaseFilter *pFilter, LPCWSTR pszFilterName,
		COMPointer<IPin> *pOutputPin, bool Direct = false);
	HRESULT AppendFilterAndConnect(
		IGraphBuilder *pGraphBuilder,
		const CLSID &clsidFilter, LPCWSTR pszFilterName, COMPointer<IBaseFilter> *pAppendedFilter,
		COMPointer<IPin> *pOutputPin, bool Direct = false);

	RECT MapRect(const RECT &Rect, LONG XNum, LONG XDenom, LONG YNum, LONG YDenom);

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_DIRECTSHOW_UTILITIES_H
