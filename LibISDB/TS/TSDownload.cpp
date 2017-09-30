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
 @file   TSDownload.cpp
 @brief  データダウンロード
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "TSDownload.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


DataModule::DataModule(uint32_t DownloadID, uint16_t BlockSize, uint16_t ModuleID, uint32_t ModuleSize, uint8_t ModuleVersion)
	: m_DownloadID(DownloadID)
	, m_BlockSize(BlockSize)
	, m_ModuleID(ModuleID)
	, m_ModuleSize(ModuleSize)
	, m_ModuleVersion(ModuleVersion)
	, m_NumBlocks((uint16_t)((ModuleSize - 1) / BlockSize + 1))
	, m_NumDownloadedBlocks(0)
	, m_pData(nullptr)
{
	m_BlockDownloaded.SetSize(m_NumBlocks);
	m_BlockDownloaded.Reset();
}


DataModule::~DataModule()
{
	delete [] m_pData;
}


bool DataModule::StoreBlock(uint16_t BlockNumber, const void *pData, uint16_t DataSize)
{
	if (BlockNumber >= m_NumBlocks)
		return false;

	if (IsBlockDownloaded(BlockNumber))
		return true;

	if (m_pData == nullptr)
		m_pData = new uint8_t[m_ModuleSize];

	size_t Offset, Size;
	Offset = (size_t)BlockNumber * m_BlockSize;
	if (BlockNumber < m_NumBlocks - 1)
		Size = m_BlockSize;
	else
		Size = m_ModuleSize - Offset;
	if (DataSize < Size)
		return false;

	std::memcpy(&m_pData[Offset], pData, Size);

	SetBlockDownloaded(BlockNumber);
	m_NumDownloadedBlocks++;

	if (IsComplete()) {
		OnComplete(m_pData, m_ModuleSize);
	}

	return true;
}


bool DataModule::IsBlockDownloaded(uint16_t BlockNumber) const
{
	if (BlockNumber >= m_NumBlocks)
		return false;
	return m_BlockDownloaded[BlockNumber];
}


bool DataModule::SetBlockDownloaded(uint16_t BlockNumber)
{
	if (BlockNumber >= m_NumBlocks)
		return false;
	m_BlockDownloaded.Set(BlockNumber);
	return true;
}




DownloadInfoIndicationParser::DownloadInfoIndicationParser(EventHandler *pHandler)
	: m_pEventHandler(pHandler)
{
}


bool DownloadInfoIndicationParser::ParseData(const uint8_t *pData, uint16_t DataSize)
{
	if (DataSize < 34)
		return false;

	MessageInfo Message;

	Message.ProtocolDiscriminator  = pData[0];
	Message.DSMCCType              = pData[1];
	Message.MessageID              = Load16(&pData[2]);
	Message.TransactionID          = Load32(&pData[4]);

	const uint8_t AdaptationLength = pData[9];
//	const uint16_t MessageLength   = Load16(&pData[10]);
	if (12 + (uint16_t)AdaptationLength > DataSize)
		return false;
	uint16_t Pos = 12 + AdaptationLength;

	Message.DownloadID             = Load32(&pData[Pos]);
	Message.BlockSize              = Load16(&pData[Pos + 4]);
	if ((Message.BlockSize == 0) || (Message.BlockSize > 4066))
		return false;
	Message.WindowSize             = pData[Pos + 6];
	Message.ACKPeriod              = pData[Pos + 7];
	Message.TCDownloadWindow       = Load32(&pData[Pos + 8]);
	Message.TCDownloadScenario     = Load32(&pData[Pos + 12]);

	// Compatibility Descriptor
	const uint16_t DescLength      = Load16(&pData[Pos + 16]);
	if (Pos + 18 + DescLength + 2 > DataSize)
		return false;
	Pos += 18 + DescLength;

	const uint16_t NumberOfModules = Load16(&pData[Pos + 0]);
	Pos += 2;

	for (uint16_t i = 0; i < NumberOfModules; i++) {
		if (Pos + 8 > DataSize)
			return false;

		ModuleInfo Module;

		Module.ModuleID      = Load16(&pData[Pos + 0]);
		Module.ModuleSize    = Load32(&pData[Pos + 2]);
		Module.ModuleVersion = pData[Pos + 6];

		const uint8_t ModuleInfoLength = pData[Pos + 7];
		Pos += 8;
		if (Pos + ModuleInfoLength > DataSize)
			return false;

		Module.ModuleDesc.Name.Length = 0;
		Module.ModuleDesc.Name.pText = nullptr;
		Module.ModuleDesc.CRC.IsValid = false;

		for (uint8_t DescPos = 0; DescPos + 2 < ModuleInfoLength;) {
			const uint8_t DescTag    = pData[Pos + DescPos + 0];
			const uint8_t DescLength = pData[Pos + DescPos + 1];

			DescPos += 2;

			if (DescPos + DescLength > ModuleInfoLength)
				break;

			switch (DescTag) {
			case 0x02:	// Name descriptor
				Module.ModuleDesc.Name.Length = DescLength;
				Module.ModuleDesc.Name.pText = (const char *)(&pData[Pos + DescPos]);
				break;

			case 0x05:	// CRC32 descriptor
				if (DescLength == 4) {
					Module.ModuleDesc.CRC.IsValid = true;
					Module.ModuleDesc.CRC.CRC32 = Load32(&pData[Pos + DescPos]);
				}
				break;
			}

			DescPos += DescLength;
		}

		if (m_pEventHandler != nullptr)
			m_pEventHandler->OnDataModule(&Message, &Module);

		Pos += ModuleInfoLength;
	}

	return true;
}




DownloadDataBlockParser::DownloadDataBlockParser(EventHandler *pHandler)
	: m_pEventHandler(pHandler)
{
}


bool DownloadDataBlockParser::ParseData(const uint8_t *pData, uint16_t DataSize)
{
	if (DataSize < 12)
		return false;

	DataBlockInfo DataBlock;

	DataBlock.ProtocolDiscrimnator = pData[0];
	DataBlock.DSMCCType            = pData[1];
	DataBlock.MessageID            = Load16(&pData[2]);
	DataBlock.DownloadID           = Load32(&pData[4]);

	const uint8_t AdaptationLength = pData[9];
//	const uint16_t MessageLength   = Load16(&pData[10]);
	if (12 + (uint16_t)AdaptationLength + 6 >= DataSize)
		return false;

	uint16_t Pos = 12 + AdaptationLength;

	DataBlock.ModuleID             = Load16(&pData[Pos + 0]);
	DataBlock.ModuleVersion        = pData[Pos + 2];
	DataBlock.BlockNumber          = Load16(&pData[Pos + 4]);
	Pos += 6;
	DataBlock.DataSize             = DataSize - Pos;
	DataBlock.pData                = &pData[Pos];

	if (m_pEventHandler != nullptr)
		m_pEventHandler->OnDataBlock(&DataBlock);

	return true;
}


}	// namespace LibISDB
