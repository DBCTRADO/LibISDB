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
 @file   DebugDef.hpp
 @brief  デバッグ用定義
 @author DBCTRADO
*/


#ifndef LIBISDB_DEBUG_DEF_H
#define LIBISDB_DEBUG_DEF_H


#ifdef LIBISDB_DEBUG
#if defined(_MSC_VER)
#if defined(LIBISDB_DEBUG_NEW) && !defined(LIBISDB_NO_DEFINE_NEW)
#define new LIBISDB_DEBUG_NEW
#endif
#endif	// _MSC_VER
#endif	// LIBISDB_DEBUG


#endif	// ifndef LIBISDB_DEBUG_DEF_H
