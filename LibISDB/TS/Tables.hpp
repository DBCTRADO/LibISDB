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
 @file   Tables.hpp
 @brief  各種テーブル
 @author DBCTRADO
*/


#ifndef LIBISDB_TABLES_H
#define LIBISDB_TABLES_H


#include "PSITable.hpp"
#include "Descriptors.hpp"
#include "DescriptorBlock.hpp"


namespace LibISDB
{

	/** PAT テーブルクラス */
	class PATTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0x00_u8;

	// PSISingleTable
		void Reset() override;

	// PATTable
		uint16_t GetTransportStreamID() const;

		int GetNITCount() const;
		uint16_t GetNITPID(int Index = 0) const;

		int GetProgramCount() const;
		uint16_t GetPMTPID(int Index = 0) const;
		uint16_t GetProgramNumber(int Index = 0) const;

		bool IsPMTTablePID(uint16_t PID) const;

	protected:
	// PSISingleTable
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		/** PAT の項目 */
		struct PATItem {
			uint16_t ProgramNumber; /**< service_id */
			uint16_t PID;           /**< PID */
		};

		std::vector<uint16_t> m_NITList;
		std::vector<PATItem> m_PMTList;
	};

	/** CAT テーブルクラス */
	class CATTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0x01_u8;

	// PSISingleTable
		virtual void Reset() override;

	// CATTable
		const CADescriptor * GetCADescriptorBySystemID(uint16_t SystemID) const;
		uint16_t GetEMMPID() const;
		uint16_t GetEMMPID(uint16_t CASystemID) const;
		const DescriptorBlock * GetCATDescriptorBlock() const;

	protected:
		virtual bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		DescriptorBlock m_DescriptorBlock;
	};

	/** PMT テーブルクラス */
	class PMTTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0x02_u8;

		PMTTable();

	// PSISingleTable
		void Reset() override;

	// PMTTable
		uint16_t GetProgramNumberID() const;

		uint16_t GetPCRPID() const;
		const DescriptorBlock * GetPMTDescriptorBlock() const;
		uint16_t GetECMPID() const;
		uint16_t GetECMPID(uint16_t CASystemID) const;

		int GetESCount() const;
		uint8_t GetStreamType(int Index) const;
		uint16_t GetESPID(int Index) const;
		const DescriptorBlock * GetItemDescriptorBlock(int Index) const;

	protected:
	// PSISingleTable
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		/** PMT の項目 */
		struct PMTItem {
			uint8_t StreamType;          /**< stream_type */
			uint16_t ESPID;              /**< elementary_PID */
			DescriptorBlock Descriptors; /**< ES Descriptor */
		};

		std::vector<PMTItem> m_ESList;

		uint16_t m_PCRPID;                 /**< PCR_PID */
		DescriptorBlock m_DescriptorBlock; /**< 記述子 */
	};

	/** SDT テーブルクラス */
	class SDTTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID_ACTUAL = 0x42_u8;
		static constexpr uint8_t TABLE_ID_OTHER  = 0x46_u8;

		SDTTable(uint8_t TableID = TABLE_ID_ACTUAL);

	// PSISingleTable
		void Reset() override;

	// SDTTable
		uint8_t GetTableID() const;
		uint16_t GetTransportStreamID() const;
		uint16_t GetNetworkID() const;
		int GetServiceCount() const;
		int GetServiceIndexByID(uint16_t ServiceID) const;
		uint16_t GetServiceID(int Index) const;
		bool GetHEITFlag(int Index) const;
		bool GetMEITFlag(int Index) const;
		bool GetLEITFlag(int Index) const;
		bool GetEITScheduleFlag(int Index) const;
		bool GetEITPresentFollowingFlag(int Index) const;
		uint8_t GetRunningStatus(int Index) const;
		bool GetFreeCAMode(int Index) const;
		const DescriptorBlock * GetItemDescriptorBlock(int Index) const;

	protected:
		bool OnPSISection(const PSISectionParser *pSectionParser, const PSISection *pSection) override;
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		/** SDT の項目 */
		struct SDTItem {
			uint16_t ServiceID;           /**< service_id */
			bool HEITFlag;                /**< H-EIT_flag */
			bool MEITFlag;                /**< M-EIT_flag */
			bool LEITFlag;                /**< L-EIT_flag */
			bool EITScheduleFlag;         /**< EIT_schedule_flag */
			bool EITPresentFollowingFlag; /**< EIT_present_following_flag */
			uint8_t RunningStatus;        /**< running_status */
			bool FreeCAMode;              /**< free_CA_mode (true: CA / false: Free) */
			DescriptorBlock Descriptors;  /**< 記述子 */
		};

		uint8_t m_TableID;
		uint16_t m_NetworkID;
		std::vector<SDTItem> m_ServiceList;
	};

	/** SDT(Other) テーブルクラス */
	class SDTOtherTable
		: public PSITable
	{
	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
	};

	/** SDT テーブル集合クラス */
	class SDTTableSet
		: public PSITableSet
	{
	public:
		SDTTableSet();

		const SDTTable * GetActualSDTTable() const;
		const SDTOtherTable * GetOtherSDTTable() const;
	};

	/** NIT テーブルクラス */
	class NITTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0x40_u8;

		NITTable();

	// PSISingleTable
		void Reset() override;

	// NITTable
		uint16_t GetNetworkID() const;
		const DescriptorBlock * GetNetworkDescriptorBlock() const;
		bool GetNetworkName(ReturnArg<ARIBString> Name) const;
		int GetTransportStreamCount() const;
		uint16_t GetTransportStreamID(int Index) const;
		uint16_t GetOriginalNetworkID(int Index) const;
		const DescriptorBlock * GetItemDescriptorBlock(int Index) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		/** NIT の項目 */
		struct NITItem {
			uint16_t TransportStreamID;  /**< transport_stream_id */
			uint16_t OriginalNetworkID;  /**< original_network_id */
			DescriptorBlock Descriptors; /**< 記述子 */
		};

		uint16_t m_NetworkID;                     /**< network_id */
		DescriptorBlock m_NetworkDescriptorBlock; /**< ネットワーク記述子 */
		std::vector<NITItem> m_TransportStreamList;
	};

	/** NIT テーブル集合クラス */
	class NITMultiTable
		: public PSITable
	{
	public:
		uint16_t GetNITSectionCount() const;
		const NITTable * GetNITTable(uint16_t SectionNumber) const;
		bool IsNITComplete() const;

	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
	};

	/** EIT テーブルクラス */
	class EITTable
		: public PSISingleTable
	{
	public:
		/** 番組情報 */
		struct EventInfo {
			uint16_t EventID;            /**< event_id */
			DateTime StartTime;          /**< start_time */
			uint32_t Duration;           /**< duration */
			uint8_t RunningStatus;       /**< running_status */
			bool FreeCAMode;             /**< free_CA_mode */
			DescriptorBlock Descriptors; /**< 記述子 */
		};

		static constexpr uint8_t TABLE_ID_PF_ACTUAL = 0x4E_u8;	// p/f actual
		static constexpr uint8_t TABLE_ID_PF_OTHER  = 0x4F_u8;	// p/f other

		EITTable();

	// PSISingleTable
		void Reset() override;

	// EITTable
		uint16_t GetServiceID() const;
		uint16_t GetTransportStreamID() const;
		uint16_t GetOriginalNetworkID() const;
		uint8_t GetSegmentLastSectionNumber() const;
		uint8_t GetLastTableID() const;
		int GetEventCount() const;
		const EventInfo * GetEventInfo(int Index = 0) const;
		uint16_t GetEventID(int Index = 0) const;
		const DateTime * GetStartTime(int Index = 0) const;
		uint32_t GetDuration(int Index = 0) const;
		uint8_t GetRunningStatus(int Index = 0) const;
		bool GetFreeCAMode(int Index = 0) const;
		const DescriptorBlock * GetItemDescriptorBlock(int Index = 0) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		uint16_t m_ServiceID;               /**< service_id */
		uint16_t m_TransportStreamID;       /**< transport_stream_id */
		uint16_t m_OriginalNetworkID;       /**< original_network_id */
		uint8_t m_SegmentLastSectionNumber; /**< segment_last_section_number */
		uint8_t m_LastTableID;              /**< last_table_id */

		std::vector<EventInfo> m_EventList;
	};

	/** EIT テーブル集合クラス */
	class EITMultiTable
		: public PSITable
	{
	public:
		const EITTable * GetEITTableByServiceID(uint16_t ServiceID, uint16_t SectionNumber) const;
		bool IsEITSectionComplete(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID) const;
		uint16_t GetServiceID(int Index) const;

		static unsigned long long MakeTableUniqueID(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID);

	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
		unsigned long long GetSectionTableUniqueID(const PSISection *pSection) override;
	};

	/** EIT[p/f] テーブルクラス */
	class EITPfTable
		: public PSITableSet
	{
	public:
		EITPfTable();

		const EITMultiTable * GetPfActualTable() const;
		const EITTable * GetPfActualTable(uint16_t ServiceID, bool Following = false) const;
		const EITMultiTable * GetPfOtherTable() const;
		const EITTable * GetPfOtherTable(uint16_t ServiceID, bool Following = false) const;
	};

	/** EIT[p/f] actual テーブルクラス */
	class EITPfActualTable
		: public PSITableSet
	{
	public:
		EITPfActualTable();

		const EITMultiTable * GetPfActualTable() const;
		const EITTable * GetPfActualTable(uint16_t ServiceID, bool Following = false) const;
	};

	/** EIT[p/f schedule] テーブルクラス */
	class EITPfScheduleTable
		: public EITPfTable
	{
	public:
		EITPfScheduleTable();

		const EITTable * GetLastUpdatedEITTable() const;
		bool ResetScheduleService(
			uint16_t NetworkID, uint16_t TransportStreamID, uint16_t ServiceID);
	};

	/** BIT テーブルクラス */
	class BITTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0xC4_u8;

		BITTable();

	// PSISingleTable
		void Reset() override;

	// BITTable
		uint16_t GetOriginalNetworkID() const;
		bool GetBroadcastViewPropriety() const;
		const DescriptorBlock * GetBITDescriptorBlock() const;
		int GetBroadcasterCount() const;
		uint8_t GetBroadcasterID(int Index) const;
		const DescriptorBlock * GetBroadcasterDescriptorBlock(int Index) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		struct BroadcasterInfo {
			uint8_t BroadcasterID;       /**< broadcaster_id */
			DescriptorBlock Descriptors; /**< 記述子 */
		};

		uint16_t m_OriginalNetworkID;      /**< original_network_id */
		bool m_BroadcastViewPropriety;     /**< broadcast_view_propriety */
		DescriptorBlock m_DescriptorBlock; /**< 記述子 */
		std::vector<BroadcasterInfo> m_BroadcasterList;
	};

	/** BIT テーブル集合クラス */
	class BITMultiTable
		: public PSITable
	{
	public:
		uint16_t GetBITSectionCount() const;
		const BITTable * GetBITTable(uint16_t SectionNumber) const;
		bool IsBITComplete() const;

	protected:
	// PSITable
		PSITableBase * CreateSectionTable(const PSISection *pSection) override;
	};

	/** TOT テーブルクラス */
	class TOTTable
		: public PSISingleTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0x73_u8;

		TOTTable();

	// PSISingleTable
		void Reset() override;

	// TOTTable
		bool GetDateTime(ReturnArg<DateTime> Time) const;
		bool GetOffsetDateTime(ReturnArg<DateTime> Time, uint32_t CountryCode, uint8_t CountryRegionID = 0) const;
		int GetLocalTimeOffset(uint32_t CountryCode, uint8_t CountryRegionID = 0) const;
		const DescriptorBlock * GetTOTDescriptorBlock() const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection, const PSISection *pOldSection) override;

		DateTime m_DateTime;               /**< JST_time */
		DescriptorBlock m_DescriptorBlock; /**< 記述子 */
	};

	/** CDT テーブルクラス */
	class CDTTable
		: public PSIStreamTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0xC8_u8;

		// データの種類
		static constexpr uint8_t DATA_TYPE_LOGO    = 0x01_u8;	// ロゴ
		static constexpr uint8_t DATA_TYPE_INVALID = 0xFF_u8;	// 無効

		CDTTable();

	// PSIStreamTable
		void Reset() override;

	// CDTTable
		uint16_t GetOriginalNetworkID() const;
		uint8_t GetDataType() const;
		const DescriptorBlock * GetDescriptorBlock() const;
		uint16_t GetDataModuleSize() const;
		const uint8_t * GetDataModuleData() const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection) override;

		uint16_t m_OriginalNetworkID;      /**< original_network_id */
		uint8_t m_DataType;                /**< data_type */
		DescriptorBlock m_DescriptorBlock; /**< 記述子 */
		DataBuffer m_ModuleData;
	};

	/** SDTT テーブルクラス */
	class SDTTTable
		: public PSIStreamTable
	{
	public:
		static constexpr uint8_t TABLE_ID = 0xC3_u8;

		struct ScheduleDescription {
			DateTime StartTime; /**< start_time */
			uint32_t Duration;  /**< duration */
		};

		struct ContentInfo {
			uint8_t GroupID;                      /**< group_id */
			uint16_t TargetVersion;               /**< target_version */
			uint16_t NewVersion;                  /**< new_version */
			uint8_t DownloadLevel;                /**< donwload_level */
			uint8_t VersionIndicator;             /**< version_indicator */
			uint8_t ScheduleTimeShiftInformation; /**< schedule_time-shift_information */
			std::vector<ScheduleDescription> ScheduleList;
			DescriptorBlock Descriptors;          /**< 記述子 */
		};

		SDTTTable();

	// PSIStreamTable
		void Reset() override;

	// SDTTTable
		uint8_t GetMakerID() const;
		uint8_t GetModelID() const;
		bool IsCommon() const;
		uint16_t GetTransportStreamID() const;
		uint16_t GetOriginalNetworkID() const;
		uint16_t GetServiceID() const;
		uint8_t GetNumOfContents() const;
		const ContentInfo * GetContentInfo(uint8_t Index) const;
		bool IsSchedule(uint32_t Index) const;
		const DescriptorBlock * GetContentDescriptorBlock(uint32_t Index) const;

	protected:
		bool OnTableUpdate(const PSISection *pCurSection) override;

		uint8_t m_MakerID;            /**< maker_id */
		uint8_t m_ModelID;            /**< model_id */
		uint16_t m_TransportStreamID; /**< transport_stream_id */
		uint16_t m_OriginalNetworkID; /**< original_network_id */
		uint16_t m_ServiceID;         /**< service_id */
		std::vector<ContentInfo> m_ContentList;
	};

	/** PCR テーブルクラス */
	class PCRTable
		: public PSINullTable
	{
	public:
		PCRTable();

	// PIDMapTarget
		bool StorePacket(const TSPacket *pPacket) override;

	// PCRTable
		uint64_t GetPCRTimeStamp() const;

	protected:
		uint64_t m_PCR;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_TABLES_H
