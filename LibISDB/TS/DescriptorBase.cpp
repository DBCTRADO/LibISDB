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
 @file   DescriptorBase.cpp
 @brief  記述子基底クラス
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "DescriptorBase.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


DescriptorBase::DescriptorBase()
	: m_Tag(0)
	, m_Length(0)
	, m_IsValid(false)
{
}


void DescriptorBase::CopyFrom(const DescriptorBase *pSrc)
{
	*this = *pSrc;
}


DescriptorBase * DescriptorBase::Clone() const
{
	return new DescriptorBase(*this);
}


bool DescriptorBase::Parse(const uint8_t *pData, uint16_t DataLength)
{
	Reset();

	if (pData == nullptr)
		return false;
	if (DataLength < 2)
		return false;
	if (DataLength < static_cast<uint16_t>(pData[1] + 2))
		return false;

	m_Tag    = pData[0];
	m_Length = pData[1];

	if ((m_Length > 0) && StoreContents(&pData[2]))
		m_IsValid = true;

	return m_IsValid;
}


void DescriptorBase::Reset()
{
	m_Tag = 0;
	m_Length = 0;
	m_IsValid = false;
}


bool DescriptorBase::StoreContents(const uint8_t *pPayload)
{
	return true;
}


}	// namespace LibISDB
