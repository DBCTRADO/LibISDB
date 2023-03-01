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
 @file   OneSegPATGenerator.cpp
 @brief  ワンセグ PAT 生成
 @author DBCTRADO
*/


#include "../LibISDBPrivate.hpp"
#include "OneSegPATGenerator.hpp"
#include "Tables.hpp"
#include "../Utilities/Utilities.hpp"
#include "../Utilities/CRC.hpp"
#include "../Base/DebugDef.hpp"


namespace LibISDB
{


OneSegPATGenerator::OneSegPATGenerator()
{
	Reset();
}


void OneSegPATGenerator::Reset()
{
	m_TransportStreamID = TRANSPORT_STREAM_ID_INVALID;
	m_HasPAT = false;
	m_GeneratePAT = false;
	m_PIDMapManager.UnmapAllTargets();
	m_ContinuityCounter = 0;
	std::memset(m_PMTCount, 0, sizeof(m_PMTCount));

	// NITテーブルPIDマップ追加
	m_PIDMapManager.MapTarget(PID_NIT, PSITableBase::CreateWithHandler<NITMultiTable>(&OneSegPATGenerator::OnNITSection, this));

	// PMTテーブルPIDマップ追加
	for (uint16_t PID = ONESEG_PMT_PID_FIRST; PID <= ONESEG_PMT_PID_LAST; PID++)
		m_PIDMapManager.MapTarget(PID, new PMTTable);
}


bool OneSegPATGenerator::StorePacket(const TSPacket *pPacket)
{
	const uint16_t PID = pPacket->GetPID();

	if (PID == PID_PAT) {
		m_HasPAT = true;
	} else if ((PID == PID_NIT) || Is1SegPMTPID(PID)) {
		if (m_PIDMapManager.StorePacket(pPacket)) {
			if ((PID != PID_NIT) && !m_HasPAT) {
				if (!m_GeneratePAT) {
					// PMTがPAT_GEN_PMT_COUNT回来る間にPATが来なければPATが無いとみなす
					constexpr uint8_t PAT_GEN_PMT_COUNT = 5;
					const int Index = PID - ONESEG_PMT_PID_FIRST;

					if (m_PMTCount[Index] < PAT_GEN_PMT_COUNT) {
						m_PMTCount[Index]++;
						if (m_PMTCount[Index] == PAT_GEN_PMT_COUNT) {
							m_GeneratePAT = true;
							LIBISDB_TRACE(LIBISDB_STR("OneSegPATGenerator : Generate 1Seg PAT\n"));
						}
					}
				}

				return m_GeneratePAT && (m_TransportStreamID != TRANSPORT_STREAM_ID_INVALID);
			}
		}
	}

	return false;
}


bool OneSegPATGenerator::GetPATPacket(TSPacket *pPacket)
{
	if (m_TransportStreamID == TRANSPORT_STREAM_ID_INVALID)
		return false;

	const PMTTable *PMTList[ONESEG_PMT_PID_COUNT];
	int PMTCount = 0;
	for (int i = 0; i < ONESEG_PMT_PID_COUNT; i++) {
		const PMTTable *pTable = m_PIDMapManager.GetMapTarget<PMTTable>(ONESEG_PMT_PID_FIRST + i);
		if ((pTable != nullptr) && (pTable->GetProgramNumberID() != 0)) {
			PMTList[i] = pTable;
			PMTCount++;
		} else {
			if (i == 0)
				return false;	// 先頭 PMT が無い
			PMTList[i] = nullptr;
		}
	}

	const uint16_t SectionLength = 5 + (PMTCount + 1) * 4 + 4;

	if (pPacket->SetSize(TS_PACKET_SIZE) < TS_PACKET_SIZE)
		return false;

	uint8_t *pData = pPacket->GetData();

	// TS header
	pData[0] = 0x47;	// Sync
	pData[1] = 0x60;
	pData[2] = 0x00;
	pData[3] = 0x10 | (m_ContinuityCounter & 0x0F);
	pData[4] = 0x00;

	// PAT
	pData[5] = 0x00;	// table_id
	pData[6] = 0xF0 | (SectionLength >> 8);
	pData[7] = SectionLength & 0xFF;
	pData[8] = m_TransportStreamID >> 8;
	pData[9] = m_TransportStreamID & 0xFF;
	pData[10] = 0xC1;	// reserved(2) + version_number(5) + current_next_indicator(1)
	pData[11] = 0x00;	// section_number
	pData[12] = 0x00;	// last_section_number

	pData[13] = 0x00;
	pData[14] = 0x00;
	pData[15] = 0xE0;	// reserved(3) + NIT PID (high)
	pData[16] = 0x10;	// NIT PID (low)

	int Pos = 17;
	for (int i = 0; i <= ONESEG_PMT_PID_LAST - ONESEG_PMT_PID_FIRST; i++) {
		if (PMTList[i] != nullptr) {
			const uint16_t ServiceID = PMTList[i]->GetProgramNumberID();
			const uint16_t PID = ONESEG_PMT_PID_FIRST + i;
			pData[Pos + 0] = ServiceID >> 8;
			pData[Pos + 1] = ServiceID & 0xFF;
			pData[Pos + 2] = 0xE0 | (PID >> 8);
			pData[Pos + 3] = PID & 0xFF;
			Pos += 4;
		}
	}

	Store32(&pData[Pos], CRC32MPEG2::Calc(&pData[5], 8 + (PMTCount + 1) * 4));
	Pos += 4;

	std::memset(&pData[Pos], 0xFF, TS_PACKET_SIZE - Pos);

#ifdef LIBISDB_DEBUG
	const TSPacket::ParseResult Result = pPacket->ParsePacket();
	LIBISDB_ASSERT(Result == TSPacket::ParseResult::OK);
#else
	pPacket->ParsePacket();
#endif

	m_ContinuityCounter++;

	return true;
}


/*
	TSID が予め分かっている場合に指定することで、NIT が来るのを待たずに PAT を生成することができる
*/
bool OneSegPATGenerator::SetTransportStreamID(uint16_t TransportStreamID)
{
	if (m_TransportStreamID != TRANSPORT_STREAM_ID_INVALID)
		return false;
	m_TransportStreamID = TransportStreamID;
	return true;
}


void OneSegPATGenerator::OnNITSection(const PSITableBase *pTable, const PSISection *pSection)
{
	const NITMultiTable *pNITMultiTable = dynamic_cast<const NITMultiTable *>(pTable);
	uint16_t TransportStreamID = TRANSPORT_STREAM_ID_INVALID;

	if (pNITMultiTable != nullptr) {
		const NITTable *pNITTable = pNITMultiTable->GetNITTable(0);

		if (pNITTable != nullptr) {
			const DescriptorBlock *pDescBlock = pNITTable->GetItemDescriptorBlock(0);

			if (pDescBlock != nullptr) {
				const PartialReceptionDescriptor *pPartialReceptionDesc =
					pDescBlock->GetDescriptor<PartialReceptionDescriptor>();
				if ((pPartialReceptionDesc != nullptr)
						&& (pPartialReceptionDesc->GetServiceCount() > 0)) {
					TransportStreamID = pNITTable->GetTransportStreamID(0);
				}
			}
		}
	}

	if (m_TransportStreamID != TransportStreamID) {
		m_TransportStreamID = TransportStreamID;
		m_HasPAT = false;
	}
}


}	// namespace LibISDB
