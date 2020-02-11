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
 @file   StreamSelector.hpp
 @brief  ストリーム選択
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_SELECTOR_H
#define LIBISDB_STREAM_SELECTOR_H


#include "TSPacket.hpp"
#include "PIDMap.hpp"
#include "PSITable.hpp"
#include <vector>
#include <array>
#include <bitset>


namespace LibISDB
{

	/** ストリーム選択クラス */
	class StreamSelector
	{
	public:
		enum class StreamFlag : unsigned long {
			None             = 0x00000000UL,
			MPEG1Video       = 0x00000001UL,
			MPEG2Video       = 0x00000002UL,
			MPEG1Audio       = 0x00000004UL,
			MPEG2Audio       = 0x00000008UL,
			AAC              = 0x00000010UL,
			MPEG4Visual      = 0x00000020UL,
			MPEG4Audio       = 0x00000040UL,
			H264             = 0x00000080UL,
			H265             = 0x00000100UL,
			AC3              = 0x00000200UL,
			DTS              = 0x00000400UL,
			TrueHD           = 0x00000800UL,
			DolbyDigitalPlus = 0x00001000UL,
			Caption          = 0x00002000UL,
			DataCarrousel    = 0x00004000UL,
			Audio            = MPEG1Audio | MPEG2Audio | AAC | MPEG4Audio | AC3 | DTS | TrueHD | DolbyDigitalPlus,
			Vido             = MPEG1Video | MPEG2Video | MPEG4Visual | H264 | H265,
			All              = 0xFFFFFFFFUL
		};

		class StreamTypeTable
		{
		public:
			StreamTypeTable();
			StreamTypeTable(StreamFlag Flags);

			bool operator [] (size_t Index) const { return m_Bitset[Index]; }

			void Set() { m_Bitset.set(); }
			void Set(size_t Pos, bool Value = true) { m_Bitset.set(Pos, Value); }
			void Reset() { m_Bitset.reset(); }
			void Reset(size_t Pos) { m_Bitset.reset(Pos); }

			void FromStreamFlags(StreamFlag Flags);

		private:
			std::bitset<256> m_Bitset;
		};

		StreamSelector();

		void Reset();
		TSPacket * InputPacket(TSPacket *pPacket);
		bool SetTarget(
			uint16_t ServiceID = SERVICE_ID_INVALID,
			const StreamTypeTable *pStreamTypes = nullptr);
		bool SetTarget(uint16_t ServiceID, StreamFlag StreamFlags);
		uint16_t GetTargetServiceID() const noexcept { return m_TargetServiceID; }
		const StreamTypeTable & GetTargetStreamType() const noexcept { return m_TargetStreamType; }
		void SetGeneratePAT(bool Generate);
		bool GetGeneratePAT() const noexcept { return m_GeneratePAT; }

	protected:
		void MakeTargetPIDTable();
		int GetServiceIndexByID(uint16_t ServiceID) const;
		bool MakePAT(const TSPacket *pSrcPacket, TSPacket *pDstPacket);

		void OnPATSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnPMTSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnCATSection(const PSITableBase *pTable, const PSISection *pSection);

		struct ESInfo {
			uint16_t PID;
			uint8_t StreamType;
		};

		struct PMTPIDInfo {
			uint16_t ServiceID;
			uint16_t PMTPID;
			uint16_t PCRPID;
			std::vector<uint16_t> ECMPIDList;
			std::vector<ESInfo> ESList;
		};

		PIDMapManager m_PIDMapManager;

		uint16_t m_TargetServiceID;
		bool m_TargetStreamTypeEnabled;
		StreamTypeTable m_TargetStreamType;
		bool m_GeneratePAT;

		std::vector<PMTPIDInfo> m_PMTPIDList;
		std::vector<uint16_t> m_EMMPIDList;
		std::array<bool, PID_MAX + 1> m_TargetPIDTable;

		TSPacket m_PATPacket;
		uint16_t m_TargetPMTPID;
		uint16_t m_LastTSID;
		uint16_t m_LastPMTPID;
		uint8_t m_LastVersion;
		uint8_t m_Version;
	};

	LIBISDB_ENUM_FLAGS(StreamSelector::StreamFlag)

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_SELECTOR_H
