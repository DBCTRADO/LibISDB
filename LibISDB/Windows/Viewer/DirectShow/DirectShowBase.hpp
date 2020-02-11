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
 @file   DirectShowBase.hpp
 @brief  DirectShow 基本ヘッダ
 @author DBCTRADO
*/


#ifndef LIBISDB_DIRECTSHOW_BASE_H
#define LIBISDB_DIRECTSHOW_BASE_H


#include <dshow.h>
#include "../../../../Thirdparty/BaseClasses/streams.h"


#pragma comment(lib, "quartz.lib")

#ifdef LIBISDB_DEBUG
#pragma comment(lib, "strmbasd.lib")
#else
#pragma comment(lib, "strmbase.lib")
#endif


#endif	// ifndef LIBISDB_DIRECTSHOW_BASE_H
