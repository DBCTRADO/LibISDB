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
 @file   EVRHelpers.cpp
 @brief  EVR ヘルパ
 @author DBCTRADO
*/


#include "../../../../../LibISDBPrivate.hpp"
#include "../../../../../LibISDBWindows.hpp"
#include "EVRPresenterBase.hpp"
#include "EVRHelpers.hpp"
#include "../../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


SamplePool::SamplePool()
	: m_Initialized(false)
	, m_PendingCount(0)
{
}


SamplePool::~SamplePool()
{
}


HRESULT SamplePool::GetSample(IMFSample **ppSample)
{
	*ppSample = nullptr;

	BlockLock Lock(m_Lock);

	if (!m_Initialized) {
		return MF_E_NOT_INITIALIZED;
	}

	if (m_VideoSampleQueue.IsEmpty()) {
		return MF_E_SAMPLEALLOCATOR_EMPTY;
	}

	IMFSample *pSample = nullptr;

	HRESULT hr = m_VideoSampleQueue.RemoveFront(&pSample);

	if (SUCCEEDED(hr)) {
		m_PendingCount++;

		*ppSample = pSample;
	}

	return hr;
}


HRESULT SamplePool::ReturnSample(IMFSample *pSample)
{
	BlockLock Lock(m_Lock);

	LIBISDB_ASSERT(m_PendingCount > 0);

	if (!m_Initialized) {
		return MF_E_NOT_INITIALIZED;
	}

	HRESULT hr = m_VideoSampleQueue.InsertBack(pSample);

	if (SUCCEEDED(hr)) {
		m_PendingCount--;
	}

	return hr;
}


bool SamplePool::AreSamplesPending()
{
	BlockLock Lock(m_Lock);

	if (!m_Initialized) {
		return false;
	}

	return m_PendingCount > 0;
}


HRESULT SamplePool::Initialize(VideoSampleList &Samples)
{
	BlockLock Lock(m_Lock);

	if (m_Initialized) {
		return MF_E_INVALIDREQUEST;
	}

	HRESULT hr = S_OK;

	for (VideoSampleList::Position Pos = Samples.FrontPosition();
			Pos != Samples.EndPosition();
			Pos = Samples.Next(Pos)) {
		IMFSample *pSample = nullptr;

		hr = Samples.GetItemPos(Pos, &pSample);
		if (SUCCEEDED(hr)) {
			hr = m_VideoSampleQueue.InsertBack(pSample);
			pSample->Release();
		}

		if (FAILED(hr)) {
			break;
		}
	}

	if (SUCCEEDED(hr)) {
		m_Initialized = true;
	}

	Samples.Clear();

	return hr;
}


HRESULT SamplePool::Clear()
{
	BlockLock Lock(m_Lock);

	m_VideoSampleQueue.Clear();
	m_Initialized = false;
	m_PendingCount = 0;

	return S_OK;
}


}	// namespace LibISDB::DirectShow
