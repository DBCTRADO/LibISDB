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
 @file   VideoParser.cpp
 @brief  映像解析
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoParser.hpp"
#include <numeric>	// for std::gcd()
#include "../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{


VideoParser::VideoParser()
	: m_pStreamCallback(nullptr)
	, m_AttachMediaType(false)
{
}


VideoParser::~VideoParser()
{
}


bool VideoParser::GetVideoInfo(VideoInfo *pInfo) const
{
	if (!pInfo)
		return false;

	CAutoLock Lock(&m_ParserLock);

	*pInfo = m_VideoInfo;

	return true;
}


void VideoParser::ResetVideoInfo()
{
	CAutoLock Lock(&m_ParserLock);

	m_VideoInfo.Reset();
}


bool VideoParser::AddEventListener(EventListener *pEventListener)
{
	CAutoLock Lock(&m_ParserLock);

	return m_EventListenerList.AddEventListener(pEventListener);
}


bool VideoParser::RemoveEventListener(EventListener *pEventListener)
{
	CAutoLock Lock(&m_ParserLock);

	return m_EventListenerList.RemoveEventListener(pEventListener);
}


void VideoParser::SetStreamCallback(StreamCallback *pCallback)
{
	CAutoLock Lock(&m_ParserLock);

	m_pStreamCallback = pCallback;
}


void VideoParser::SetAttachMediaType(bool Attach)
{
	CAutoLock Lock(&m_ParserLock);

	m_AttachMediaType = Attach;
}


void VideoParser::NotifyVideoInfo() const
{
	m_EventListenerList.CallEventListener(&EventListener::OnVideoInfoUpdated, m_VideoInfo);
}


bool VideoParser::SARToDAR(
	int SARX, int SARY, int Width, int Height,
	int *pDARX, int *pDARY)
{
	int DispWidth = Width * SARX, DispHeight = Height * SARY;
	int Denom = std::gcd(DispWidth, DispHeight);

	if (Denom != 0) {
		int DARX = DispWidth / Denom, DARY = DispHeight / Denom;
		*pDARX = DARX;
		*pDARY = DARY;
		return true;
	}

	return false;
}


}	// namespace LibISDB::DirectShow
