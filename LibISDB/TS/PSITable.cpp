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
 @file   PSITable.cpp
 @brief  PSI テーブル
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "PSITable.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


PSITableBase::PSITableBase(bool ExtendedSection, bool IgnoreSectionNumber)
	: m_PSISectionParser(this, ExtendedSection, IgnoreSectionNumber)
	, m_UniqueID(0)
{
}


void PSITableBase::Reset()
{
	m_PSISectionParser.Reset();
}


unsigned long PSITableBase::GetCRCErrorCount() const
{
	return m_PSISectionParser.GetCRCErrorCount();
}


void PSITableBase::SetSectionHandler(const SectionHandler &Handler)
{
	m_SectionHandler = Handler;
}


bool PSITableBase::StorePacket(const TSPacket *pPacket)
{
	if (pPacket == nullptr)
		return false;

	m_PSISectionParser.StorePacket(pPacket);

	return true;
}


void PSITableBase::OnPIDUnmapped(uint16_t PID)
{
	delete this;
}




PSITable::PSITable()
	: m_LastUpdatedSectionIndex(-1)
	, m_LastUpdatedSectionNumber(0xFFFF_u16)
{
}


void PSITable::Reset()
{
	PSITableBase::Reset();

	m_TableList.clear();

	m_LastUpdatedSectionIndex = -1;
	m_LastUpdatedSectionNumber = 0xFFFF_u16;
}


int PSITable::GetTableCount() const
{
	return static_cast<int>(m_TableList.size());
}


bool PSITable::GetTableID(int Index, uint8_t *pTableID) const
{
	if ((Index < 0) || (Index >= GetTableCount()) || (pTableID == nullptr))
		return false;

	*pTableID = m_TableList[Index].TableID;

	return true;
}


bool PSITable::GetTableUniqueID(int Index, unsigned long long *pUniqueID) const
{
	if ((Index < 0) || (Index >= GetTableCount()) || (pUniqueID == nullptr))
		return false;

	*pUniqueID = m_TableList[Index].UniqueID;

	return true;
}


int PSITable::GetTableIndexByTableID(uint8_t TableID) const
{
	for (size_t Index = 0; Index < m_TableList.size(); Index++) {
		if (m_TableList[Index].TableID == TableID)
			return static_cast<int>(Index);
	}

	return -1;
}


int PSITable::GetTableIndexByTableIDs(uint8_t TableID, unsigned long long UniqueID) const
{
	for (size_t Index = 0; Index < m_TableList.size(); Index++) {
		if ((m_TableList[Index].TableID == TableID)
				&& (m_TableList[Index].UniqueID == UniqueID))
			return static_cast<int>(Index);
	}

	return -1;
}


int PSITable::GetTableIndexByUniqueID(unsigned long long UniqueID) const
{
	for (size_t Index = 0; Index < m_TableList.size(); Index++) {
		if (m_TableList[Index].UniqueID == UniqueID)
			return static_cast<int>(Index);
	}

	return -1;
}


uint16_t PSITable::GetSectionCount(int Index) const
{
	if ((Index < 0) || (Index >= GetTableCount()))
		return 0;

	return m_TableList[Index].LastSectionNumber + 1;
}


const PSITableBase * PSITable::GetSection(int Index, uint16_t SectionNumber) const
{
	if ((Index < 0) || (Index >= GetTableCount()))
		return nullptr;
	if (SectionNumber > m_TableList[Index].LastSectionNumber)
		return nullptr;

	return m_TableList[Index].SectionList[SectionNumber].Table.get();
}


const PSITableBase * PSITable::GetLastUpdatedSection() const
{
	if ((m_LastUpdatedSectionIndex < 0) || (static_cast<size_t>(m_LastUpdatedSectionIndex) >= m_TableList.size()))
		return nullptr;
	if (m_LastUpdatedSectionNumber > m_TableList[m_LastUpdatedSectionIndex].LastSectionNumber)
		return nullptr;

	return m_TableList[m_LastUpdatedSectionIndex].SectionList[m_LastUpdatedSectionNumber].Table.get();
}


bool PSITable::ResetTable(int Index)
{
	if ((Index < 0) || (Index >= GetTableCount()))
		return false;

	for (auto &Section : m_TableList[Index].SectionList) {
		Section.Table.reset();
		Section.IsUpdated = false;
	}

	return true;
}


bool PSITable::ResetSection(int Index, uint16_t SectionNumber)
{
	if ((Index < 0) || (Index >= GetTableCount()))
		return false;

	TableItem &Table = m_TableList[Index];

	if (SectionNumber > Table.LastSectionNumber)
		return false;

	TableItem::SectionItem &Section = Table.SectionList[SectionNumber];

	Section.Table.reset();
	Section.IsUpdated = false;

	return true;
}


bool PSITable::IsSectionComplete(int Index, uint16_t LastSectionNumber) const
{
	if ((Index < 0) || (Index >= GetTableCount()))
		return false;

	const TableItem &Table = m_TableList[Index];

	for (size_t i = 0; (i < Table.SectionList.size()) && (i <= LastSectionNumber); i++) {
		const TableItem::SectionItem &Section = Table.SectionList[i];

		if (!Section.Table || !Section.IsUpdated)
			return false;
	}

	return true;
}


bool PSITable::OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection)
{
	if (pSection->GetSectionNumber() > pSection->GetLastSectionNumber())
		return false;

	if (pSection->GetPayloadSize() == 0)
		return false;

	if (!pSection->GetCurrentNextIndicator())
		return false;

	// ユニークIDを検索する
	const unsigned long long UniqueID = GetSectionTableUniqueID(pSection);
	int Index = GetTableIndexByUniqueID(UniqueID);

	if (Index < 0) {
		// 新規テーブルの追加
		Index = static_cast<int>(m_TableList.size());
		m_TableList.emplace_back();
		m_TableList[Index].UniqueID = UniqueID;
		m_TableList[Index].TableID = pSection->GetTableID();
		m_TableList[Index].LastSectionNumber = pSection->GetLastSectionNumber();
		m_TableList[Index].VersionNumber = pSection->GetVersionNumber();
		m_TableList[Index].SectionList.resize(m_TableList[Index].LastSectionNumber + 1);
	} else if ((m_TableList[Index].VersionNumber != pSection->GetVersionNumber())
			|| (m_TableList[Index].LastSectionNumber != pSection->GetLastSectionNumber())) {
		// テーブルが更新された
		m_TableList[Index].LastSectionNumber = pSection->GetLastSectionNumber();
		m_TableList[Index].VersionNumber = pSection->GetVersionNumber();
		m_TableList[Index].SectionList.clear();
		m_TableList[Index].SectionList.resize(m_TableList[Index].LastSectionNumber + 1);
	}

	// セクションデータを更新する
	const uint16_t SectionNumber = pSection->GetSectionNumber();
	TableItem::SectionItem &Section = m_TableList[Index].SectionList[SectionNumber];
	if (!Section.Table) {
		Section.Table.reset(CreateSectionTable(pSection));
		if (!Section.Table)
			return false;
		Section.Table->SetUniqueID(UniqueID);
	}

	if (!Section.Table->OnPSISection(pSectionParser, pSection))
		return false;

	Section.IsUpdated = true;

	m_LastUpdatedSectionIndex = Index;
	m_LastUpdatedSectionNumber = SectionNumber;

	if (m_SectionHandler)
		m_SectionHandler(this, pSection);

	return true;
}


unsigned long long PSITable::GetSectionTableUniqueID(const PSISection *pSection)
{
	return pSection->GetTableIDExtension();
}




PSISingleTable::PSISingleTable(bool ExtendedSection)
	: PSITableBase(ExtendedSection)
{
}


void PSISingleTable::Reset()
{
	PSITableBase::Reset();
	m_CurSection.Reset();
}


bool PSISingleTable::OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection)
{
	if (*pSection != m_CurSection) {
		// セクションが更新された
		if (OnTableUpdate(pSection, &m_CurSection)) {
			m_CurSection = *pSection;
			if (m_SectionHandler)
				m_SectionHandler(this, pSection);
			return true;
		}
	}

	return false;
}


bool PSISingleTable::OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection)
{
	return true;
}




PSIStreamTable::PSIStreamTable(bool ExtendedSection, bool IgnoreSectionNumber)
	: PSITableBase(ExtendedSection, IgnoreSectionNumber)
{
}


void PSIStreamTable::Reset()
{
	PSITableBase::Reset();
}


bool PSIStreamTable::OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection)
{
	if (OnTableUpdate(pSection)) {
		if (m_SectionHandler)
			m_SectionHandler(this, pSection);
		return true;
	}

	return false;
}


bool PSIStreamTable::OnTableUpdate(const PSISection *pCurSection)
{
	return true;
}




void PSINullTable::OnPIDUnmapped(uint16_t PID)
{
	delete this;
}




PSITableSet::PSITableSet(bool ExtendedSection)
	: PSITableBase(ExtendedSection)
	, m_LastUpdatedTableID(0xFF_u8)
	, m_LastUpdatedSectionNumber(0xFF_u8)
	, m_LastUpdatedTableUniqueID(0)
{
}


PSITableSet::~PSITableSet()
{
	UnmapAllTables();
}


void PSITableSet::Reset()
{
	PSITableBase::Reset();

	for (auto &e : m_TableMap)
		e.second->Reset();

	m_LastUpdatedTableID = 0xFF_u8;
	m_LastUpdatedSectionNumber = 0xFF_u8;
	m_LastUpdatedTableUniqueID = 0;
}


bool PSITableSet::MapTable(uint8_t TableID, PSITableBase *pTable)
{
	if (pTable == nullptr)
		return false;

	UnmapTable(TableID);

	m_TableMap.emplace(TableID, pTable);

	return true;
}


bool PSITableSet::UnmapTable(uint8_t TableID)
{
	auto it = m_TableMap.find(TableID);

	if (it == m_TableMap.end())
		return false;

	m_TableMap.erase(it);

	return true;
}


void PSITableSet::UnmapAllTables()
{
	m_TableMap.clear();
}


PSITableBase * PSITableSet::GetTableByID(uint8_t TableID)
{
	auto it = m_TableMap.find(TableID);

	if (it == m_TableMap.end())
		return nullptr;

	return it->second.get();
}


const PSITableBase * PSITableSet::GetTableByID(uint8_t TableID) const
{
	auto it = m_TableMap.find(TableID);

	if (it == m_TableMap.end())
		return nullptr;

	return it->second.get();
}


const PSITableBase * PSITableSet::GetLastUpdatedTable() const
{
	return GetTableByID(m_LastUpdatedTableID);
}


bool PSITableSet::OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection)
{
	PSITableBase *pTable = GetTableByID(pSection->GetTableID());

	if (pTable != nullptr) {
		if (pTable->OnPSISection(pSectionParser, pSection)) {
			m_LastUpdatedTableID = pSection->GetTableID();
			m_LastUpdatedSectionNumber = pSection->GetSectionNumber();
			m_LastUpdatedTableUniqueID = 0;

			const PSITable *pPSITable = dynamic_cast<const PSITable *>(pTable);
			if (pPSITable != nullptr) {
				const PSITableBase *pSection = pPSITable->GetLastUpdatedSection();
				if (pSection != nullptr)
					m_LastUpdatedTableUniqueID = pSection->GetUniqueID();
			}

			if (m_SectionHandler)
				m_SectionHandler(this, pSection);
		}
	}

	return false;
}


}	// namespace LibISDB
