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
 @file   KnownDecoderManager.cpp
 @brief  既知のデコーダ管理
 @author DBCTRADO
*/


#include "../../../LibISDBPrivate.hpp"
#include "../../../LibISDBWindows.hpp"
#include "KnownDecoderManager.hpp"
#include <shlwapi.h>
#include "../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


KnownDecoderManager::KnownDecoderManager() noexcept
	: m_hLib(nullptr)
{
}


KnownDecoderManager::~KnownDecoderManager()
{
	FreeDecoderModule();
}


HRESULT KnownDecoderManager::CreateInstance(const GUID &MediaSubType, IBaseFilter **ppFilter)
{
	*ppFilter = nullptr;

	if (!IsMediaSupported(MediaSubType))
		return E_INVALIDARG;

	HRESULT hr;

	hr = LoadDecoderModule();
	if (FAILED(hr))
		return hr;

	auto pGetInfo = reinterpret_cast<decltype(TVTestVideoDecoder_GetInfo) *>(
		::GetProcAddress(m_hLib, "TVTestVideoDecoder_GetInfo"));
	auto pCreateInstance = reinterpret_cast<decltype(TVTestVideoDecoder_CreateInstance) *>(
		::GetProcAddress(m_hLib, "TVTestVideoDecoder_CreateInstance"));
	if (!pGetInfo || !pCreateInstance)
		return E_FAIL;

	TVTestVideoDecoderInfo DecoderInfo;
	DecoderInfo.HostVersion = TVTVIDEODEC_HOST_VERSION;
	if (!pGetInfo(&DecoderInfo)
			|| DecoderInfo.InterfaceVersion != TVTVIDEODEC_INTERFACE_VERSION)
		return E_FAIL;

	ITVTestVideoDecoder *pDecoder;

	hr = pCreateInstance(IID_PPV_ARGS(&pDecoder));
	if (FAILED(hr))
		return hr;
	hr = pDecoder->QueryInterface(IID_PPV_ARGS(ppFilter));
	if (FAILED(hr)) {
		pDecoder->Release();
		return hr;
	}

	pDecoder->SetEnableDeinterlace(m_VideoDecoderSettings.bEnableDeinterlace);
	pDecoder->SetDeinterlaceMethod(m_VideoDecoderSettings.DeinterlaceMethod);
	pDecoder->SetAdaptProgressive(m_VideoDecoderSettings.bAdaptProgressive);
	pDecoder->SetAdaptTelecine(m_VideoDecoderSettings.bAdaptTelecine);
	pDecoder->SetInterlacedFlag(m_VideoDecoderSettings.bSetInterlacedFlag);
	pDecoder->SetBrightness(m_VideoDecoderSettings.Brightness);
	pDecoder->SetContrast(m_VideoDecoderSettings.Contrast);
	pDecoder->SetHue(m_VideoDecoderSettings.Hue);
	pDecoder->SetSaturation(m_VideoDecoderSettings.Saturation);
	pDecoder->SetNumThreads(m_VideoDecoderSettings.NumThreads);
	pDecoder->SetEnableDXVA2(m_VideoDecoderSettings.bEnableDXVA2);

	ITVTestVideoDecoder2 *pDecoder2;
	if (SUCCEEDED(pDecoder->QueryInterface(IID_PPV_ARGS(&pDecoder2)))) {
		pDecoder2->SetEnableD3D11(m_VideoDecoderSettings.bEnableD3D11);
		pDecoder2->SetNumQueueFrames(m_VideoDecoderSettings.NumQueueFrames);
		pDecoder2->Release();
	}

	pDecoder->Release();

	return S_OK;
}


void KnownDecoderManager::SetVideoDecoderSettings(const VideoDecoderSettings &Settings)
{
	m_VideoDecoderSettings = Settings;
}


bool KnownDecoderManager::GetVideoDecoderSettings(VideoDecoderSettings *pSettings) const
{
	if (!pSettings)
		return false;
	*pSettings = m_VideoDecoderSettings;
	return true;
}


bool KnownDecoderManager::SaveVideoDecoderSettings(IBaseFilter *pFilter)
{
	if (!pFilter)
		return false;

	ITVTestVideoDecoder *pDecoder;
	HRESULT hr = pFilter->QueryInterface(IID_PPV_ARGS(&pDecoder));
	if (FAILED(hr))
		return false;

	m_VideoDecoderSettings.bEnableDeinterlace = pDecoder->GetEnableDeinterlace() != FALSE;
	m_VideoDecoderSettings.DeinterlaceMethod = pDecoder->GetDeinterlaceMethod();
	m_VideoDecoderSettings.bAdaptProgressive = pDecoder->GetAdaptProgressive() != FALSE;
	m_VideoDecoderSettings.bAdaptTelecine = pDecoder->GetAdaptTelecine() != FALSE;
	m_VideoDecoderSettings.bSetInterlacedFlag = pDecoder->GetInterlacedFlag() != FALSE;
	m_VideoDecoderSettings.Brightness = pDecoder->GetBrightness();
	m_VideoDecoderSettings.Contrast = pDecoder->GetContrast();
	m_VideoDecoderSettings.Hue = pDecoder->GetHue();
	m_VideoDecoderSettings.Saturation = pDecoder->GetSaturation();
	m_VideoDecoderSettings.NumThreads = pDecoder->GetNumThreads();
	m_VideoDecoderSettings.bEnableDXVA2 = pDecoder->GetEnableDXVA2() != FALSE;

	ITVTestVideoDecoder2 *pDecoder2;
	if (SUCCEEDED(pDecoder->QueryInterface(IID_PPV_ARGS(&pDecoder2)))) {
		m_VideoDecoderSettings.bEnableD3D11 = pDecoder2->GetEnableD3D11();
		m_VideoDecoderSettings.NumQueueFrames = pDecoder2->GetNumQueueFrames();
		pDecoder2->Release();
	}

	pDecoder->Release();

	return true;
}


bool KnownDecoderManager::IsMediaSupported(const GUID &MediaSubType)
{
	return (MediaSubType == MEDIASUBTYPE_MPEG2_VIDEO) != FALSE;
}


bool KnownDecoderManager::IsDecoderAvailable(const GUID &MediaSubType)
{
	if (!IsMediaSupported(MediaSubType))
		return false;

	TCHAR Path[MAX_PATH];

	if (!GetModulePath(Path))
		return false;

	return ::PathFileExists(Path) != FALSE;
}


LPCWSTR KnownDecoderManager::GetDecoderName(const GUID &MediaSubType)
{
	if (MediaSubType == MEDIASUBTYPE_MPEG2_VIDEO)
		return TVTVIDEODEC_FILTER_NAME;
	return nullptr;
}


CLSID KnownDecoderManager::GetDecoderCLSID(const GUID &MediaSubType)
{
	if (MediaSubType == MEDIASUBTYPE_MPEG2_VIDEO)
		return __uuidof(ITVTestVideoDecoder);
	return GUID_NULL;
}


HRESULT KnownDecoderManager::LoadDecoderModule()
{
	if (!m_hLib) {
		TCHAR szPath[MAX_PATH];

		GetModulePath(szPath);
		m_hLib = ::LoadLibrary(szPath);
		if (!m_hLib)
			return HRESULT_FROM_WIN32(::GetLastError());
	}

	return S_OK;
}


void KnownDecoderManager::FreeDecoderModule()
{
	if (m_hLib) {
		::FreeLibrary(m_hLib);
		m_hLib = nullptr;
	}
}


bool KnownDecoderManager::GetModulePath(LPTSTR pszPath)
{
	::GetModuleFileName(nullptr, pszPath, MAX_PATH);
	::PathRemoveFileSpec(pszPath);
	::PathAppend(pszPath, TEXT("TVTestVideoDecoder.ax"));
	return true;
}


}	// namespace LibISDB::DirectShow
