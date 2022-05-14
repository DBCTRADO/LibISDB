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
 @file   LibISDBVersion.hpp
 @brief  LibISDB バージョン
 @author DBCTRADO
*/


#ifndef LIBISDB_VERSION_H
#define LIBISDB_VERSION_H


#define LIBISDB_VERSION_MAJOR 0
#define LIBISDB_VERSION_MINOR 1
#define LIBISDB_VERSION_REV   0

#define LIBISDB_MAKE_VERSION(major, minor, rev) \
	(((major) << 24) | ((minor) << 12) | (rev))
#define LIBISDB_VERSION \
	LIBISDB_MAKE_VERSION(LIBISDB_VERSION_MAJOR, LIBISDB_VERSION_MINOR, LIBISDB_VERSION_REV)

#define LIBISDB_MAKE_STRING_(s) LIBISDB_STR(#s)
#define LIBISDB_MAKE_STRING(s) LIBISDB_MAKE_STRING_(s)

#define LIBISDB_VERSION_STRING \
	LIBISDB_MAKE_STRING(LIBISDB_VERSION_MAJOR) \
	LIBISDB_STR(".") \
	LIBISDB_MAKE_STRING(LIBISDB_VERSION_MINOR) \
	LIBISDB_STR(".") \
	LIBISDB_MAKE_STRING(LIBISDB_VERSION_REV) \

#if __has_include("LibISDBVersionHash.hpp")
#include "LibISDBVersionHash.hpp"
#define LIBISDB_VERSION_HASH LIBISDB_STR(LIBISDB_VERSION_HASH_)
#endif


#endif	// ifndef LIBISDB_VERSION_H
