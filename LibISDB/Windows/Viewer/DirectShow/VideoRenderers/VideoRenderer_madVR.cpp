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
 @file   VideoRenderer_madVR.cpp
 @brief  madVR 映像レンダラ
 @author DBCTRADO
*/


#include "../../../../LibISDBPrivate.hpp"
#include "../../../../LibISDBWindows.hpp"
#include "VideoRenderer_madVR.hpp"
#include "../../../../Base/DebugDef.hpp"


static const CLSID CLSID_madVR = {0xe1a8b82a, 0x32ce, 0x4b0d, {0xbe, 0x0d, 0xaa, 0x68, 0xc7, 0x72, 0xe4, 0x23}};


namespace LibISDB::DirectShow
{


VideoRenderer_madVR::VideoRenderer_madVR()
	: VideoRenderer_Basic(CLSID_madVR, LIBISDB_STR("madVR"), true)
{
}


const CLSID & VideoRenderer_madVR::GetCLSID()
{
	return CLSID_madVR;
}


}	// namespace LibISDB::DirectShow
