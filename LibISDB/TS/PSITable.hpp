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
 @file   PSITable.hpp
 @brief  PSI テーブル
 @author DBCTRADO
*/


#ifndef LIBISDB_PSI_TABLE_H
#define LIBISDB_PSI_TABLE_H


#include "PSISection.hpp"
#include "PIDMap.hpp"
#include <vector>
#include <map>
#include <memory>
#include <functional>


namespace LibISDB
{

	/** PSI テーブル基底クラス */
	class PSITableBase
		: public PIDMapTarget
		, public PSISectionParser::PSISectionHandler
	{
	public:
		typedef std::function<void(const PSITableBase *pTable, const PSISection *pSection)> SectionHandler;

		template<typename T> static SectionHandler BindHandler(
			void (T::*pFunc)(const PSITableBase *, const PSISection *), T *pObject)
		{
			return std::bind(pFunc, pObject, std::placeholders::_1, std::placeholders::_2);
		}

		template<typename TTable, typename THandler> static PSITableBase * CreateWithHandler(
			void (THandler::*pFunc)(const PSITableBase *, const PSISection *), THandler *pObject)
		{
			TTable *pTable = new TTable;
			pTable->SetSectionHandler(BindHandler(pFunc, pObject));
			return pTable;
		}

		PSITableBase(bool ExtendedSection = true, bool IgnoreSectionNumber = false);
		virtual ~PSITableBase() = default;

		virtual void Reset();

		unsigned long GetCRCErrorCount() const noexcept;

		void SetUniqueID(unsigned long long UniqueID) noexcept { m_UniqueID = UniqueID; }
		unsigned long long GetUniqueID() const noexcept { return m_UniqueID; }

		void SetSectionHandler(const SectionHandler &Handler);

	// PIDMapTarget
		bool StorePacket(const TSPacket *pPacket) override;
		void OnPIDUnmapped(uint16_t PID) override;

	protected:
		PSISectionParser m_PSISectionParser;
		unsigned long long m_UniqueID;
		SectionHandler m_SectionHandler;
	};

	/** PSI テーブルクラス */
	class PSITable
		: public PSITableBase
	{
	public:
		PSITable();

	// PSITableBase
		void Reset() override;

	// PSITable
		int GetTableCount() const;
		bool GetTableID(int Index, uint8_t *pTableID) const;
		bool GetTableUniqueID(int Index, unsigned long long *pUniqueID) const;
		int GetTableIndexByTableID(uint8_t TableID) const;
		int GetTableIndexByTableIDs(uint8_t TableID, unsigned long long UniqueID) const;
		int GetTableIndexByUniqueID(unsigned long long UniqueID) const;
		uint16_t GetSectionCount(int Index) const;
		const PSITableBase * GetSection(int Index = 0, uint16_t SectionNumber = 0) const;
		const PSITableBase * GetLastUpdatedSection() const;
		bool ResetTable(int Index = 0);
		bool ResetSection(int Index = 0, uint16_t SectionNumber = 0);
		bool IsSectionComplete(int Index, uint16_t LastSectionNumber = 0xFFFF_u16) const;

	protected:
	// PSISectionParser::PSISectionHandler
		bool OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection) override;

		virtual PSITableBase * CreateSectionTable(const PSISection *pSection) = 0;
		virtual unsigned long long GetSectionTableUniqueID(const PSISection *pSection);

		struct TableItem
		{
			struct SectionItem
			{
				std::unique_ptr<PSITableBase> Table;
				bool IsUpdated = false;
			};

			unsigned long long UniqueID;          /**< 識別ID */
			uint8_t TableID;                      /**< テーブルID */
			uint16_t LastSectionNumber;           /**< 最終セクション番号 */
			uint8_t VersionNumber;                /**< バージョン番号 */
			std::vector<SectionItem> SectionList; /**< セクションデータ */
		};

		std::vector<TableItem> m_TableList;

		int m_LastUpdatedSectionIndex;
		uint16_t m_LastUpdatedSectionNumber;
	};

	/** 単独 PSI テーブルクラス */
	class PSISingleTable
		: public PSITableBase
	{
	public:
		PSISingleTable(bool ExtendedSection = true);

	// PSITableBase
		void Reset() override;

	// PSISingleTable
		uint8_t GetTableID() const noexcept { return m_CurSection.GetTableID(); }
		uint16_t GetTableIDExtension() const noexcept { return m_CurSection.GetTableIDExtension(); }
		uint8_t GetVersionNumber() const noexcept { return m_CurSection.GetVersionNumber(); }
		uint8_t GetSectionNumber() const noexcept { return m_CurSection.GetSectionNumber(); }
		uint8_t GetLastSectionNumber() const noexcept { return m_CurSection.GetLastSectionNumber(); }

	protected:
	// PSISectionParser::PSISectionHandler
		bool OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection) override;

		virtual bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection);

		PSISection m_CurSection;
	};

	/** ストリーム PSI テーブルクラス */
	class PSIStreamTable
		: public PSITableBase
	{
	public:
		PSIStreamTable(bool ExtendedSection = true, bool IgnoreSectionNumber = false);

	// PSITableBase
		void Reset() override;

	protected:
	// PSISectionParser::PSISectionHandler
		bool OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection) override;

		virtual bool OnTableUpdate(const PSISection *pCurSection);
	};

	/** 空 PSI テーブルクラス */
	class PSINullTable
		: public PIDMapTarget
	{
	public:
	// PIDMapTarget
		void OnPIDUnmapped(uint16_t PID) override;
	};

	/** PSI テーブル集合クラス */
	class PSITableSet
		: public PSITableBase
	{
	public:
		PSITableSet(bool ExtendedSection = true);
		~PSITableSet();

	// PSITableBase
		void Reset() override;

	// PSITableSet
		bool MapTable(uint8_t TableID, PSITableBase *pTable);
		bool UnmapTable(uint8_t TableID);
		void UnmapAllTables();
		PSITableBase * GetTableByID(uint8_t TableID);
		const PSITableBase * GetTableByID(uint8_t TableID) const;
		const PSITableBase * GetLastUpdatedTable() const;

		uint8_t GetLastUpdatedTableID() const noexcept { return m_LastUpdatedTableID; }
		uint8_t GetLastUpdatedSectionNumber() const noexcept { return m_LastUpdatedSectionNumber; }
		unsigned long long GetLastUpdatedTableUniqueID() const noexcept { return m_LastUpdatedTableUniqueID; }

	protected:
	// PSISectionParser::PSISectionHandler
		bool OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection) override;

		typedef std::map<uint8_t, std::unique_ptr<PSITableBase>> SectionTableMap;
		SectionTableMap m_TableMap;

		uint8_t m_LastUpdatedTableID;
		uint8_t m_LastUpdatedSectionNumber;
		unsigned long long m_LastUpdatedTableUniqueID;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_SPI_TABLE_H
