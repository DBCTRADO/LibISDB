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
 @file   FilterGraph.hpp
 @brief  フィルタグラフ
 @author DBCTRADO
*/


#ifndef LIBISDB_FILTER_GRAPH_H
#define LIBISDB_FILTER_GRAPH_H


#include "../Filters/FilterBase.hpp"
#include <vector>
#include <memory>
#include <typeinfo>
#include <typeindex>


namespace LibISDB
{

	/** フィルタグラフクラス */
	class FilterGraph
	{
	public:
		typedef unsigned int IDType;

		struct ConnectionInfo {
			IDType UpstreamFilterID = 0;
			IDType DownstreamFilterID = 0;
			int SinkIndex = 0;
			int OutputIndex = 0;
		};

		enum class ConnectDirection : unsigned int {
			None       = 0x0000U,
			Upstream   = 0x0001U,
			Downstream = 0x0002U,
			Both       = 0x0003U,
		};

		FilterGraph() noexcept;

		bool ConnectFilters(const ConnectionInfo *pConnectionList, size_t ConnectionCount);
		void DisconnectFilters();
		IDType RegisterFilter(FilterBase *pFilter);
		bool UnregisterFilter(FilterBase *pFilter, bool Delete = true);
		void UnregisterAllFilters();
		bool IsFilterRegistered(const FilterBase *pFilter) const;
		IDType GetFilterID(const FilterBase *pFilter) const;
		template<typename T> IDType GetFilterID() const
		{
			const FilterInfo *pInfo = GetFilterInfoByTypeID(typeid(T));
			if (pInfo == nullptr)
				return 0;
			return pInfo->ID;
		}
		size_t GetFilterCount() const;
		FilterBase * GetFilterByIndex(size_t Index) const;
		FilterBase * GetFilterByID(IDType ID) const;
		FilterBase * GetFilterByTypeID(const std::type_info &Type) const;
		FilterBase * GetFilterByTypeIndex(const std::type_index &Type) const;
		FilterBase * GetRootFilter() const;
		template<typename T> T * GetFilter() const
		{
			for (auto &e : m_FilterList) {
				T *pFilter = dynamic_cast<T *>(e.Filter.get());
				if (pFilter != nullptr)
					return pFilter;
			}
			return nullptr;
		}
		template<typename T> T * GetFilterExplicit() const
		{
			return static_cast<T *>(GetFilterByTypeID(typeid(T)));
		}

		bool ConnectFilter(IDType ID, ConnectDirection Direction);
		bool DisconnectFilter(IDType ID, ConnectDirection Direction);
		const ConnectionInfo * GetConnectionInfoByUpstreamID(IDType ID) const;
		const ConnectionInfo * GetConnectionInfoByDownstreamID(IDType ID) const;

		template<typename TPred> void EnumFilters(TPred Pred) const
		{
			for (auto &e : m_FilterList)
				Pred(e.Filter.get());
		}

		template<typename TPred> void WalkGraph(FilterBase *pFilter, TPred Pred) const
		{
			Pred(pFilter);
			const int OutputCount = pFilter->GetOutputCount();
			for (int i = 0; i < OutputCount; i++) {
				FilterBase *pOutFilter = pFilter->GetOutputFilter(i);
				if (pOutFilter != nullptr)
					WalkGraph(pOutFilter, Pred);
			}
		}
		template<typename TPred> void WalkGraph(TPred Pred) const
		{
			WalkGraph(GetRootFilter(), Pred);
		}

	private:
		struct FilterInfo {
			std::unique_ptr<FilterBase> Filter;
			IDType ID;

			FilterInfo(FilterBase *pFilter, IDType id)
				: Filter(pFilter)
				, ID(id)
			{
			}
		};

		std::vector<FilterInfo> m_FilterList;
		std::vector<ConnectionInfo> m_ConnectionList;
		IDType m_CurID;

		const FilterInfo * GetFilterInfoByTypeID(const std::type_info &Type) const;
	};

	LIBISDB_ENUM_FLAGS(FilterGraph::ConnectDirection)

}	// namespace LibISDB


#endif	// ifndef LIBISDB_FILTER_GRAPH_H
