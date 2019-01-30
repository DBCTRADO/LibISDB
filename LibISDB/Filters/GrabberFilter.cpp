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
 @file   GrabberFilter.cpp
 @brief  データ取り込みフィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "GrabberFilter.hpp"
#include <algorithm>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


void GrabberFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	for (auto e : m_GrabberList)
		e->OnReset();
}


bool GrabberFilter::ReceiveData(DataStream *pData)
{
	BlockLock Lock(m_FilterLock);

	do {
		DataBuffer *pBuffer = pData->GetData();
		bool Filtered = false;

		for (auto e : m_GrabberList) {
			if (!e->ReceiveData(pBuffer))
				Filtered = true;
		}

		if (!Filtered)
			m_OutputSequence.push_back(pBuffer);
	} while (pData->Next());

	if (!m_OutputSequence.empty()) {
		OutputData(m_OutputSequence);
		m_OutputSequence.clear();
	}

	return true;
}


bool GrabberFilter::AddGrabber(Grabber *pGrabber)
{
	if (LIBISDB_TRACE_ERROR_IF(pGrabber == nullptr))
		return false;

	BlockLock Lock(m_FilterLock);

	auto it = std::find(m_GrabberList.begin(), m_GrabberList.end(), pGrabber);
	if (it != m_GrabberList.end())
		return false;

	m_GrabberList.push_back(pGrabber);

	return true;
}


bool GrabberFilter::RemoveGrabber(Grabber *pGrabber)
{
	BlockLock Lock(m_FilterLock);

	auto it = std::find(m_GrabberList.begin(), m_GrabberList.end(), pGrabber);
	if (it == m_GrabberList.end())
		return false;

	m_GrabberList.erase(it);

	return true;
}


}	// namespace LibISDB
