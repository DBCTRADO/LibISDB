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
 @file   VideoRenderer_OverlayMixer.cpp
 @brief  Overlay Mixer 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_OverlayMixer.hpp"
#include "../DirectShowUtilities.hpp"
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


bool VideoRenderer_OverlayMixer::Initialize(
	IGraphBuilder *pGraphBuilder, IPin *pInputPin, HWND hwndRender, HWND hwndMessageDrain)
{
	if (pGraphBuilder == nullptr) {
		SetHRESULTError(E_POINTER);
		return false;
	}

	HRESULT hr;

	hr = ::CoCreateInstance(
		CLSID_OverlayMixer, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(m_Renderer.GetPP()));
	if (FAILED(hr)) {
		SetHRESULTError(hr, LIBISDB_STR("Overlay Mixer のインスタンスを作成できません。"));
		return false;
	}
	hr = pGraphBuilder->AddFilter(m_Renderer.Get(), L"Overlay Mixer");
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("OverlayMixer をフィルタグラフに追加できません。"));
		return false;
	}

	hr = ::CoCreateInstance(
		CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(m_CaptureGraphBuilder2.GetPP()));
	if (FAILED(hr)) {
		m_Renderer.Release();
		SetHRESULTError(hr, LIBISDB_STR("Capture Graph Builder2 のインスタンスを作成できません。"));
		return false;
	}
	hr = m_CaptureGraphBuilder2->SetFiltergraph(pGraphBuilder);
	if (FAILED(hr)) {
		m_Renderer.Release();
		m_CaptureGraphBuilder2.Release();
		SetHRESULTError(hr, LIBISDB_STR("Capture Graph Builder2 にフィルタグラフを設定できません。"));
		return false;
	}
	hr = m_CaptureGraphBuilder2->RenderStream(nullptr, nullptr, pInputPin, m_Renderer.Get(), nullptr);
	if (FAILED(hr)) {
		m_Renderer.Release();
		m_CaptureGraphBuilder2.Release();
		SetHRESULTError(hr, LIBISDB_STR("映像レンダラを構築できません。"));
		return false;
	}

	if (!InitializeBasicVideo(pGraphBuilder, hwndRender, hwndMessageDrain)) {
		m_Renderer.Release();
		m_CaptureGraphBuilder2.Release();
		return false;
	}

	ResetError();

	return true;
}


bool VideoRenderer_OverlayMixer::Finalize()
{
	VideoRenderer_Default::Finalize();

	m_CaptureGraphBuilder2.Release();

	return true;
}


}	// namespace LibISDB::DirectShow
