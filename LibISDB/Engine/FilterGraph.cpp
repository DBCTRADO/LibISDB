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
 @file   FilterGraph.cpp
 @brief  フィルタグラフ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "FilterGraph.hpp"
#include <algorithm>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


FilterGraph::FilterGraph()
	: m_CurID(0)
{
}


bool FilterGraph::ConnectFilters(const ConnectionInfo *pConnectionList, size_t ConnectionCount)
{
	if (LIBISDB_TRACE_ERROR_IF((pConnectionList == nullptr) || (ConnectionCount == 0)))
		return false;

	m_ConnectionList.clear();
	m_ConnectionList.reserve(ConnectionCount);

	for (size_t i = 0; i < ConnectionCount; i++) {
		const ConnectionInfo &Info = pConnectionList[i];

		FilterBase *pUpstreamFilter = GetFilterByID(Info.UpstreamFilterID);
		FilterBase *pDownstreamFilter = GetFilterByID(Info.DownstreamFilterID);
		if (LIBISDB_TRACE_ERROR_IF((pUpstreamFilter == nullptr) || (pDownstreamFilter == nullptr)))
			return false;

		FilterSink *pSink = pDownstreamFilter->GetInputSink(Info.SinkIndex);
		if (LIBISDB_TRACE_ERROR_IF(pSink == nullptr))
			return false;

		pUpstreamFilter->SetOutputFilter(pDownstreamFilter, pSink, Info.OutputIndex);

		m_ConnectionList.push_back(Info);

		LIBISDB_TRACE(
			LIBISDB_STR("Filter connected : %") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR(" [%d] -> %") LIBISDB_STR(LIBISDB_PRIS) LIBISDB_STR("\n"),
			pUpstreamFilter->GetObjectName(),
			Info.OutputIndex,
			pDownstreamFilter->GetObjectName());
	}

	return true;
}


void FilterGraph::DisconnectFilters()
{
	for (auto &e : m_FilterList)
		e.Filter->ResetOutputFilters();
}


FilterGraph::IDType FilterGraph::RegisterFilter(FilterBase *pFilter)
{
	if (LIBISDB_TRACE_ERROR_IF(pFilter == nullptr))
		return 0;

	if (IsFilterRegistered(pFilter))
		return 0;

	IDType ID = ++m_CurID;

	m_FilterList.emplace_back(pFilter, ID);

	return ID;
}


bool FilterGraph::UnregisterFilter(FilterBase *pFilter, bool Delete)
{
	auto it = std::find_if(
		m_FilterList.begin(), m_FilterList.end(),
		[pFilter](auto &e) -> bool { return e.Filter.get() == pFilter; });
	if (it == m_FilterList.end())
		return false;

	if (!Delete)
		it->Filter.release();

	m_FilterList.erase(it);

	return true;
}


void FilterGraph::UnregisterAllFilters()
{
	m_FilterList.clear();
}


bool FilterGraph::IsFilterRegistered(const FilterBase *pFilter) const
{
	for (auto &e : m_FilterList) {
		if (e.Filter.get() == pFilter)
			return true;
	}
	return false;
}


FilterGraph::IDType FilterGraph::GetFilterID(const FilterBase *pFilter) const
{
	for (auto &e : m_FilterList) {
		if (e.Filter.get() == pFilter)
			return e.ID;
	}
	return 0;
}


size_t FilterGraph::GetFilterCount() const
{
	return m_FilterList.size();
}


FilterBase * FilterGraph::GetFilterByIndex(size_t Index) const
{
	if (Index >= m_FilterList.size())
		return nullptr;

	return m_FilterList[Index].Filter.get();
}


FilterBase * FilterGraph::GetFilterByID(IDType ID) const
{
	auto it = std::find_if(
		m_FilterList.begin(), m_FilterList.end(),
		[ID](auto &e) -> bool { return e.ID == ID; });
	if (it == m_FilterList.end())
		return nullptr;

	return it->Filter.get();
}


FilterBase * FilterGraph::GetFilterByTypeID(const std::type_info &Type) const
{
	const FilterInfo *pInfo = GetFilterInfoByTypeID(Type);
	if (pInfo == nullptr)
		return nullptr;

	return pInfo->Filter.get();
}


FilterBase * FilterGraph::GetFilterByTypeIndex(const std::type_index &Type) const
{
	auto it = std::find_if(
		m_FilterList.begin(), m_FilterList.end(),
		[&Type](auto &e) -> bool { return std::type_index(typeid(*e.Filter)) == Type; });
	if (it == m_FilterList.end())
		return nullptr;

	return it->Filter.get();
}


FilterBase * FilterGraph::GetRootFilter() const
{
	for (auto &Connection : m_ConnectionList) {
		const IDType ID = Connection.UpstreamFilterID;
		bool Connected = false;

		for (auto &e : m_ConnectionList) {
			if (e.DownstreamFilterID == ID) {
				Connected = true;
				break;
			}
		}

		if (!Connected)
			return GetFilterByID(ID);
	}

	return nullptr;
}


bool FilterGraph::ConnectFilter(IDType ID, ConnectDirection Direction)
{
	FilterBase *pFilter = GetFilterByID(ID);
	if (pFilter == nullptr)
		return false;

	if (!!(Direction & ConnectDirection::Upstream)) {
		for (auto const &e : m_ConnectionList) {
			if (e.DownstreamFilterID == ID) {
				FilterBase *pUpstreamFilter = GetFilterByID(e.UpstreamFilterID);
				if (pUpstreamFilter != nullptr) {
					pUpstreamFilter->SetOutputFilter(pFilter, pFilter->GetInputSink(e.SinkIndex), e.OutputIndex);
				}
			}
		}
	}

	if (!!(Direction & ConnectDirection::Downstream)) {
		for (auto const &e : m_ConnectionList) {
			if (e.UpstreamFilterID == ID) {
				FilterBase *pDownstreamFilter = GetFilterByID(e.DownstreamFilterID);
				if (pDownstreamFilter != nullptr) {
					pFilter->SetOutputFilter(pDownstreamFilter, pDownstreamFilter->GetInputSink(e.SinkIndex), e.OutputIndex);
				}
			}
		}
	}

	return true;
}


bool FilterGraph::DisconnectFilter(IDType ID, ConnectDirection Direction)
{
	FilterBase *pFilter = GetFilterByID(ID);
	if (pFilter == nullptr)
		return false;

	if (!!(Direction & ConnectDirection::Upstream)) {
		for (auto const &e : m_ConnectionList) {
			if (e.DownstreamFilterID == ID) {
				FilterBase *pUpstreamFilter = GetFilterByID(e.UpstreamFilterID);
				if (pUpstreamFilter != nullptr) {
					pUpstreamFilter->SetOutputFilter(nullptr, nullptr, e.OutputIndex);
				}
			}
		}
	}

	if (!!(Direction & ConnectDirection::Downstream)) {
		pFilter->ResetOutputFilters();
	}

	return true;
}


const FilterGraph::ConnectionInfo * FilterGraph::GetConnectionInfoByUpstreamID(IDType ID) const
{
	for (auto &e : m_ConnectionList) {
		if (e.UpstreamFilterID == ID)
			return &e;
	}
	return nullptr;
}


const FilterGraph::ConnectionInfo * FilterGraph::GetConnectionInfoByDownstreamID(IDType ID) const
{
	for (auto &e : m_ConnectionList) {
		if (e.DownstreamFilterID == ID)
			return &e;
	}
	return nullptr;
}


const FilterGraph::FilterInfo * FilterGraph::GetFilterInfoByTypeID(const std::type_info &Type) const
{
	auto it = std::find_if(
		m_FilterList.begin(), m_FilterList.end(),
		[&Type](auto &e) -> bool { return typeid(*e.Filter) == Type; });
	if (it == m_FilterList.end())
		return nullptr;

	return &*it;
}


}	// namespace LibISDB
