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
 @file   VideoRenderer_EVRCustomPresenter.cpp
 @brief  EVR Custom Presenter 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../../LibISDBPrivate.hpp"
#include "../../../../../LibISDBWindows.hpp"
#include "VideoRenderer_EVRCustomPresenter.hpp"
#include "EVRPresenterBase.hpp"
#include "EVRPresenter.hpp"
#include "../../DirectShowUtilities.hpp"
#include "../../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


bool VideoRenderer_EVRCustomPresenter::Finalize()
{
	m_Presenter.Release();

	return VideoRenderer_EVR::Finalize();
}


HRESULT VideoRenderer_EVRCustomPresenter::InitializePresenter(IBaseFilter *pFilter)
{
	HRESULT hr;
	IMFVideoRenderer *pRenderer;

	hr = pFilter->QueryInterface(IID_PPV_ARGS(&pRenderer));

	if (SUCCEEDED(hr)) {
		IMFVideoPresenter *pPresenter;

		hr = EVRPresenter::CreateInstance(nullptr, IID_PPV_ARGS(&pPresenter));
		if (SUCCEEDED(hr)) {
			hr = pRenderer->InitializeRenderer(nullptr, pPresenter);
			if (SUCCEEDED(hr)) {
				m_Presenter.Attach(pPresenter);
			} else {
				pPresenter->Release();
			}
		}

		pRenderer->Release();
	}

	return hr;
}


}	// namespace LibISDB::DirectShow
