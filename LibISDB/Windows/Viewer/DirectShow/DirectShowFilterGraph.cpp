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
 @file   DirectShowFilterGraph.cpp
 @brief  DirectShow フィルタグラフ
 @author DBCTRADO
*/


#include "../../../LibISDBPrivate.hpp"
#include "../../../LibISDBWindows.hpp"
#include "DirectShowFilterGraph.hpp"
#include "../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


FilterGraph::~FilterGraph()
{
	DestroyGraphBuilder();
}


bool FilterGraph::Play()
{
	bool Result = false;

	if (m_MediaControl) {
		if (m_MediaControl->Run() == S_OK) {
			Result = true;
		} else {
			for (int i = 0; i < 20; i++) {
				OAFilterState fs;

				if ((m_MediaControl->GetState(100, &fs) == S_OK) && (fs == State_Running)) {
					Result = true;
					break;
				}
			}
		}
	}

	return Result;
}


bool FilterGraph::Stop()
{
	bool Result = false;

	if (m_MediaControl) {
		if (m_MediaControl->Stop() == S_OK)
			Result = true;
	}

	return Result;
}


bool FilterGraph::Pause()
{
	bool Result = false;

	if (m_MediaControl) {
		if (m_MediaControl->Pause() == S_OK) {
			Result = true;
		} else {
			for (int i = 0; i < 20; i++) {
				OAFilterState fs;
				HRESULT hr = m_MediaControl->GetState(100, &fs);

				if ((hr == S_OK || hr == VFW_S_CANT_CUE) && (fs == State_Paused)) {
					Result = true;
					break;
				}
			}
		}
	}

	return Result;
}


bool FilterGraph::SetVolume(float Volume)
{
	// 音量をdBで設定する( -100.0(無音) <= Volume <= 0(最大) )
	bool Result = false;

	if (m_GraphBuilder) {
		IBasicAudio *pBasicAudio;

		if (SUCCEEDED(m_GraphBuilder.QueryInterface(&pBasicAudio))) {
			const long lVolume = std::clamp(static_cast<long>(Volume * 100.0f), -10000L, 0L);

			LIBISDB_TRACE(LIBISDB_STR("Volume = %ld\n"), lVolume);
			if (SUCCEEDED(pBasicAudio->put_Volume(lVolume)))
				Result = true;
			pBasicAudio->Release();
		}
	}

	return Result;
}


float FilterGraph::GetVolume() const
{
	float Volume = -1.0f;

	if (m_GraphBuilder) {
		IBasicAudio *pBasicAudio;

		if (SUCCEEDED(m_GraphBuilder.QueryInterface(&pBasicAudio))) {
			long lVolume;

			if (SUCCEEDED(pBasicAudio->get_Volume(&lVolume)))
				Volume = (float)lVolume / 100.0f;
			pBasicAudio->Release();
		}
	}

	return Volume;
}


HRESULT FilterGraph::CreateGraphBuilder()
{
	HRESULT hr;

	hr = ::CoCreateInstance(
		CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(m_GraphBuilder.GetPP()));
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_GraphBuilder.QueryInterface(&m_MediaControl);
	if (FAILED(hr)) {
		m_GraphBuilder.Release();
		return hr;
	}

#ifdef LIBISDB_DEBUG
	DirectShow::AddToROT(m_GraphBuilder.Get(), &m_Register);
#endif

	return S_OK;
}


void FilterGraph::DestroyGraphBuilder()
{
	m_MediaControl.Release();

#ifdef LIBISDB_DEBUG
	if (m_Register != 0){
		DirectShow::RemoveFromROT(m_Register);
		m_Register = 0;
	}
#endif

	m_GraphBuilder.Release();
}


}	// namespace LibISDB::DirectShow
