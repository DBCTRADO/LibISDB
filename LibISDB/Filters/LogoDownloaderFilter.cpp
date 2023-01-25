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
 @file   LogoDownloaderFilter.cpp
 @brief  ロゴ取得フィルタ
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "LogoDownloaderFilter.hpp"
#include "../TS/TSDownload.hpp"
#include "../TS/Tables.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/StringUtilities.hpp"
#include <functional>
#include <memory>
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


namespace
{


class LogoDataModule : public DataModule
{
public:
	struct ServiceInfo {
		uint16_t NetworkID;
		uint16_t TransportStreamID;
		uint16_t ServiceID;
	};

	struct LogoInfo {
		uint8_t LogoType;
		uint16_t LogoID;
		std::vector<ServiceInfo> ServiceList;
		uint16_t DataSize;
		const uint8_t *pData;
	};

	class EventHandler {
	public:
		virtual void OnLogoData(const LogoDataModule *pModule, const LogoInfo *pInfo) = 0;
	};

	LogoDataModule(
		uint32_t DownloadID, uint16_t BlockSize, uint16_t ModuleID, uint32_t ModuleSize, uint8_t ModuleVersion,
		EventHandler *pHandler)
		: DataModule(DownloadID, BlockSize, ModuleID, ModuleSize, ModuleVersion)
		, m_pEventHandler(pHandler)
	{
	}

	bool EnumLogoData()
	{
		if (!IsComplete())
			return false;
		OnComplete(m_pData, m_ModuleSize);
		return true;
	}

private:
// DataModule
	void OnComplete(const uint8_t *pData, uint32_t ModuleSize) override;

	EventHandler *m_pEventHandler;
};


void LogoDataModule::OnComplete(const uint8_t *pData, uint32_t ModuleSize)
{
	if (ModuleSize < 3)
		return;

	LogoInfo Info;

	Info.LogoType = pData[0];
	if (Info.LogoType > 0x05)
		return;

	const uint16_t NumberOfLoop = Load16(&pData[1]);

	uint32_t Pos = 3;

	for (uint16_t i = 0; i < NumberOfLoop; i++) {
		if (Pos + 3 >= ModuleSize)
			return;

		Info.LogoID = static_cast<uint16_t>(((pData[Pos + 0] & 0x01) << 8) | pData[Pos + 1]);
		const uint8_t NumberOfServices = pData[Pos + 2];
		Pos += 3;
		if (Pos + 6 * NumberOfServices + 2 >= ModuleSize)
			return;

		Info.ServiceList.resize(NumberOfServices);

		LIBISDB_TRACE(LIBISDB_STR("[{}/{}] Logo ID {:04X} / {} Services\n"), i + 1, NumberOfLoop, Info.LogoID, NumberOfServices);

		for (uint8_t j = 0; j < NumberOfServices; j++) {
			Info.ServiceList[j].NetworkID         = Load16(&pData[Pos + 0]);
			Info.ServiceList[j].TransportStreamID = Load16(&pData[Pos + 2]);
			Info.ServiceList[j].ServiceID         = Load16(&pData[Pos + 4]);
			Pos += 6;

			LIBISDB_TRACE(
				LIBISDB_STR("[{}:{:2}/{:2}] Network ID {:04X} / TSID {:04X} / Service ID {:04X}\n"),
				i + 1, j + 1, NumberOfServices,
				Info.ServiceList[j].NetworkID,
				Info.ServiceList[j].TransportStreamID,
				Info.ServiceList[j].ServiceID);
		}

		const uint16_t DataSize = Load16(&pData[Pos + 0]);
		Pos += 2;
		if (Pos + DataSize > ModuleSize)
			return;

		if ((NumberOfServices > 0) && (DataSize > 0)) {
			Info.DataSize = DataSize;
			Info.pData = &pData[Pos];

			m_pEventHandler->OnLogoData(this, &Info);
		}

		Pos += DataSize;
	}
}


class DSMCCSection
	: public PSIStreamTable
	, public DownloadInfoIndicationParser::EventHandler
	, public DownloadDataBlockParser::EventHandler
	, public LogoDataModule::EventHandler
{
public:
	typedef std::function<void(LogoDownloaderFilter::LogoData *pData, uint32_t DownloadID)> LogoDataHandler;

	template<typename T> static LogoDataHandler BindLogoDataHandler(
		void (T::*pFunc)(LogoDownloaderFilter::LogoData *, uint32_t), T *pObject)
	{
		return std::bind(pFunc, pObject, std::placeholders::_1, std::placeholders::_2);
	}

	DSMCCSection(const LogoDataHandler &Handler);
	bool EnumLogoData(uint32_t DownloadID);

private:
// PSIStreamTable
	bool OnTableUpdate(const PSISection *pCurSection) override;

// DownloadInfoIndicationParser::EventHandler
	void OnDataModule(
		const DownloadInfoIndicationParser::MessageInfo *pMessageInfo,
		const DownloadInfoIndicationParser::ModuleInfo *pModuleInfo) override;

// DownloadDataBlockParser::EventHandler
	void OnDataBlock(const DownloadDataBlockParser::DataBlockInfo *pDataBlock) override;

// LogoDataModule::EventHandler
	void OnLogoData(const LogoDataModule *pModule, const LogoDataModule::LogoInfo *pInfo) override;

	DownloadInfoIndicationParser m_DII;
	DownloadDataBlockParser m_DDB;

	typedef std::map<uint16_t, std::unique_ptr<LogoDataModule>> LogoDataMap;
	LogoDataMap m_LogoDataMap;

	LogoDataHandler m_LogoDataHandler;

#ifdef LIBISDB_ENABLE_TRACE
	uint16_t m_PID;

	void OnPIDMapped(uint16_t PID) override
	{
		m_PID = PID;
		PSIStreamTable::OnPIDMapped(PID);
	}
#endif
};


DSMCCSection::DSMCCSection(const LogoDataHandler &Handler)
	: PSIStreamTable(true, true)
	, m_DII(this)
	, m_DDB(this)
	, m_LogoDataHandler(Handler)
#ifdef LIBISDB_ENABLE_TRACE
	, m_PID(PID_INVALID)
#endif
{
}


bool DSMCCSection::EnumLogoData(uint32_t DownloadID)
{
	for (auto const &e : m_LogoDataMap) {
		if (e.second->GetDownloadID() == DownloadID) {
			if (e.second->IsComplete()) {
				return e.second->EnumLogoData();
			}
		}
	}

	return false;
}


bool DSMCCSection::OnTableUpdate(const PSISection *pCurSection)
{
	const uint16_t DataSize = pCurSection->GetPayloadSize();
	const uint8_t *pData = pCurSection->GetPayloadData();

	if (pCurSection->GetTableID() == 0x3B) {
		// DII
		return m_DII.ParseData(pData, DataSize);
	} else if (pCurSection->GetTableID() == 0x3C) {
		// DDB
		return m_DDB.ParseData(pData, DataSize);
	}

	return false;
}


void DSMCCSection::OnDataModule(
	const DownloadInfoIndicationParser::MessageInfo *pMessageInfo,
	const DownloadInfoIndicationParser::ModuleInfo *pModuleInfo)
{
	if ((pModuleInfo->ModuleDesc.Name.pText == nullptr)
			|| (pModuleInfo->ModuleDesc.Name.Length != 7
				&& pModuleInfo->ModuleDesc.Name.Length != 10)
			|| (pModuleInfo->ModuleDesc.Name.Length == 7
				&& StringCompare(pModuleInfo->ModuleDesc.Name.pText, "LOGO-0", 6) != 0)
			|| (pModuleInfo->ModuleDesc.Name.Length == 10
				&& StringCompare(pModuleInfo->ModuleDesc.Name.pText, "CS_LOGO-0", 9) != 0))
		return;

#ifdef LIBISDB_ENABLE_TRACE
	LIBISDB_TRACE(
		LIBISDB_STR("DII Logo Data [PID {:04x}] : Download ID {:08x} / Module ID {:04X} / Module size {}\n"),
		m_PID,
		pMessageInfo->DownloadID,
		pModuleInfo->ModuleID,
		pModuleInfo->ModuleSize);
#endif

	auto it = m_LogoDataMap.find(pModuleInfo->ModuleID);
	if (it == m_LogoDataMap.end()) {
		m_LogoDataMap.emplace(
			pModuleInfo->ModuleID,
			new LogoDataModule(
				pMessageInfo->DownloadID, pMessageInfo->BlockSize,
				pModuleInfo->ModuleID, pModuleInfo->ModuleSize, pModuleInfo->ModuleVersion,
				this));
	} else if ((it->second->GetDownloadID()    != pMessageInfo->DownloadID)
			|| (it->second->GetBlockSize()     != pMessageInfo->BlockSize)
			|| (it->second->GetModuleSize()    != pModuleInfo->ModuleSize)
			|| (it->second->GetModuleVersion() != pModuleInfo->ModuleVersion)) {
		m_LogoDataMap[pModuleInfo->ModuleID].reset(
			new LogoDataModule(
				pMessageInfo->DownloadID, pMessageInfo->BlockSize,
				pModuleInfo->ModuleID, pModuleInfo->ModuleSize, pModuleInfo->ModuleVersion,
				this));
	}
}


void DSMCCSection::OnDataBlock(const DownloadDataBlockParser::DataBlockInfo *pDataBlock)
{
	auto it = m_LogoDataMap.find(pDataBlock->ModuleID);
	if (it != m_LogoDataMap.end()) {
		if ((it->second->GetDownloadID() == pDataBlock->DownloadID)
				&& (it->second->GetModuleVersion() == pDataBlock->ModuleVersion)) {
			it->second->StoreBlock(pDataBlock->BlockNumber, pDataBlock->pData, pDataBlock->DataSize);
		}
	}
}


void DSMCCSection::OnLogoData(const LogoDataModule *pModule, const LogoDataModule::LogoInfo *pInfo)
{
	LogoDownloaderFilter::LogoData LogoData;

	LogoData.NetworkID = pInfo->ServiceList[0].NetworkID;
	LogoData.ServiceList.resize(pInfo->ServiceList.size());
	for (size_t i = 0; i < pInfo->ServiceList.size(); i++) {
		LogoData.ServiceList[i].NetworkID         = pInfo->ServiceList[i].NetworkID;
		LogoData.ServiceList[i].TransportStreamID = pInfo->ServiceList[i].TransportStreamID;
		LogoData.ServiceList[i].ServiceID         = pInfo->ServiceList[i].ServiceID;
	}
	LogoData.LogoID      = pInfo->LogoID;
	LogoData.LogoVersion = 0;
	LogoData.LogoType    = pInfo->LogoType;
	LogoData.DataSize    = pInfo->DataSize;
	LogoData.pData       = pInfo->pData;

	m_LogoDataHandler(&LogoData, pModule->GetDownloadID());
}


}	// namespace




LogoDownloaderFilter::LogoDownloaderFilter()
	: m_pLogoHandler(nullptr)
{
	Reset();
}


void LogoDownloaderFilter::Reset()
{
	BlockLock Lock(m_FilterLock);

	m_PIDMapManager.UnmapAllTargets();
	m_PIDMapManager.MapTarget(PID_PAT, PSITableBase::CreateWithHandler<PATTable>(&LogoDownloaderFilter::OnPATSection, this));
	m_PIDMapManager.MapTarget(PID_CDT, PSITableBase::CreateWithHandler<CDTTable>(&LogoDownloaderFilter::OnCDTSection, this));
	m_PIDMapManager.MapTarget(PID_SDTT, PSITableBase::CreateWithHandler<SDTTTable>(&LogoDownloaderFilter::OnSDTTSection, this));
	m_PIDMapManager.MapTarget(PID_TOT, new TOTTable);

	m_ServiceList.clear();

	m_VersionMap.clear();
}


bool LogoDownloaderFilter::ProcessData(DataStream *pData)
{
	if (pData->Is<TSPacket>())
		m_PIDMapManager.StorePacketStream(pData);

	return true;
}


void LogoDownloaderFilter::SetLogoHandler(LogoHandler *pHandler)
{
	BlockLock Lock(m_FilterLock);

	m_pLogoHandler = pHandler;
}


void LogoDownloaderFilter::OnLogoDataModule(LogoData *pData, uint32_t DownloadID)
{
	if (m_pLogoHandler != nullptr) {
		auto itVersion = m_VersionMap.find(DownloadID);
		if (itVersion != m_VersionMap.end())
			pData->LogoVersion = itVersion->second;

		GetTOTTime(&pData->Time);

		m_pLogoHandler->OnLogoDownloaded(*pData);
	}
}


void LogoDownloaderFilter::OnCDTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// CDTからロゴ取得
	const CDTTable *pCDTTable = dynamic_cast<const CDTTable *>(pTable);

	if ((pCDTTable != nullptr)
			&& (pCDTTable->GetDataType() == CDTTable::DATA_TYPE_LOGO)
			&& (m_pLogoHandler != nullptr)) {
		const uint16_t DataSize = pCDTTable->GetDataModuleSize();
		const uint8_t *pData = pCDTTable->GetDataModuleData();

		if ((DataSize > 7) && (pData != nullptr)) {
			LogoData Data;

			Data.NetworkID         = pCDTTable->GetOriginalNetworkID();
			Data.LogoID            = Load16(&pData[1]) & 0x01FF_u16;
			Data.LogoVersion       = Load16(&pData[3]) & 0x0FFF_u16;
			Data.LogoType          = pData[0];
			Data.DataSize          = Load16(&pData[5]);
			Data.pData             = &pData[7];

			if ((Data.LogoType <= 0x05) && (Data.DataSize <= DataSize - 7)) {
				GetTOTTime(&Data.Time);

				m_pLogoHandler->OnLogoDownloaded(Data);
			}
		}
	}
}


void LogoDownloaderFilter::OnSDTTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// SDTTからバージョンを取得
	// (SDTTを元にダウンロードするのが本来だと思うが、SDTTがあまり流れて来ないのでこのような形になっている)
	const SDTTTable *pSDTTTable = dynamic_cast<const SDTTTable *>(pTable);

	if ((pSDTTTable != nullptr) && pSDTTTable->IsCommon()) {
		std::vector<uint32_t> UpdatedDownloadIDList;

		const SDTTTable::ContentInfo *pInfo;
		for (uint8_t i = 0; (pInfo = pSDTTTable->GetContentInfo(i)) != nullptr; i++) {
			const DescriptorBase *pDesc;
			for (int j = 0; (pDesc = pInfo->Descriptors.GetDescriptorByIndex(j)) != nullptr; j++) {
				if (pDesc->GetTag() == DownloadContentDescriptor::TAG) {
					const DownloadContentDescriptor *pDownloadContentDesc =
						dynamic_cast<const DownloadContentDescriptor *>(pDesc);
					if (pDownloadContentDesc != nullptr) {
						const uint32_t DownloadID = pDownloadContentDesc->GetDownloadID();
						LIBISDB_TRACE(
							LIBISDB_STR("Download version {:#x} = {:#03x}\n"),
							DownloadID, pInfo->NewVersion);
						std::map<uint32_t, uint16_t>::iterator itVersion = m_VersionMap.find(DownloadID);
						if ((itVersion == m_VersionMap.end())
								|| (itVersion->second != pInfo->NewVersion)) {
							m_VersionMap[DownloadID] = pInfo->NewVersion;
							UpdatedDownloadIDList.push_back(DownloadID);
						}
					}
				}
			}
		}

		if (!UpdatedDownloadIDList.empty()) {
			for (auto itService = m_ServiceList.begin(); itService != m_ServiceList.end(); ++itService) {
				for (size_t i = 0; i < itService->ESList.size(); i++) {
					DSMCCSection *pDSMCCSection =
						m_PIDMapManager.GetMapTarget<DSMCCSection>(itService->ESList[i]);
					if (pDSMCCSection != nullptr) {
						for (size_t j = 0; j < UpdatedDownloadIDList.size(); j++)
							pDSMCCSection->EnumLogoData(UpdatedDownloadIDList[j]);
					}
				}
			}
		}
	}
}


void LogoDownloaderFilter::OnPATSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PATが更新された
	const PATTable *pPATTable = dynamic_cast<const PATTable *>(pTable);
	if (pPATTable == nullptr)
		return;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		m_PIDMapManager.UnmapTarget(m_ServiceList[i].PMTPID);
		if (m_ServiceList[i].ServiceType == SERVICE_TYPE_ENGINEERING) {
			UnmapDataES(static_cast<int>(i));
		}
	}

	m_ServiceList.resize(pPATTable->GetProgramCount());

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		const uint16_t PMTPID = pPATTable->GetPMTPID(static_cast<int>(i));

		m_ServiceList[i].ServiceID   = pPATTable->GetProgramNumber(static_cast<int>(i));
		m_ServiceList[i].PMTPID      = PMTPID;
		m_ServiceList[i].ServiceType = SERVICE_TYPE_INVALID;
		m_ServiceList[i].ESList.clear();

		m_PIDMapManager.MapTarget(PMTPID, PSITableBase::CreateWithHandler<PMTTable>(&LogoDownloaderFilter::OnPMTSection, this));
	}

	m_PIDMapManager.MapTarget(PID_NIT, PSITableBase::CreateWithHandler<NITMultiTable>(&LogoDownloaderFilter::OnNITSection, this));
}


void LogoDownloaderFilter::OnPMTSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// PMTが更新された
	const PMTTable *pPMTTable = dynamic_cast<const PMTTable *>(pTable);
	if (pPMTTable == nullptr)
		return;

	const int ServiceIndex = GetServiceIndexByID(pPMTTable->GetProgramNumberID());
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = m_ServiceList[ServiceIndex];

	if (Info.ServiceType == SERVICE_TYPE_ENGINEERING) {
		UnmapDataES(ServiceIndex);
	}

	Info.ESList.clear();

	for (uint16_t ESIndex = 0; ESIndex < pPMTTable->GetESCount(); ESIndex++) {
		if (pPMTTable->GetStreamType(ESIndex) == STREAM_TYPE_DATA_CARROUSEL) {
			const DescriptorBlock *pDescBlock = pPMTTable->GetItemDescriptorBlock(ESIndex);
			if (pDescBlock != nullptr) {
				const StreamIDDescriptor *pStreamIDDesc = pDescBlock->GetDescriptor<StreamIDDescriptor>();

				if ((pStreamIDDesc != nullptr)
						&& (pStreamIDDesc->GetComponentTag() == 0x79
							|| pStreamIDDesc->GetComponentTag() == 0x7A)) {
					// 全受信機共通データ
					Info.ESList.push_back(pPMTTable->GetESPID(ESIndex));
				}
			}
		}
	}

	if (Info.ServiceType == SERVICE_TYPE_ENGINEERING) {
		MapDataES(ServiceIndex);
	}
}


void LogoDownloaderFilter::OnNITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	// NITが更新された
	const NITMultiTable *pNITMultiTable = dynamic_cast<const NITMultiTable *>(pTable);
	if ((pNITMultiTable == nullptr) || !pNITMultiTable->IsNITComplete())
		return;

	for (int SectionNo = 0; SectionNo < pNITMultiTable->GetNITSectionCount(); SectionNo++) {
		const NITTable *pNITTable = pNITMultiTable->GetNITTable(SectionNo);

		if (pNITTable != nullptr) {
			for (int i = 0; i < pNITTable->GetTransportStreamCount(); i++) {
				const DescriptorBlock *pDescBlock = pNITTable->GetItemDescriptorBlock(i);

				if (pDescBlock != nullptr) {
					const ServiceListDescriptor *pServiceListDesc = pDescBlock->GetDescriptor<ServiceListDescriptor>();

					if (pServiceListDesc != nullptr) {
						for (int j = 0; j < pServiceListDesc->GetServiceCount(); j++) {
							ServiceListDescriptor::ServiceInfo Info;

							if (pServiceListDesc->GetServiceInfo(j, &Info)) {
								int Index = GetServiceIndexByID(Info.ServiceID);
								if (Index >= 0) {
									const uint8_t ServiceType = Info.ServiceType;
									if (m_ServiceList[Index].ServiceType != ServiceType) {
										if (ServiceType == SERVICE_TYPE_ENGINEERING) {
											MapDataES(Index);
										} else if (m_ServiceList[Index].ServiceType == SERVICE_TYPE_ENGINEERING) {
											UnmapDataES(Index);
										}
										m_ServiceList[Index].ServiceType = ServiceType;
									}
								}
							}
						}
					}
				}
			}
		}
	}
}


int LogoDownloaderFilter::GetServiceIndexByID(uint16_t ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return static_cast<int>(i);
	}
	return -1;
}


bool LogoDownloaderFilter::MapDataES(int Index)
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	ServiceInfo &Info = m_ServiceList[Index];

	LIBISDB_TRACE(
		LIBISDB_STR("LogoDownloaderFilter::MapDataES() : SID {:04X} / {} stream(s)\n"),
		Info.ServiceID, Info.ESList.size());

	for (uint16_t PID : Info.ESList) {
		m_PIDMapManager.MapTarget(
			PID,
			new DSMCCSection(
				DSMCCSection::BindLogoDataHandler(&LogoDownloaderFilter::OnLogoDataModule, this)));
	}

	return true;
}


bool LogoDownloaderFilter::UnmapDataES(int Index)
{
	if (static_cast<unsigned int>(Index) >= m_ServiceList.size())
		return false;

	ServiceInfo &Info = m_ServiceList[Index];
	for (uint16_t PID : Info.ESList) {
		m_PIDMapManager.UnmapTarget(PID);
	}

	return true;
}


bool LogoDownloaderFilter::GetTOTTime(DateTime *pTime)
{
	const TOTTable *pTOTTable = m_PIDMapManager.GetMapTarget<TOTTable>(PID_TOT);
	if ((pTOTTable == nullptr) || !pTOTTable->GetDateTime(pTime)) {
		pTime->Reset();
		return false;
	}
	return true;
}


}	// namespace LibISDB
