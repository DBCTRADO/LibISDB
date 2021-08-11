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
 @file   DirectShowUtilities.cpp
 @brief  DirectShow 用ユーティリティ
 @author DBCTRADO
*/


#include "../../../LibISDBPrivate.hpp"
#include "../../../LibISDBWindows.hpp"
#include <initguid.h>
#include <ks.h>
#include "DirectShowUtilities.hpp"
#include "../../../Utilities/StringUtilities.hpp"
#include "../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{

namespace
{


bool GetFilterInfoListFromEnumMoniker(IEnumMoniker *pEnum, FilterInfoList *pFilterList)
{
	bool OK = false;
	IMoniker *pMoniker;
	ULONG cFetched;

	while (pEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
		FilterInfo Info;

		LPOLESTR pDisplayName;
		HRESULT hr = pMoniker->GetDisplayName(nullptr, nullptr, &pDisplayName);
		if (SUCCEEDED(hr)) {
			Info.MonikerName = pDisplayName;
			::CoTaskMemFree(pDisplayName);
		}

		IPropertyBag *pPropBag;

		hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
		if (SUCCEEDED(hr)) {
			VARIANT var;

			::VariantInit(&var);
			hr = pPropBag->Read(L"FriendlyName", &var, 0);
			if (SUCCEEDED(hr)) {
				Info.FriendlyName = var.bstrVal;
				::VariantClear(&var);

				hr = pPropBag->Read(L"CLSID", &var, 0);
				if (SUCCEEDED(hr)) {
					::CLSIDFromString(var.bstrVal, &Info.clsid);
					::VariantClear(&var);

					pFilterList->emplace_back(std::move(Info));
					OK = true;
				}
			}

			pPropBag->Release();
		}

		pMoniker->Release();
	}

	return OK;
}


}	// namespace




void FilterFinder::Clear()
{
	m_FilterList.clear();
}


int FilterFinder::GetFilterCount() const
{
	return static_cast<int>(m_FilterList.size());
}


bool FilterFinder::GetFilterInfo(int Index, FilterInfo *pInfo) const
{
	if ((Index < 0) || (Index >= GetFilterCount()))
		return false;
	if (pInfo == nullptr)
		return false;

	*pInfo = m_FilterList[Index];

	return true;
}


bool FilterFinder::GetFilterInfo(
	int Index, CLSID *pClass, std::wstring *pFriendlyName, std::wstring *pMonikerName) const
{
	if ((Index < 0) || (Index >= GetFilterCount()))
		return false;

	const FilterInfo &Info = m_FilterList[Index];

	if (pClass != nullptr)
		*pClass = Info.clsid;

	if (pFriendlyName != nullptr)
		*pFriendlyName = Info.FriendlyName;

	if (pMonikerName != nullptr)
		*pMonikerName = Info.MonikerName;

	return true;
}


bool FilterFinder::GetFilterList(FilterInfoList *pList) const
{
	if (pList == nullptr)
		return false;

	*pList = m_FilterList;

	return !pList->empty();
}


bool FilterFinder::FindFilters(
	const GUID *pInTypes, int InTypeCount,
	const GUID *pOutTypes, int OutTypeCount,
	DWORD Merit)
{
	// フィルタを検索する
	IFilterMapper2 *pMapper;
	HRESULT hr = ::CoCreateInstance(CLSID_FilterMapper2, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&pMapper));
	if (FAILED(hr))
		return false;

	bool OK = false;

	IEnumMoniker *pEnum;

	hr = pMapper->EnumMatchingFilters(
		&pEnum,       // ppEnum
		0,            // dwFlags
		TRUE,         // bExactMatch
		Merit,        // dwMerit
		TRUE,         // bInputNeeded
		InTypeCount,  // cInputTypes
		pInTypes,     // pInputTypes
		nullptr,      // pMedIn
		nullptr,      // pPinCategoryIn
		FALSE,        // bRender
		TRUE,         // bOutputNeeded
		OutTypeCount, // cOutputTypes
		pOutTypes,    // pOutputTypes
		nullptr,      // pMedOut
		nullptr       // pPinCategoryOut
	);

	if (SUCCEEDED(hr)) {
		hr = GetFilterInfoListFromEnumMoniker(pEnum, &m_FilterList);
		pEnum->Release();
	}

	pMapper->Release();

	return OK;
}


bool FilterFinder::FindFilters(
	const GUID *pidInType, const GUID *pidInSubType,
	const GUID *pidOutType, const GUID *pidOutSubType,
	DWORD Merit)
{
	GUID InType[2], OutType[2];
	GUID *pInTypes = nullptr, *pOutTypes = nullptr;

	if ((pidInType != nullptr) || (pidInSubType != nullptr)) {
		InType[0] = pidInType != nullptr ? *pidInType : GUID_NULL;
		InType[1] = pidInSubType != nullptr ? *pidInSubType : GUID_NULL;
		pInTypes = InType;
	}
	if ((pidOutType != nullptr) || (pidOutSubType != nullptr)) {
		OutType[0] = pidOutType != nullptr ? *pidOutType : GUID_NULL;
		OutType[1] = pidOutSubType != nullptr ? *pidOutSubType : GUID_NULL;
		pOutTypes = OutType;
	}

	return FindFilters(
		pInTypes, pInTypes != nullptr ? 1 : 0,
		pOutTypes, pOutTypes != nullptr ? 1 : 0,
		Merit);
}


bool FilterFinder::SetPreferredFilter(const CLSID &idFilter)
{
	FilterInfoList TempList;

	TempList.reserve(m_FilterList.size());

	for (auto const &e : m_FilterList) {
		if (e.clsid == idFilter) {
			TempList.push_back(e);
		}
	}

	if (TempList.empty())
		return false;

	for (auto const &e : m_FilterList) {
		if (e.clsid != idFilter) {
			TempList.push_back(e);
		}
	}

	m_FilterList = std::move(TempList);

	return true;
}




void DeviceEnumerator::Clear()
{
	m_DeviceList.clear();
}


int DeviceEnumerator::GetDeviceCount() const
{
	return static_cast<int>(m_DeviceList.size());
}


bool DeviceEnumerator::EnumDevice(const CLSID &clsidDeviceClass)
{
	HRESULT hr;
	ICreateDevEnum *pDevEnum;

	hr = ::CoCreateInstance(
		CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pDevEnum));
	if (FAILED(hr))
		return false;

	IEnumMoniker *pEnumCategory;
	hr = pDevEnum->CreateClassEnumerator(clsidDeviceClass, &pEnumCategory, 0);
	if (hr == S_OK) {
		hr = GetFilterInfoListFromEnumMoniker(pEnumCategory, &m_DeviceList);
		pEnumCategory->Release();
	}

	pDevEnum->Release();

	return true;
}


bool DeviceEnumerator::CreateFilter(const CLSID &clsidDeviceClass, LPCWSTR pszFriendlyName, IBaseFilter **ppFilter)
{
	HRESULT hr;
	ICreateDevEnum *pDevEnum;

	hr = ::CoCreateInstance(
		CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pDevEnum));
	if (FAILED(hr))
		return false;

	IEnumMoniker *pEnumCategory;
	hr = pDevEnum->CreateClassEnumerator(clsidDeviceClass, &pEnumCategory, 0);

	bool Found = false;

	if (hr == S_OK) {
		IMoniker *pMoniker;
		ULONG cFetched;

		while (pEnumCategory->Next(1, &pMoniker, &cFetched) == S_OK) {
			IPropertyBag *pPropBag;

			hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
			if (SUCCEEDED(hr)) {
				VARIANT varName;

				::VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr)) {
					if (::lstrcmpiW(varName.bstrVal, pszFriendlyName) == 0) {
						hr = pMoniker->BindToObject(nullptr, nullptr, IID_PPV_ARGS(ppFilter));
						Found = true;
					}
					::VariantClear(&varName);
				}

				pPropBag->Release();
			}

			pMoniker->Release();

			if (Found)
				break;
		}

		pEnumCategory->Release();
	}

	pDevEnum->Release();

	if (!Found)
		return false;
	return SUCCEEDED(hr);
}


bool DeviceEnumerator::GetFilterInfo(int Index, FilterInfo *pInfo) const
{
	if ((Index < 0) || (Index >= GetDeviceCount()))
		return false;
	if (pInfo == nullptr)
		return false;

	*pInfo = m_DeviceList[Index];

	return true;
}


bool DeviceEnumerator::GetFilterList(FilterInfoList *pList) const
{
	if (pList == nullptr)
		return false;

	*pList = m_DeviceList;

	return !pList->empty();
}




HRESULT AddToROT(IUnknown *pUnkGraph, DWORD *pRegister)
{
	HRESULT hr;
	IRunningObjectTable *pROT;

	hr = ::GetRunningObjectTable(0, &pROT);
	if (FAILED(hr))
		return hr;

	WCHAR Name[256];
	StringPrintf(Name, L"FilterGraph %p pid %08x", pUnkGraph, ::GetCurrentProcessId());

	IMoniker *pMoniker;
	hr = ::CreateItemMoniker(L"!", Name, &pMoniker);

	if (SUCCEEDED(hr)) {
		hr = pROT->Register(0, pUnkGraph, pMoniker, pRegister);
		pMoniker->Release();
	}

	pROT->Release();

	return hr;
}


void RemoveFromROT(DWORD Register)
{
	IRunningObjectTable *pROT;

	if (SUCCEEDED(::GetRunningObjectTable(0, &pROT))) {
		pROT->Revoke(Register);
		pROT->Release();
	}
}


// フィルタから指定メディアのピンを検索する
IPin * GetFilterPin(IBaseFilter *pFilter, const PIN_DIRECTION dir, const AM_MEDIA_TYPE *pMediaType)
{
	IPin *pRetPin = nullptr;
	IEnumPins *pEnumPins;

	if (pFilter->EnumPins(&pEnumPins) == S_OK) {
		IPin *pPin;
		ULONG cFetched;

		while ((pRetPin == nullptr) && (pEnumPins->Next(1, &pPin, &cFetched) == S_OK)){
			PIN_INFO Pin;

			if (pPin->QueryPinInfo(&Pin) == S_OK){
				if (Pin.dir == dir) {
					if (pMediaType == nullptr) {
						pRetPin = pPin;
						pRetPin->AddRef();
					} else {
						if (pPin->QueryAccept(pMediaType) == S_OK){
							pRetPin = pPin;
							pRetPin->AddRef();
						}
					}
				}

				if (Pin.pFilter != nullptr)
					Pin.pFilter->Release();
			}

			pPin->Release();
		}

		pEnumPins->Release();
	}

	return pRetPin;
}


// フィルタのプロパティページを開く
bool ShowPropertyPage(IBaseFilter *pFilter, HWND hwndOwner)
{
	if (pFilter == nullptr)
		return false;

	bool Result = false;
	ISpecifyPropertyPages *pProp;

	HRESULT hr = pFilter->QueryInterface(IID_PPV_ARGS(&pProp));
	if (SUCCEEDED(hr)) {
		CAUUID caGUID = {0, nullptr};

		if (SUCCEEDED(pProp->GetPages(&caGUID))) {
			FILTER_INFO FilterInfo;

			hr = pFilter->QueryFilterInfo(&FilterInfo);
			if (SUCCEEDED(hr)) {
				IUnknown *pUnknown;

				hr = pFilter->QueryInterface(IDD_PPV_ARGS_IUNKNOWN(&pUnknown));
				if (SUCCEEDED(hr)) {
					::OleCreatePropertyFrame(
						hwndOwner,
						0, 0,
						FilterInfo.achName,
						1,
						&pUnknown,
						caGUID.cElems,
						caGUID.pElems,
						0,
						0, nullptr
					);
					pUnknown->Release();
					Result = true;
				}

				if (FilterInfo.pGraph != nullptr)
					FilterInfo.pGraph->Release();
			}

			::CoTaskMemFree(caGUID.pElems);
		}

		pProp->Release();
	}

	return Result;
}


bool HasPropertyPage(IBaseFilter *pFilter)
{
	bool Result = false;

	if (pFilter != nullptr) {
		ISpecifyPropertyPages *pProp;
		HRESULT hr = pFilter->QueryInterface(IID_PPV_ARGS(&pProp));
		if (SUCCEEDED(hr)) {
			CAUUID caGUID = {0, nullptr};

			if (SUCCEEDED(pProp->GetPages(&caGUID))) {
				Result = (caGUID.cElems > 0);
				::CoTaskMemFree(caGUID.pElems);
			}
			pProp->Release();
		}
	}

	return Result;
}


/**
 @brief 指定されたフィルタを指定されたピンに接続する

 @param[in]     pGraphBuilder フィルタグラフの IGraphBuilder
 @param[in]     pFilter       接続するフィルタ
 @param[in]     pszFilterName フィルタ名(nullptr でも可)
 @param[in,out] pOutputPin    接続するピン。正常終了なら解放され、換わりにフィルタの出力ピンが返される
 @param[in]     Direct        直接接続に限定する場合は true

 @return 結果
*/
HRESULT AppendFilterAndConnect(
	IGraphBuilder *pGraphBuilder,
	IBaseFilter *pFilter, LPCWSTR pszFilterName,
	COMPointer<IPin> *pOutputPin, bool Direct)
{
	HRESULT hr;

	// ポインタチェック
	if ((pGraphBuilder == nullptr) || (pFilter == nullptr) || (pOutputPin == nullptr))
		return E_POINTER;

	hr = pGraphBuilder->AddFilter(pFilter, (pszFilterName != nullptr) ? pszFilterName : L"No Name");
	if (FAILED(hr)) {
		// フィルタグラフに追加できない
		return hr;
	}

	IPin *pInput = GetFilterPin(pFilter, PINDIR_INPUT);
	if (pInput == nullptr) {
		// 入力ピンが見つからない
		pGraphBuilder->RemoveFilter(pFilter);
		return E_FAIL;
	}

	// 接続
	if (Direct)
		hr = pGraphBuilder->ConnectDirect(pOutputPin->Get(), pInput, nullptr);
	else
		hr = pGraphBuilder->Connect(pOutputPin->Get(), pInput);
	pInput->Release();
	if (FAILED(hr)) {
		// 接続できない
		pGraphBuilder->RemoveFilter(pFilter);
		return hr;
	}

	// 出力ピンを返す
	pOutputPin->Attach(GetFilterPin(pFilter, PINDIR_OUTPUT));

	return S_OK;
}


/**
 @brief 指定されたフィルタを指定されたピンに接続する

 @param[in]     pGraphBuilder   フィルタグラフの IGraphBuilder
 @param[in]     clsidFilter     接続するフィルタの CLSID
 @param[in]     pszFilterName   フィルタ名(nullptr でも可)
 @param[out]    pAppendedFilter 接続されたフィルタを返す変数へのポインタ。不要な場合は nullptr
 @param[in,out] pOutputPin      接続するピン。正常終了なら解放され、換わりにフィルタの出力ピンが返される
 @param[in]     Direct          直接接続に限定する場合は true

 @return 結果
*/
HRESULT AppendFilterAndConnect(
	IGraphBuilder *pGraphBuilder,
	const CLSID &clsidFilter, LPCWSTR pszFilterName, COMPointer<IBaseFilter> *pAppendedFilter,
	COMPointer<IPin> *pOutputPin, bool Direct)
{
	// フィルタのインスタンス作成
	IBaseFilter *pFilter;
	HRESULT hr = ::CoCreateInstance(
		clsidFilter, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&pFilter));
	if (FAILED(hr)) {
		return hr;
	}

	hr = AppendFilterAndConnect(
		pGraphBuilder, pFilter, pszFilterName,
		pOutputPin, Direct);
	if (FAILED(hr)) {
		pFilter->Release();
		return hr;
	}

	if (pAppendedFilter != nullptr)
		pAppendedFilter->Attach(pFilter);
	else
		pFilter->Release();

	return S_OK;
}


HRESULT CreateFilterFromMonikerName(
	LPCWSTR pszMonikerName, COMPointer<IBaseFilter> *pFilter, std::wstring *pFriendlyName)
{
	if (StringIsEmpty(pszMonikerName))
		return E_INVALIDARG;
	if (pFilter == nullptr)
		return E_POINTER;

	IBindCtx *pBindCtx;
	HRESULT hr = ::CreateBindCtx(0, &pBindCtx);
	if (FAILED(hr))
		return hr;

	IMoniker *pMoniker;
	ULONG Eaten;
	hr = ::MkParseDisplayName(pBindCtx, pszMonikerName, &Eaten, &pMoniker);
	if (SUCCEEDED(hr)) {
		IBaseFilter *pBaseFilter;
		hr = pMoniker->BindToObject(nullptr, nullptr, IID_PPV_ARGS(&pBaseFilter));
		if (SUCCEEDED(hr)) {
			pFilter->Attach(pBaseFilter);

			if (pFriendlyName != nullptr) {
				pFriendlyName->clear();

				IPropertyBag *pPropBag;
				HRESULT hrName = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
				if (SUCCEEDED(hrName)) {
					VARIANT varName;
					::VariantInit(&varName);
					hrName = pPropBag->Read(L"FriendlyName", &varName, 0);
					if (SUCCEEDED(hrName)) {
						*pFriendlyName = varName.bstrVal;
						::VariantClear(&varName);
					}

					pPropBag->Release();
				}
			}
		}

		pMoniker->Release();
	}

	pBindCtx->Release();

	return hr;
}


RECT MapRect(const RECT &Rect, LONG XNum, LONG XDenom, LONG YNum, LONG YDenom)
{
	RECT rc;

	rc.left   = ::MulDiv(Rect.left,   XNum, XDenom);
	rc.top    = ::MulDiv(Rect.top,    YNum, YDenom);
	rc.right  = ::MulDiv(Rect.right,  XNum, XDenom);
	rc.bottom = ::MulDiv(Rect.bottom, YNum, YDenom);

	return rc;
}


}	// namespace LibISDB::DirectShow
