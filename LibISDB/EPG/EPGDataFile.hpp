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
 @file   EPGDataFile.hpp
 @brief  EPG データファイル
 @author DBCTRADO
*/


#ifndef LIBISDB_EPG_DATA_FILE_H
#define LIBISDB_EPG_DATA_FILE_H


#include "../Base/ObjectBase.hpp"
#include "../Base/FileStream.hpp"
#include "EPGDatabase.hpp"


namespace LibISDB
{

	/** EPG データファイルクラス */
	class EPGDataFile
		: public ObjectBase
	{
	public:
		/** オープンフラグ */
		enum class OpenFlag : unsigned int {
			None         = 0x0000U, /**< 指定なし */
			Read         = 0x0001U, /**< 読み込み */
			Write        = 0x0002U, /**< 書き出し */
			ShareRead    = 0x0004U, /**< 共有読み込み */
			PriorityLow  = 0x0010U, /**< 低優先度 */
			PriorityIdle = 0x0020U, /**< 最低優先度 */
			DiscardOld   = 0x0040U, /**< 古い情報を破棄 */
			Flush        = 0x0080U, /**< 書き出し時にフラッシュする */
		};

		enum class Exception {
			Read,
			Write,
			Seek,
			MemoryAllocate,
			FormatError,
			Internal,
		};

		EPGDataFile() noexcept;

	// LibISDB::ObjectBase
		const CharType * GetObjectName() const noexcept { return LIBISDB_STR("EPGDataFile"); }

		bool Open(EPGDatabase *pEPGDatabase, const CStringView &FileName, OpenFlag Flags);
		void Close();
		bool IsOpen() const;
		bool Load();
		bool LoadMerged();
		bool LoadHeader();
		bool Save();

		EPGDatabase * GetEPGDatabase() const noexcept { return m_pEPGDatabase; }
		const String & GetFileName() const noexcept { return m_FileName; }
		OpenFlag GetOpenFlags() const noexcept { return m_OpenFlags; }
		uint64_t GetUpdateCount() const noexcept { return m_UpdateCount; }

	protected:
		struct ServiceInfo {
			EPGDatabase::ServiceInfo Info;
			EPGDatabase::EventList EventList;
		};

		void LoadService(FileStream &File, ServiceInfo *pServiceInfo);
		void LoadEvent(FileStream &File, const ServiceInfo *pServiceInfo, EventInfo *pEvent);
		void SaveService(
			FileStream &File, const EPGDatabase::ServiceInfo &ServiceInfo,
			uint16_t EventCount, const DateTime &EarliestTime);
		void SaveEvent(FileStream &File, const EventInfo &Event);
		void ExceptionLog(Exception Code);

		EPGDatabase *m_pEPGDatabase;
		String m_FileName;
		OpenFlag m_OpenFlags;
		uint64_t m_UpdateCount;
	};

	LIBISDB_ENUM_FLAGS(EPGDataFile::OpenFlag)

}	// namespace LibISDB


#endif	// ifndef LIBISDB_EPG_DATA_FILE_H
