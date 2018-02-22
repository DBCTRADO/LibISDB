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
 @file   SourceFilter.hpp
 @brief  ソースフィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "SourceFilter.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


SourceFilter::SourceFilter(SourceMode Mode)
	: m_SourceMode(Mode)
{
}


void SourceFilter::Finalize()
{
	CloseSource();
}


bool SourceFilter::SetSourceMode(SourceMode Mode)
{
	if (LIBISDB_TRACE_ERROR_IF((Mode != SourceMode::Push) && (Mode != SourceMode::Pull)))
		return false;
	if (LIBISDB_TRACE_ERROR_IF(!(Mode & GetAvailableSourceModes())))
		return false;

	m_SourceMode = Mode;

	return true;
}


bool SourceFilter::AddEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.AddEventListener(pEventListener);
}


bool SourceFilter::RemoveEventListener(EventListener *pEventListener)
{
	return m_EventListenerList.RemoveEventListener(pEventListener);
}


}	// namespace LibISDB
