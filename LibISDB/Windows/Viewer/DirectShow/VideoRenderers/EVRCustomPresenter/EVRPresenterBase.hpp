/*
  LibISDB
  Copyright(c) 2017 DBCTRADO

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
 @file   EVRPresenterBase.hpp
 @brief  EVR プレゼンタ基本ヘッダ
 @author DBCTRADO
*/


#ifndef LIBISDB_EVR_PRESENTER_BASE_H
#define LIBISDB_EVR_PRESENTER_BASE_H


#include <cmath>
#include <new>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <evr.h>
#include <evr9.h>
#include <evcode.h>

#include "../../DirectShowUtilities.hpp"
#include "EVRHelpers.hpp"
#include "EVRMediaType.hpp"

#pragma comment(lib, "evr.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dxva2.lib")


// {CB9FCC04-4247-47A8-9B55-4CCB1D2CC95B}
constexpr GUID SampleAttribute_Counter = {
	0xCB9FCC04, 0x4247, 0x47A8, {0x9B, 0x55, 0x4C, 0xCB, 0x1D, 0x2C, 0xC9, 0x5B}
};

// {99F1B32D-B439-424A-B2E9-8877C6E0DDFD}
constexpr GUID SampleAttribute_SwapChain = {
	0x99F1B32D, 0xB439, 0x424A, {0xB2, 0xE9, 0x88, 0x77, 0xC6, 0xE0, 0xDD, 0xFD}
};


#endif	// ifndef LIBISDB_EVR_PRESENTER_BASE_H
