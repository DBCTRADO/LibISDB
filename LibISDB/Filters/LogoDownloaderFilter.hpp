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
 @file   LogoDownloaderFilter.hpp
 @brief  ロゴ取得フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_LOGO_DOWNLOADER_FILTER_H
#define LIBISDB_LOGO_DOWNLOADER_FILTER_H


#include "FilterBase.hpp"
#include "../TS/PSISection.hpp"
#include "../TS/PSITable.hpp"
#include "../TS/PIDMap.hpp"
#include "../Base/DateTime.hpp"
#include <vector>
#include <map>


namespace LibISDB
{

	/** ロゴ取得フィルタクラス */
	class LogoDownloaderFilter
		: public SingleIOFilter
	{
	public:
		struct LogoService {
			uint16_t NetworkID;
			uint16_t TransportStreamID;
			uint16_t ServiceID;
		};

		struct LogoData {
			uint16_t NetworkID;
			std::vector<LogoService> ServiceList;
			uint16_t LogoID;
			uint16_t LogoVersion;
			uint8_t LogoType;
			uint16_t DataSize;
			const uint8_t *pData;
			DateTime Time;
		};

		class LogoHandler
		{
		public:
			virtual ~LogoHandler() = default;

			virtual void OnLogoDownloaded(const LogoData &Data) = 0;
		};

		LogoDownloaderFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("LogoDownloaderFilter"); }

	// FilterBase
		void Reset() override;

	// SingleIOFilter
		bool ProcessData(DataStream *pData) override;

	// LogoDownloaderFilter
		void SetLogoHandler(LogoHandler *pHandler);

	private:
		void OnLogoDataModule(LogoData *pData, uint32_t DownloadID);
		void OnCDTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnSDTTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnPATSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnPMTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnNITSection(const PSITableBase *pTable, const PSISection *pSection);

		int GetServiceIndexByID(uint16_t ServiceID) const;
		bool MapDataES(int Index);
		bool UnmapDataES(int Index);
		bool GetTOTTime(DateTime *pTime);

		struct ServiceInfo {
			uint16_t ServiceID;
			uint16_t PMTPID;
			uint8_t ServiceType;
			std::vector<uint16_t> ESList;
		};

		PIDMapManager m_PIDMapManager;
		LogoHandler *m_pLogoHandler;
		std::vector<ServiceInfo> m_ServiceList;
		std::map<uint32_t, uint16_t> m_VersionMap;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_LOGO_DOWNLOADER_FILTER_H
