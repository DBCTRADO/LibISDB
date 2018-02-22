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
 @file   DirectShowFilterGraph.hpp
 @brief  DirectShow フィルタグラフ
 @author DBCTRADO
*/


#ifndef LIBISDB_DIRECT_SHOW_FILTER_GRAPH_H
#define LIBISDB_DIRECT_SHOW_FILTER_GRAPH_H


#include "DirectShowBase.hpp"
#include "DirectShowUtilities.hpp"


namespace LibISDB::DirectShow
{

	class FilterGraph
	{
	public:
		virtual ~FilterGraph();

		virtual bool Play();
		virtual bool Stop();
		virtual bool Pause();

		bool SetVolume(float Volume);
		float GetVolume() const;

	protected:
		HRESULT CreateGraphBuilder();
		void DestroyGraphBuilder();

		COMPointer<IGraphBuilder> m_GraphBuilder;
		COMPointer<IMediaControl> m_MediaControl;

#ifdef LIBISDB_DEBUG
	private:
		DWORD m_Register = 0;
#endif
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_DIRECT_SHOW_FILTER_GRAPH_H
