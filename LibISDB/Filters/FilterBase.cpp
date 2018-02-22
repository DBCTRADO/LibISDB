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
 @file   FilterBase.cpp
 @brief  フィルタ基底クラス
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "FilterBase.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


bool FilterBase::Initialize()
{
	return true;
}


void FilterBase::Finalize()
{
}


void FilterBase::Reset()
{
}


void FilterBase::ResetGraph()
{
	BlockLock Lock(m_FilterLock);

	Reset();
	ResetDownstreamFilters();
}


bool FilterBase::StartStreaming()
{
	return StartDownstreamFilters();
}


bool FilterBase::StopStreaming()
{
	return StopDownstreamFilters();
}


bool FilterBase::OutputData(DataStream *pData, int OutputIndex)
{
	FilterSink *pSink = GetOutputSink(OutputIndex);

	if (pSink == nullptr)
		return false;

	pData->Rewind();

	return pSink->ReceiveData(pData);
}


bool FilterBase::OutputData(DataBuffer *pData, int OutputIndex)
{
	FilterSink *pSink = GetOutputSink(OutputIndex);

	if (pSink == nullptr)
		return false;

	SingleDataStream<DataBuffer> Stream(pData);

	return pSink->ReceiveData(&Stream);
}


void FilterBase::ResetDownstreamFilters()
{
	const int OutputCount = GetOutputCount();

	for (int i = 0; i < OutputCount; i++) {
		FilterBase *pFilter = GetOutputFilter(i);

		if (pFilter != nullptr)
			pFilter->ResetGraph();
	}
}


bool FilterBase::StartDownstreamFilters()
{
	const int OutputCount = GetOutputCount();
	bool Failed = false;

	for (int i = 0; i < OutputCount; i++) {
		FilterBase *pFilter = GetOutputFilter(i);

		if (pFilter != nullptr) {
			if (!pFilter->StartStreaming())
				Failed = true;
		}
	}

	return !Failed;
}


bool FilterBase::StopDownstreamFilters()
{
	const int OutputCount = GetOutputCount();
	bool Failed = false;

	for (int i = 0; i < OutputCount; i++) {
		FilterBase *pFilter = GetOutputFilter(i);

		if (pFilter != nullptr) {
			if (!pFilter->StopStreaming())
				Failed = true;
		}
	}

	return !Failed;
}




bool SingleOutputFilter::SetOutputFilter(FilterBase *pFilter, FilterSink *pSink, int Index)
{
	if (LIBISDB_TRACE_ERROR_IF(Index != 0))
		return false;

	m_OutputFilter.pFilter = pFilter;
	m_OutputFilter.pSink = pSink;

	return true;
}


void SingleOutputFilter::ResetOutputFilters()
{
	m_OutputFilter.pFilter = nullptr;
	m_OutputFilter.pSink = nullptr;
}


FilterBase * SingleOutputFilter::GetOutputFilter(int Index) const
{
	if (Index != 0)
		return nullptr;

	return m_OutputFilter.pFilter;
}


FilterSink * SingleOutputFilter::GetOutputSink(int Index) const
{
	if (Index != 0)
		return nullptr;

	return m_OutputFilter.pSink;
}




FilterSink * SingleInputFilter::GetInputSink(int Index)
{
	if (Index != 0)
		return nullptr;

	return this;
}


bool SingleInputFilter::ReceiveData(DataStream *pData)
{
	BlockLock Lock(m_FilterLock);

	ProcessData(pData);

	return true;
}




FilterSink * SingleIOFilter::GetInputSink(int Index)
{
	if (Index != 0)
		return nullptr;

	return this;
}


bool SingleIOFilter::ReceiveData(DataStream *pData)
{
	BlockLock Lock(m_FilterLock);

	ProcessData(pData);
	OutputData(pData);

	return true;
}


}	// namespace LibISDB
