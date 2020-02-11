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
 @file   TSDownload.hpp
 @brief  データダウンロード
 @author DBCTRADO
*/


#ifndef LIBISDB_TS_DOWNLOAD_H
#define LIBISDB_TS_DOWNLOAD_H


#include "../Utilities/BitTable.hpp"


namespace LibISDB
{

	class DataModule
	{
	public:
		DataModule(uint32_t DownloadID, uint16_t BlockSize, uint16_t ModuleID, uint32_t ModuleSize, uint8_t ModuleVersion);
		virtual ~DataModule();

		bool StoreBlock(uint16_t BlockNumber, const void *pData, uint16_t DataSize);
		uint32_t GetDownloadID() const noexcept { return m_DownloadID; }
		uint16_t GetBlockSize() const noexcept { return m_BlockSize; }
		uint16_t GetModuleID() const noexcept { return m_ModuleID; }
		uint32_t GetModuleSize() const noexcept { return m_ModuleSize; }
		uint8_t GetModuleVersion() const noexcept { return m_ModuleVersion; }
		bool IsComplete() const noexcept { return m_NumDownloadedBlocks == m_NumBlocks; }
		bool IsBlockDownloaded(uint16_t BlockNumber) const;

	protected:
		bool SetBlockDownloaded(uint16_t BlockNumber);

		virtual void OnComplete(const uint8_t *pData, uint32_t ModuleSize) {}

		uint32_t m_DownloadID;
		uint16_t m_BlockSize;
		uint16_t m_ModuleID;
		uint32_t m_ModuleSize;
		uint8_t m_ModuleVersion;
		uint16_t m_NumBlocks;
		uint16_t m_NumDownloadedBlocks;
		uint8_t *m_pData;
		BitTable m_BlockDownloaded;
	};

	class DownloadInfoIndicationParser
	{
	public:
		struct MessageInfo {
			uint8_t ProtocolDiscriminator;
			uint8_t DSMCCType;
			uint16_t MessageID;
			uint32_t TransactionID;
			uint32_t DownloadID;
			uint16_t BlockSize;
			uint8_t WindowSize;
			uint8_t ACKPeriod;
			uint32_t TCDownloadWindow;
			uint32_t TCDownloadScenario;
		};

		struct ModuleInfo {
			uint16_t ModuleID;
			uint32_t ModuleSize;
			uint8_t ModuleVersion;
			struct {
				struct {
					uint8_t Length;
					const char *pText;
				} Name;
				struct {
					bool IsValid;
					uint32_t CRC32;
				} CRC;
			} ModuleDesc;
		};

		class EventHandler
		{
		public:
			virtual ~EventHandler() = default;
			virtual void OnDataModule(const MessageInfo *pMessageInfo, const ModuleInfo *pModuleInfo) = 0;
		};

		DownloadInfoIndicationParser(EventHandler *pHandler);

		bool ParseData(const uint8_t *pData, uint16_t DataSize);

	private:
		EventHandler *m_pEventHandler;
	};

	class DownloadDataBlockParser
	{
	public:
		struct DataBlockInfo {
			uint8_t ProtocolDiscrimnator;
			uint8_t DSMCCType;
			uint16_t MessageID;
			uint32_t DownloadID;
			uint16_t MessageLength;
			uint16_t ModuleID;
			uint8_t ModuleVersion;
			uint8_t Reserved;
			uint16_t BlockNumber;
			uint16_t DataSize;
			const uint8_t *pData;
		};

		class EventHandler
		{
		public:
			virtual ~EventHandler() = default;
			virtual void OnDataBlock(const DataBlockInfo *pDataBlock) = 0;
		};

		DownloadDataBlockParser(EventHandler *pHandler);

		bool ParseData(const uint8_t *pData, uint16_t DataSize);

	private:
		EventHandler *m_pEventHandler;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TS_DOWNLOAD_H
