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
 @file   Descriptors.hpp
 @brief  各種記述子
 @author DBCTRADO
*/


#ifndef LIBISDB_DESCRIPTORS_H
#define LIBISDB_DESCRIPTORS_H


#include "DescriptorBase.hpp"
#include "../Base/DataBuffer.hpp"
#include "../Base/DateTime.hpp"
#include "../Base/ARIBString.hpp"
#include <vector>


namespace LibISDB
{

	/** 限定受信方式記述子クラス */
	class CADescriptor
		: public DescriptorTemplate<CADescriptor, 0x09>
	{
	public:
		CADescriptor();

	// DescriptorBase
		void Reset() override;

	// CADescriptor
		uint16_t GetCASystemID() const noexcept { return m_CASystemID; }
		uint16_t GetCAPID() const noexcept { return m_CAPID; }
		const DataBuffer * GetPrivateData() const noexcept { return &m_PrivateData; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_CASystemID;    /**< CA_system_ID */
		uint16_t m_CAPID;         /**< CA_PID */
		DataBuffer m_PrivateData; /**< private_data_byte */
	};

	/** ネットワーク名記述子クラス */
	class NetworkNameDescriptor
		: public DescriptorTemplate<NetworkNameDescriptor, 0x40>
	{
	public:
	// DescriptorBase
		void Reset() override;

	// NetworkNameDescriptor
		bool GetNetworkName(ReturnArg<ARIBString> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		ARIBString m_NetworkName;
	};

	/** サービスリスト記述子クラス */
	class ServiceListDescriptor
		: public DescriptorTemplate<ServiceListDescriptor, 0x41>
	{
	public:
		/** サービス情報 */
		struct ServiceInfo {
			uint16_t ServiceID;  /**< service_id */
			uint8_t ServiceType; /**< service_type */
		};

	// DescriptorBase
		void Reset() override;

	// ServiceListDescriptor
		int GetServiceCount() const;
		int GetServiceIndexByID(uint16_t ServiceID) const;
		uint8_t GetServiceTypeByID(uint16_t ServiceID) const;
		bool GetServiceInfo(int Index, ReturnArg<ServiceInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<ServiceInfo> m_ServiceList;
	};

	/** 衛星分配システム記述子クラス */
	class SatelliteDeliverySystemDescriptor
		: public DescriptorTemplate<SatelliteDeliverySystemDescriptor, 0x43>
	{
	public:
		SatelliteDeliverySystemDescriptor();

	// DescriptorBase
		void Reset() override;

	// SatelliteDeliverySystemDescriptor
		uint32_t GetFrequency() const noexcept { return m_Frequency; }
		uint16_t GetOrbitalPosition() const noexcept { return m_OrbitalPosition; }
		bool GetWestEastFlag() const noexcept { return m_WestEastFlag; }
		uint8_t GetPolarization() const noexcept { return m_Polarization; }
		uint8_t GetModulation() const noexcept { return m_Modulation; }
		uint32_t GetSymbolRate() const noexcept { return m_SymbolRate; }
		uint8_t GetFECInner() const noexcept { return m_FECInner; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint32_t m_Frequency;       /**< frequency */
		uint16_t m_OrbitalPosition; /**< orbital_position */
		bool m_WestEastFlag;        /**< west_east_flag */
		uint8_t m_Polarization;     /**< polarisation */
		uint8_t m_Modulation;       /**< modulation */
		uint32_t m_SymbolRate;      /**< symbol_rate */
		uint8_t m_FECInner;         /**< FEC_inner */
	};

	/** 有線分配システム記述子クラス */
	class CableDeliverySystemDescriptor
		: public DescriptorTemplate<CableDeliverySystemDescriptor, 0x44>
	{
	public:
		CableDeliverySystemDescriptor();

	// DescriptorBase
		void Reset() override;

	// CableDeliverySystemDescriptor
		uint32_t GetFrequency() const noexcept { return m_Frequency; }
		uint8_t GetFrameType() const noexcept { return m_FrameType; }
		uint8_t GetFECOuter() const noexcept { return m_FECOuter; }
		uint8_t GetModulation() const noexcept { return m_Modulation; }
		uint32_t GetSymbolRate() const noexcept { return m_SymbolRate; }
		uint8_t GetFECInner() const noexcept { return m_FECInner; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint32_t m_Frequency;       /**< frequency */
		uint8_t m_FrameType;        /**< frame_type */
		uint8_t m_FECOuter;         /**< FEC_outer */
		uint8_t m_Modulation;       /**< modulation */
		uint32_t m_SymbolRate;      /**< symbol_rate */
		uint8_t m_FECInner;         /**< FEC_inner */
	};

	/** サービス記述子クラス */
	class ServiceDescriptor
		: public DescriptorTemplate<ServiceDescriptor, 0x48>
	{
	public:
		ServiceDescriptor();

	// DescriptorBase
		void Reset() override;

	// ServiceDescriptor
		uint8_t GetServiceType() const noexcept { return m_ServiceType; }
		bool GetProviderName(ReturnArg<ARIBString> Name) const;
		bool GetServiceName(ReturnArg<ARIBString> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_ServiceType;     /**< service_type */
		ARIBString m_ProviderName; /**< service_provider_name */
		ARIBString m_ServiceName;  /**< service_name */
	};

	/** リンク記述子クラス */
	class LinkageDescriptor
		: public DescriptorTemplate<LinkageDescriptor, 0x4A>
	{
	public:
		LinkageDescriptor();

	// DescriptorBase
		void Reset() override;

	// LinkageDescriptor
		uint16_t GetTransportStreamID() const noexcept { return m_TransportStreamID; }
		uint16_t GetOriginalNetworkID() const noexcept { return m_OriginalNetworkID; }
		uint16_t GetServiceID() const noexcept { return m_ServiceID; }
		uint8_t GetLinkageType() const noexcept { return m_LinkageType; }
		const DataBuffer * GetPrivateData() const noexcept { return &m_PrivateData; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_TransportStreamID; /**< transport_stream_id */
		uint16_t m_OriginalNetworkID; /**< original_network_id */
		uint16_t m_ServiceID;         /**< service_id */
		uint8_t m_LinkageType;        /**< linkage_type */
		DataBuffer m_PrivateData;     /**< private_data_byte */
	};

	/** 短形式イベント記述子クラス */
	class ShortEventDescriptor
		: public DescriptorTemplate<ShortEventDescriptor, 0x4D>
	{
	public:
		ShortEventDescriptor();

	// DescriptorBase
		void Reset() override;

	// ShortEventDescriptor
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		bool GetEventName(ReturnArg<ARIBString> Name) const;
		bool GetEventDescription(ReturnArg<ARIBString> Description) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint32_t m_LanguageCode;       /**< ISO_639_language_code */
		ARIBString m_EventName;        /**< event_name */
		ARIBString m_EventDescription; /**< text */
	};

	/** 拡張形式イベント記述子クラス */
	class ExtendedEventDescriptor
		: public DescriptorTemplate<ExtendedEventDescriptor, 0x4E>
	{
	public:
		struct ItemInfo {
			ARIBString Description;
			ARIBString ItemChar;
		};

		ExtendedEventDescriptor();

	// DescriptorBase
		void Reset() override;

	// ExtendedEventDescriptor
		uint8_t GetDescriptorNumber() const noexcept { return m_DescriptorNumber; }
		uint8_t GetLastDescriptorNumber() const noexcept { return m_LastDescriptorNumber; }
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		int GetItemCount() const;
		const ItemInfo * GetItem(int Index) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_DescriptorNumber;       /**< descriptor_number */
		uint8_t m_LastDescriptorNumber;   /**< last_descriptor_number */
		uint32_t m_LanguageCode;          /**< ISO_639_language_code */
		std::vector<ItemInfo> m_ItemList; /**< 項目 */
	};

	/** コンポーネント記述子クラス */
	class ComponentDescriptor
		: public DescriptorTemplate<ComponentDescriptor, 0x50>
	{
	public:
		ComponentDescriptor();

	// DescriptorBase
		void Reset() override;

	// ComponentDescriptor
		uint8_t GetStreamContent() const noexcept { return m_StreamContent; }
		uint8_t GetComponentType() const noexcept { return m_ComponentType; }
		uint8_t GetComponentTag() const noexcept { return m_ComponentTag; }
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		bool GetText(ReturnArg<ARIBString> Text) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_StreamContent; /**< stream_content */
		uint8_t m_ComponentType; /**< component_type */
		uint8_t m_ComponentTag;  /**< component_tag */
		uint32_t m_LanguageCode; /**< ISO_639_language_code */
		ARIBString m_Text;       /**< text_char */
	};

	/** ストリーム識別記述子クラス */
	class StreamIDDescriptor
		: public DescriptorTemplate<StreamIDDescriptor, 0x52>
	{
	public:
		StreamIDDescriptor();

	// DescriptorBase
		void Reset() override;

	// StreamIDDescriptor
		uint8_t GetComponentTag() const noexcept { return m_ComponentTag; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_ComponentTag; /**< component_tag */
	};

	/** コンテント記述子クラス */
	class ContentDescriptor
		: public DescriptorTemplate<ContentDescriptor, 0x54>
	{
	public:
		/** コンテント分類 */
		struct NibbleInfo {
			uint8_t ContentNibbleLevel1; /**< content_nibble_level_1 */
			uint8_t ContentNibbleLevel2; /**< content_nibble_level_2 */
			uint8_t UserNibble1;         /**< user_nibble */
			uint8_t UserNibble2;         /**< user_nibble */

			bool operator == (const NibbleInfo &rhs) const noexcept
			{
				return (ContentNibbleLevel1 == rhs.ContentNibbleLevel1)
					&& (ContentNibbleLevel2 == rhs.ContentNibbleLevel2)
					&& (UserNibble1 == rhs.UserNibble1)
					&& (UserNibble2 == rhs.UserNibble2);
			}

			bool operator != (const NibbleInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		ContentDescriptor();

	// DescriptorBase
		void Reset() override;

	// ContentDescriptor
		int GetNibbleCount() const;
		bool GetNibble(int Index, ReturnArg<NibbleInfo> Nibble) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		int m_NibbleCount;
		NibbleInfo m_NibbleList[7];
	};

	/** ローカル時間オフセット記述子クラス */
	class LocalTimeOffsetDescriptor
		: public DescriptorTemplate<LocalTimeOffsetDescriptor, 0x58>
	{
	public:
		static constexpr uint32_t COUNTRY_CODE_JPN = 0x4A504E_u32;
		static constexpr uint8_t COUNTRY_REGION_ALL = 0x00_u8;

		struct TimeOffsetInfo {
			uint32_t CountryCode;         /**< country_code */
			uint8_t CountryRegionID;      /**< country_region_id */
			bool LocalTimeOffsetPolarity; /**< local_time_offset_polarity */
			uint16_t LocalTimeOffset;     /**< local_time_offset */
			DateTime TimeOfChange;        /**< time_of_change */
			uint16_t NextTimeOffset;      /**< next_time_offset */
		};

	// DescriptorBase
		void Reset() override;

	// LocalTimeOffsetDescriptor
		int GetTimeOffsetInfoCount() const;
		bool GetTimeOffsetInfo(int Index, ReturnArg<TimeOffsetInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<TimeOffsetInfo> m_TimeOffsetList;
	};

	/** 階層伝送記述子クラス */
	class HierarchicalTransmissionDescriptor
		: public DescriptorTemplate<HierarchicalTransmissionDescriptor, 0xC0>
	{
	public:
		HierarchicalTransmissionDescriptor();

	// DescriptorBase
		void Reset() override;

	// HierarchicalTransmissionDescriptor
		uint8_t GetQualityLevel() const noexcept { return m_QualityLevel; }
		uint16_t GetReferencePID() const noexcept { return m_ReferencePID; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_QualityLevel;  /**< quality_level */
		uint16_t m_ReferencePID; /**< reference_PID */
	};

	/** デジタルコピー制御記述子 */
	class DigitalCopyControlDescriptor
		: public DescriptorTemplate<DigitalCopyControlDescriptor, 0xC1>
	{
	public:
		struct ComponentControlInfo {
			uint8_t ComponentTag;                /**< component_tag */
			uint8_t DigitalRecordingControlData; /**< digital_recording_control_data */
			bool MaximumBitRateFlag;             /**< maximum_bitrate_flag */
			uint8_t CopyControlType;             /**< copy_control_type */
			uint8_t APSControlData;              /**< APS_control_data */
			uint8_t MaximumBitRate;              /**< maximum_bitrate */
		};

		DigitalCopyControlDescriptor();

	// DescriptorBase
		void Reset() override;

	// DigitalCopyControlDescriptor
		uint8_t GetDigitalRecordingControlData() const noexcept { return m_DigitalRecordingControlData; }
		bool GetMaximumBitRateFlag() const noexcept { return m_MaximumBitRateFlag; }
		bool GetComponentControlFlag() const noexcept { return m_ComponentControlFlag; }
		uint8_t GetCopyControlType() const noexcept { return m_CopyControlType; }
		uint8_t GetAPSControlData() const noexcept { return m_APSControlData; }
		uint8_t GetMaximumBitRate() const noexcept { return m_MaximumBitRate; }
		int GetComponentControlCount() const;
		bool GetComponentControlInfo(int Index, ReturnArg<ComponentControlInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_DigitalRecordingControlData; /**< digital_recording_control_data */
		bool m_MaximumBitRateFlag;             /**< maximum_bit_rate_flag */
		bool m_ComponentControlFlag;           /**< compoent_control_flag */
		uint8_t m_CopyControlType;             /**< copy_control_type */
		uint8_t m_APSControlData;              /**< APS_control_data */
		uint8_t m_MaximumBitRate;              /**< maximum_bit_rate */
		std::vector<ComponentControlInfo> m_ComponentControlList;
	};

	/** 音声コンポーネント記述子クラス */
	class AudioComponentDescriptor
		: public DescriptorTemplate<AudioComponentDescriptor, 0xC4>
	{
	public:
		AudioComponentDescriptor();

	// DescriptorBase
		void Reset() override;

	// AudioComponentDescriptor
		uint8_t GetStreamContent() const noexcept { return m_StreamContent; }
		uint8_t GetComponentType() const noexcept { return m_ComponentType; }
		uint8_t GetComponentTag() const noexcept { return m_ComponentTag; }
		uint8_t GetSimulcastGroupTag() const noexcept { return m_SimulcastGroupTag; }
		bool GetESMultiLingualFlag() const noexcept { return m_ESMultiLingualFlag; }
		bool GetMainComponentFlag() const noexcept { return m_MainComponentFlag; }
		uint8_t GetQualityIndicator() const noexcept { return m_QualityIndicator; }
		uint8_t GetSamplingRate() const noexcept { return m_SamplingRate; }
		uint32_t GetLanguageCode() const noexcept { return m_LanguageCode; }
		uint32_t GetLanguageCode2() const noexcept { return m_LanguageCode2; }
		bool GetText(ReturnArg<ARIBString> Text) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_StreamContent;     /**< stream_content */
		uint8_t m_ComponentType;     /**< component_type */
		uint8_t m_ComponentTag;      /**< component_tag */
		uint8_t m_StreamType;        /**< stream_type */
		uint8_t m_SimulcastGroupTag; /**< simulcast_group_tag */
		bool m_ESMultiLingualFlag;   /**< ES_multi_lingual_flag */
		bool m_MainComponentFlag;    /**< main_component_flag */
		uint8_t m_QualityIndicator;  /**< quality_indicator */
		uint8_t m_SamplingRate;      /**< sampling_rate */
		uint32_t m_LanguageCode;     /**< ISO_639_language_code */
		uint32_t m_LanguageCode2;    /**< ISO_639_language_code_2 */
		ARIBString m_Text;           /**< text_char */
	};

	/** ハイパーリンク記述子クラス */
	class HyperLinkDescriptor
		: public DescriptorTemplate<HyperLinkDescriptor, 0xC5>
	{
	public:
		// hyper_linkage_type
		static constexpr uint8_t HYPER_LINKAGE_TYPE_COMBINED_DATA       = 0x01_u8; /**< combined_data */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_COMBINED_STREAM     = 0x02_u8; /**< combined_stream */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_CONTENT_TO_INDEX    = 0x03_u8; /**< content_to_index */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_INDEX_TO_CONTENT    = 0x04_u8; /**< index_to_content */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_GUIDE_DATA          = 0x05_u8; /**< guide_data */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_CONTENT_TO_METADATA = 0x07_u8; /**< content_to_metadata */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_METADATA_TO_CONTENT = 0x08_u8; /**< metadata_to_content */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_PORTAL_URI          = 0x09_u8; /**< portal_URI */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_AUTHORITY_URI       = 0x0A_u8; /**< authority_URI */
		static constexpr uint8_t HYPER_LINKAGE_TYPE_INDEX_MODULE        = 0x40_u8; /**< index_module */

		// link_destination_type
		static constexpr uint8_t LINK_DESTINATION_TYPE_LINK_TO_SERVICE        = 0x01_u8; /**< link_to_service */
		static constexpr uint8_t LINK_DESTINATION_TYPE_LINK_TO_EVENT          = 0x02_u8; /**< link_to_event */
		static constexpr uint8_t LINK_DESTINATION_TYPE_LINK_TO_MODULE         = 0x03_u8; /**< link_to_module */
		static constexpr uint8_t LINK_DESTINATION_TYPE_LINK_TO_CONTENT        = 0x04_u8; /**< link_to_content */
		static constexpr uint8_t LINK_DESTINATION_TYPE_LINK_TO_CONTENT_MODULE = 0x05_u8; /**< link_to_content_module */
		static constexpr uint8_t LINK_DESTINATION_TYPE_LINK_TO_ERT_MODE       = 0x06_u8; /**< link_to_ert_node */
		static constexpr uint8_t LINK_DESTINATION_TYPE_LINK_TO_STORED_CONTENT = 0x07_u8; /**< link_to_stored_content */

		struct SelectorInfo {
			uint8_t SelectorLength; /**< selector_length */

			/** link_service_info */
			struct {
				uint16_t OriginalNetworkID; /**< original_network_id */
				uint16_t TransportStreamID; /**< transport_stream_id */
				uint16_t ServiceID;         /**< service_id */
			} LinkServiceInfo;

			/** link_event_info */
			struct {
				uint16_t OriginalNetworkID; /**< original_network_id */
				uint16_t TransportStreamID; /**< transport_stream_id */
				uint16_t ServiceID;         /**< service_id */
				uint16_t EventID;           /**< event_id */
			} LinkEventInfo;

			/** link_module_info */
			struct {
				uint16_t OriginalNetworkID; /**< original_network_id */
				uint16_t TransportStreamID; /**< transport_stream_id */
				uint16_t ServiceID;         /**< service_id */
				uint16_t EventID;           /**< event_id */
				uint8_t ComponentTag;       /**< component_tag */
				uint16_t ModuleID;          /**< moduleId */
			} LinkModuleInfo;

			/** link_content_info */
			struct {
				uint16_t OriginalNetworkID; /**< original_network_id */
				uint16_t TransportStreamID; /**< transport_stream_id */
				uint16_t ServiceID;         /**< service_id */
				uint32_t ContentID;         /**< content_id */
			} LinkContentInfo;

			/** link_content_module_info */
			struct {
				uint16_t OriginalNetworkID; /**< original_network_id */
				uint16_t TransportStreamID; /**< transport_stream_id */
				uint16_t ServiceID;         /**< service_id */
				uint32_t ContentID;         /**< content_id */
				uint8_t ComponentTag;       /**< component_tag */
				uint16_t ModuleID;          /**< moduleId */
			} LinkContentModuleInfo;

			/** link_ert_node_info */
			struct {
				uint16_t InformationProviderID; /**< information_provider_id */
				uint16_t TransportStreamID;     /**< event_relation_id */
				uint16_t NodeID;                /**< node_id */
			} LinkERTNodeInfo;

			/** link_stored_content_info */
			struct {
				uint8_t URIChar[255];           /**< uri_char */
			} LinkStoredContentInfo;
		};

		HyperLinkDescriptor();

	// DescriptorBase
		void Reset() override;

	// HyperLinkDescriptor
		uint8_t GetHyperLinkageType() const noexcept { return m_HyperLinkageType; }
		uint8_t GetLinkDestinationType() const noexcept { return m_LinkDestinationType; }
		bool GetSelectorInfo(ReturnArg<SelectorInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_HyperLinkageType;    /**< hyper_linkage_type */
		uint8_t m_LinkDestinationType; /**< link_destination_type */
		SelectorInfo m_SelectorInfo;
	};

	/** 対象地域記述子クラス */
	class TargetRegionDescriptor
		: public DescriptorTemplate<TargetRegionDescriptor, 0xC6>
	{
	public:
		static constexpr uint8_t REGION_SPEC_TYPE_BS = 0x01_u8;

		/** bs_prefecture_spec */
		struct BSPrefectureSpec {
			uint8_t PrefectureBitmap[7]; /**< prefecture_bitmap */
		};

		/** target_region_spec */
		struct TargetRegionSpec {
			BSPrefectureSpec BS; /** bs_prefecture_spec */
		};

		TargetRegionDescriptor();

	// DescriptorBase
		void Reset() override;

	// TargetRegionDescriptor
		uint8_t GetRegionSpecType() const noexcept { return m_RegionSpecType; }
		bool GetTargetRegionSpec(ReturnArg<TargetRegionSpec> Spec) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_RegionSpecType;            /**< region_spec_type */
		TargetRegionSpec m_TargetRegionSpec; /**< target_region_spec */
	};

	/** ビデオデコードコントロール記述子クラス */
	class VideoDecodeControlDescriptor
		: public DescriptorTemplate<VideoDecodeControlDescriptor, 0xC8>
	{
	public:
		VideoDecodeControlDescriptor();

	// DescriptorBase
		void Reset() override;

	// VideoDecodeControlDescriptor
		bool GetStillPictureFlag() const noexcept { return m_StillPictureFlag; }
		bool GetSequenceEndCodeFlag() const noexcept { return m_SequenceEndCodeFlag; }
		uint8_t GetVideoEncodeFormat() const noexcept { return m_VideoEncodeFormat; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		bool m_StillPictureFlag;     /**< still_picture_flag */
		bool m_SequenceEndCodeFlag;  /**< sequence_end_code_flag */
		uint8_t m_VideoEncodeFormat; /**< video_encode_format */
	};

	/** ダウンロードコンテンツ記述子クラス */
	class DownloadContentDescriptor
		: public DescriptorTemplate<DownloadContentDescriptor, 0xC9>
	{
	public:
		DownloadContentDescriptor();

	// DescriptorBase
		void Reset() override;

	// DownloadContentDescriptor
		bool GetReboot() const noexcept { return m_Info.Reboot; }
		bool GetAddOn() const noexcept { return m_Info.AddOn; }
		uint32_t GetComponentSize() const noexcept { return m_Info.ComponentSize; }
		uint32_t GetDownloadID() const noexcept { return m_Info.DownloadID; }
		uint32_t GetTimeOutValueDII() const noexcept { return m_Info.TimeOutValueDII; }
		uint32_t GetLeakRate() const noexcept { return m_Info.LeakRate; }
		uint8_t GetComponentTag() const noexcept { return m_Info.ComponentTag; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		struct CompatibilityDescriptorInfo {
			struct SubDescriptorInfo {
				uint8_t SubDescriptorType;        /**< subDescriptorType */
				DataBuffer AdditionalInformation; /**< additionalInformation */
			};

			struct DescriptorInfo {
				uint8_t DescriptorType;                           /**< descriptorType */
				uint8_t SpecifierType;                            /**< specifierType */
				uint32_t SpecifierData;                           /**< specifierData */
				uint16_t Model;                                   /**< model */
				uint16_t Version;                                 /**< version */
				std::vector<SubDescriptorInfo> SubDescriptorList; /**< subDescriptor */
			};

			std::vector<DescriptorInfo> DescriptorList;
		};

		struct ModuleInfo {
			uint16_t ModuleID;         /**< module_id */
			uint32_t ModuleSize;       /**< module_size */
			DataBuffer ModuleInfoByte; /**< module_info_byte */
		};

		struct DownloadContentInfo {
			bool Reboot;                                         /**< reboot */
			bool AddOn;                                          /**< add_on */
			bool CompatibilityFlag;                              /**< compatibility_flag */
			bool ModuleInfoFlag;                                 /**< module_info_flag */
			bool TextInfoFlag;                                   /**< text_info_flag */
			uint32_t ComponentSize;                              /**< component_size */
			uint32_t DownloadID;                                 /**< download_id */
			uint32_t TimeOutValueDII;                            /**< time_out_value_DII */
			uint32_t LeakRate;                                   /**< leak_rate */
			uint8_t ComponentTag;                                /**< component_tag */
			CompatibilityDescriptorInfo CompatibilityDescriptor; /**< compatibilityDescriptor */
			std::vector<ModuleInfo> ModuleList;
			DataBuffer PrivateData;                              /**< private_data_byte */
			uint32_t LanguageCode;                               /**< ISO_639_language_code */
			ARIBString Text;                                     /**< text_char */
		};

		DownloadContentInfo m_Info;
	};

	/** CA EMM TS 記述子 */
	class CAEMMTSDescriptor
		: public DescriptorTemplate<CAEMMTSDescriptor, 0xCA>
	{
	public:
		CAEMMTSDescriptor();

	// DescriptorBase
		void Reset() override;

	// CAEMMTSDescriptor
		uint16_t GetCASystemID() const noexcept { return m_CASystemID; }
		uint16_t GetTransportStreamID() const noexcept { return m_TransportStreamID; }
		uint16_t GetOriginalNetworkID() const noexcept { return m_OriginalNetworkID; }
		uint8_t GetPowerSupplyPeriod() const noexcept { return m_PowerSupplyPeriod; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_CASystemID;
		uint16_t m_TransportStreamID;
		uint16_t m_OriginalNetworkID;
		uint8_t m_PowerSupplyPeriod;
	};

	/** CA契約情報記述子クラス */
	class CAContractInfoDescriptor
		: public DescriptorTemplate<CAContractInfoDescriptor, 0xCB>
	{
	public:
		enum {
			MAX_NUM_OF_COMPONENT = 12,
			MAX_VERIFICATION_INFO_LENGTH = 172,
		};

		CAContractInfoDescriptor();

	// DescriptorBase
		void Reset() override;

	// CAContractInfoDescriptor
		uint16_t GetCASystemID() const noexcept { return m_CASystemID; }
		uint8_t GetCAUnitID() const noexcept { return m_CAUnitID; }
		uint8_t GetNumOfComponent() const noexcept { return m_NumOfComponent; }
		uint8_t GetComponentTag(uint8_t Index) const noexcept;
		uint8_t GetContractVerificationInfoLength() const noexcept { return m_ContractVerificationInfoLength; }
		uint8_t GetContractVerificationInfo(uint8_t *pInfo, uint8_t MaxLength) const;
		bool GetFeeName(ReturnArg<ARIBString> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_CASystemID;                                            /**< CA_system_id */
		uint8_t m_CAUnitID;                                               /**< CA_unit_id */
		uint8_t m_NumOfComponent;                                         /**< num_of_component */
		uint8_t m_ComponentTag[MAX_NUM_OF_COMPONENT];                     /**< component_tag */
		uint8_t m_ContractVerificationInfoLength;                         /**< contract_verification_info_length */
		uint8_t m_ContractVerificationInfo[MAX_VERIFICATION_INFO_LENGTH]; /**< contract_verification_info */
		ARIBString m_FeeName;                                             /**< fee_name */
	};

	/** CAサービス記述子クラス */
	class CAServiceDescriptor
		: public DescriptorTemplate<CAServiceDescriptor, 0xCC>
	{
	public:
		CAServiceDescriptor();

	// DescriptorBase
		void Reset() override;

	// CAServiceDescriptor
		uint16_t GetCASystemID() const noexcept { return m_CASystemID; }
		uint8_t GetCABroadcasterGroupID() const noexcept { return m_CABroadcasterGroupID; }
		uint8_t GetMessageControl() const noexcept { return m_MessageControl; }
		int GetServiceIDCount() const;
		uint16_t GetServiceID(int Index) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_CASystemID;                 /**< CA_system_ID */
		uint8_t m_CABroadcasterGroupID;        /**< CA_broadcaster_group_id */
		uint8_t m_MessageControl;              /**< message_control */
		std::vector<uint16_t> m_ServiceIDList; /**< service_id */
	};

	/** TS情報記述子クラス */
	class TSInformationDescriptor
		: public DescriptorTemplate<TSInformationDescriptor, 0xCD>
	{
	public:
		TSInformationDescriptor();

	// DescriptorBase
		void Reset() override;

	// TSInformationDescriptor
		uint8_t GetRemoteControlKeyID() const noexcept { return m_RemoteControlKeyID; }
		bool GetTSName(ReturnArg<ARIBString> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_RemoteControlKeyID; /**< remote_control_key_id */
		ARIBString m_TSName;          /**< ts_name_char */
	};

	/** 拡張ブロードキャスタ記述子クラス */
	class ExtendedBroadcasterDescriptor
		: public DescriptorTemplate<ExtendedBroadcasterDescriptor, 0xCE>
	{
	public:
		static constexpr uint8_t BROADCASTER_TYPE_TERRESTRIAL       = 0x01_u8;
		static constexpr uint8_t BROADCASTER_TYPE_TERRESTRIAL_SOUND = 0x02_u8;

		/** 地上デジタルテレビジョン放送ブロードキャスタの情報 */
		struct TerrestrialBroadcasterInfo {
			uint16_t TerrestrialBroadcasterID; /**< terrestrial_broadcaster_id */
			uint8_t NumberOfAffiliationIDLoop; /**< number_of_affiliation_id_loop */
			uint8_t NumberOfBroadcasterIDLoop; /**< number_of_broadcaster_id_loop */
			uint8_t AffiliationIDList[15];     /**< affiliation_id */
			struct {
				uint16_t OriginalNetworkID;    /**< original_network_id */
				uint8_t BroadcasterID;         /**< broadcaster_id */
			} BroadcasterIDList[15];
		};

		ExtendedBroadcasterDescriptor();

	// DescriptorBase
		void Reset() override;

	// ExtendedBroadcasterDescriptor
		uint8_t GetBroadcasterType() const noexcept { return m_BroadcasterType; }
		bool GetTerrestrialBroadcasterInfo(ReturnArg<TerrestrialBroadcasterInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_BroadcasterType; /**< broadcaster_type */
		TerrestrialBroadcasterInfo m_TerrestrialBroadcasterInfo;
	};

	/** ロゴ伝送記述子クラス */
	class LogoTransmissionDescriptor
		: public DescriptorTemplate<LogoTransmissionDescriptor, 0xCF>
	{
	public:
		// logo_transmission_type
		enum {
			TRANSMISSION_UNDEFINED,
			TRANSMISSION_CDT1,		// CDT伝送方式1
			TRANSMISSION_CDT2,		// CDT伝送方式2
			TRANSMISSION_CHAR		// 簡易ロゴ方式
		};

		static constexpr size_t MAX_LOGO_CHAR = 12;		// 最大簡易ロゴ長
		static constexpr uint16_t LOGO_ID_INVALID      = 0xFFFF_u16;	// 無効な logo_id
		static constexpr uint16_t LOGO_VERSION_INVALID = 0xFFFF_u16;	// 無効な logo_version
		static constexpr uint16_t DATA_ID_INVALID      = 0xFFFF_u16;	// 無効な download_data_id

		LogoTransmissionDescriptor();

	// DescriptorBase
		void Reset() override;

	// LogoTransmissionDescriptor
		uint8_t GetLogoTransmissionType() const noexcept { return m_LogoTransmissionType; }
		uint16_t GetLogoID() const noexcept { return m_LogoID; }
		uint16_t GetLogoVersion() const noexcept { return m_LogoVersion; }
		uint16_t GetDownloadDataID() const noexcept { return m_DownloadDataID; }
		size_t GetLogoChar(char *pChar, size_t MaxLength) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_LogoTransmissionType; /**< logo_transmission_type */
		uint16_t m_LogoID;              /**< logo_id */
		uint16_t m_LogoVersion;         /**< logo_version */
		uint16_t m_DownloadDataID;      /**< download_data_id */
		char m_LogoChar[MAX_LOGO_CHAR]; /**< logo_char */
	};

	/** シリーズ記述子クラス */
	class SeriesDescriptor
		: public DescriptorTemplate<SeriesDescriptor, 0xD5>
	{
	public:
		enum {
			PROGRAM_PATTERN_IRREGULAR,                // 不定期
			PROGRAM_PATTERN_ACROSS_THE_BOARD,         // 帯番組
			PROGRAM_PATTERN_WEEKLY,                   // 週一回
			PROGRAM_PATTERN_MONTHLY,                  // 月一回
			PROGRAM_PATTERN_MULTIPLE_EPISODES_IN_DAY, // 同日内に複数話編成
			PROGRAM_PATTERN_DIVISION_LONG_PROGRAM,    // 長時間番組の分割
			PROGRAM_PATTERN_INVALID = 0xFF
		};

		static constexpr uint16_t SERIES_ID_INVALID = 0xFFFF_u16;

		SeriesDescriptor();

	// DescriptorBase
		void Reset() override;

	// SeriesDescriptor
		uint16_t GetSeriesID() const noexcept { return m_SeriesID; }
		uint8_t GetRepeatLabel() const noexcept { return m_RepeatLabel; }
		uint8_t GetProgramPattern() const noexcept { return m_ProgramPattern; }
		bool IsExpireDateValid() const noexcept { return m_ExpireDateValidFlag; }
		bool GetExpireDate(ReturnArg<DateTime> Date) const;
		uint16_t GetEpisodeNumber() const noexcept { return m_EpisodeNumber; }
		uint16_t GetLastEpisodeNumber() const noexcept { return m_LastEpisodeNumber; }
		bool GetSeriesName(ReturnArg<ARIBString> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_SeriesID;          /**< series_id */
		uint8_t m_RepeatLabel;        /**< repeat_label */
		uint8_t m_ProgramPattern;     /**< program_pattern */
		bool m_ExpireDateValidFlag;   /**< expire_date_valid_flag */
		DateTime m_ExpireDate;        /**< expire_date */
		uint16_t m_EpisodeNumber;     /**< episode_number */
		uint16_t m_LastEpisodeNumber; /**< last_episode_number */
		ARIBString m_SeriesName;      /**< series_name_char */
	};

	/** イベントグループ記述子クラス */
	class EventGroupDescriptor
		: public DescriptorTemplate<EventGroupDescriptor, 0xD6>
	{
	public:
		static constexpr uint8_t GROUP_TYPE_UNDEFINED                   = 0x00_u8;
		static constexpr uint8_t GROUP_TYPE_COMMON                      = 0x01_u8;
		static constexpr uint8_t GROUP_TYPE_RELAY                       = 0x02_u8;
		static constexpr uint8_t GROUP_TYPE_MOVEMENT                    = 0x03_u8;
		static constexpr uint8_t GROUP_TYPE_RELAY_TO_OTHER_NETWORK      = 0x04_u8;
		static constexpr uint8_t GROUP_TYPE_MOVEMENT_FROM_OTHER_NETWORK = 0x05_u8;

		struct EventInfo {
			uint16_t ServiceID;         /**< service_id */
			uint16_t EventID;           /**< event_id */
			uint16_t NetworkID;         /**< original_network_id */
			uint16_t TransportStreamID; /**< transport_stream_id */

			bool operator == (const EventInfo &rhs) const noexcept
			{
				return (ServiceID == rhs.ServiceID)
					&& (EventID == rhs.EventID)
					&& (NetworkID == rhs.NetworkID)
					&& (TransportStreamID == rhs.TransportStreamID);
			}

			bool operator != (const EventInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}
		};

		EventGroupDescriptor();

	// DescriptorBase
		void Reset() override;

	// EventGroupDescriptor
		uint8_t GetGroupType() const noexcept { return m_GroupType; }
		int GetEventCount() const;
		bool GetEventInfo(int Index, ReturnArg<EventInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_GroupType;                /**< group_type */
		std::vector<EventInfo> m_EventList;
	};

	/** SI伝送パラメータ記述子クラス */
	class SIParameterDescriptor
		: public DescriptorTemplate<SIParameterDescriptor, 0xD7>
	{
	public:
		static constexpr uint8_t TABLE_ID_NIT                    = 0x40_u8; /**< NIT */
		static constexpr uint8_t TABLE_ID_SDT_ACTUAL             = 0x42_u8; /**< SDT[actual] */
		static constexpr uint8_t TABLE_ID_SDT_OTHER              = 0x46_u8; /**< SDT[other] */
		static constexpr uint8_t TABLE_ID_EIT_PF_ACTUAL          = 0x4E_u8; /**< EIT[p/f actual] */
		static constexpr uint8_t TABLE_ID_EIT_PF_OTHER           = 0x4F_u8; /**< EIT[p/f other] */
		static constexpr uint8_t TABLE_ID_EIT_SCHEDULE_ACTUAL    = 0x50_u8; /**< EIT[schedule actual] */
		static constexpr uint8_t TABLE_ID_EIT_SCHEDULE_EXTENDED  = 0x58_u8; /*<< EIT[schedule extended] */
		static constexpr uint8_t TABLE_ID_EIT_SCHEDULE_OTHER     = 0x60_u8; /**< EIT[schedule other] */
		static constexpr uint8_t TABLE_ID_SDTT                   = 0xC3_u8; /**< SDTT */
		static constexpr uint8_t TABLE_ID_BIT                    = 0xC4_u8; /**< BIT */
		static constexpr uint8_t TABLE_ID_NBIT_MSG               = 0xC5_u8; /**< NBIT[msg] */
		static constexpr uint8_t TABLE_ID_NBIT_REF               = 0xC6_u8; /**< NBIT[ref] */
		static constexpr uint8_t TABLE_ID_LDT                    = 0xC7_u8; /**< LDT */
		static constexpr uint8_t TABLE_ID_CDT                    = 0xC8_u8; /**< CDT */

		struct TableInfo {
			uint8_t TableID;                  /**< table_id */

			union {
				// NIT
				struct {
					uint8_t TableCycle;       /**< table_cycle */
				} NIT;

				// SDT
				struct {
					uint8_t TableCycle;       /**< table_cycle */
				} SDT;

				// EIT[p/f]
				struct {
					uint8_t TableCycle;       /**< table_cycle */
				} EIT_PF;

				// SDTT
				struct {
					uint16_t TableCycle;      /**< table_cycle */
				} SDTT;

				// BIT
				struct {
					uint8_t TableCycle;       /**< table_cycle */
				} BIT;

				// NBIT
				struct {
					uint8_t TableCycle;       /**< table_cycle */
				} NBIT;

				// LDT
				struct {
					uint16_t TableCycle;      /**< table_cycle */
				} LDT;

				// CDT
				struct {
					uint16_t TableCycle;      /**< table_cycle */
				} CDT;

				// H-EIT[p/f], M-EIT, L-EIT
				struct {
					uint8_t HEITTableCycle;   /**< table_cycle(H-EIT[p/f]) */
					uint8_t MEITTableCycle;   /**< table_cycle(M-EIT) */
					uint8_t LEITTableCycle;   /**< table_cycle(L-EIT) */
					uint8_t NumOfMEITEvent;   /**< num_of_M-EIT_event */
					uint8_t NumOfLEITEvent;   /**< num_of_L-EIT_event */
				} HMLEIT;

				// EIT[schedule]
				struct {
					uint8_t MediaTypeCount;
					struct {
						uint8_t MediaType;        /**< media_type */
						uint8_t Pattern;          /**< pattern */
						bool EITOtherFlag;        /**< EIT_other_flag */
						uint8_t ScheduleRange;    /**< schedule_range */
						uint16_t BaseCycle;       /**< base_cycle */
						uint8_t CycleGroupCount;  /**< cycle_group_count */
						struct {
							uint8_t NumOfSegment; /**< num_of_segment */
							uint8_t Cycle;        /**< cycle */
						} CycleGroup[3];
					} MediaTypeList[3];
				} HEIT_Schedule;
			};
		};

		SIParameterDescriptor();

	// DescriptorBase
		void Reset() override;

	// SIParameterDescriptor
		uint8_t GetParameterVersion() const noexcept { return m_ParameterVersion; }
		bool GetUpdateTime(ReturnArg<DateTime> Time) const;
		int GetTableCount() const;
		bool GetTableInfo(int Index, ReturnArg<TableInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_ParameterVersion; /**< parameter_version */
		DateTime m_UpdateTime;      /**< update_time */
		std::vector<TableInfo> m_TableList;
	};

	/** ブロードキャスタ名記述子クラス */
	class BroadcasterNameDescriptor
		: public DescriptorTemplate<BroadcasterNameDescriptor, 0xD8>
	{
	public:
	// DescriptorBase
		void Reset() override;

	// BroadcasterNameDescriptor
		bool GetBroadcasterName(ReturnArg<ARIBString> Name) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		ARIBString m_BroadcasterName;
	};

	/** コンポーネントグループ記述子クラス */
	class ComponentGroupDescriptor
		: public DescriptorTemplate<ComponentGroupDescriptor, 0xD9>
	{
	public:
		struct CAUnitInfo {
			uint8_t CAUnitID;         /**< CA_unit_id */
			uint8_t NumOfComponent;   /**< num_of_component */
			uint8_t ComponentTag[16]; /**< component_tag */
		};

		struct GroupInfo {
			uint8_t ComponentGroupID;  /**< component_group_id */
			uint8_t NumOfCAUnit;       /**< num_of_CA_unit */
			CAUnitInfo CAUnitList[16];
			uint8_t TotalBitRate;      /**< total_bit_rate */
			ARIBString Text;           /**< text_char */
		};

		ComponentGroupDescriptor();

	// DescriptorBase
		void Reset() override;

	// ComponentGroupDescriptor
		uint8_t GetComponentGroupType() const noexcept { return m_ComponentGroupType; }
		bool GetTotalBitRateFlag() const noexcept { return m_TotalBitRateFlag; }
		uint8_t GetGroupCount() const;
		const GroupInfo * GetGroupInfo(int Index) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_ComponentGroupType; /**< component_group_id */
		bool m_TotalBitRateFlag;      /**< total_bit_rate_flag */
		std::vector<GroupInfo> m_GroupList;
	};

	/** LDT リンク記述子 */
	class LDTLinkageDescriptor
		: public DescriptorTemplate<LDTLinkageDescriptor, 0xDC>
	{
	public:
		struct DescriptionInfo {
			uint16_t DescriptionID;  /**< description_id */
			uint8_t DescriptionType; /**< description_type */
		};

		LDTLinkageDescriptor();

	// DescriptorBase
		void Reset() override;

	// LDTLinkageDescriptor
		uint16_t GetOriginalServiceID() const noexcept { return m_OriginalServiceID; }
		uint16_t GetTransportStreamID() const noexcept { return m_TransportStreamID; }
		uint16_t GetOriginalNetworkID() const noexcept { return m_OriginalNetworkID; }
		int GetDescriptionInfoCount() const;
		bool GetDescriptionInfo(int Index, ReturnArg<DescriptionInfo> Info) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_OriginalServiceID; /**< original_service_id */
		uint16_t m_TransportStreamID; /**< transport_stream_id */
		uint16_t m_OriginalNetworkID; /**< original_network_id */
		std::vector<DescriptionInfo> m_DescriptionList;
	};

	/** アクセス制御記述子クラス */
	class AccessControlDescriptor
		: public DescriptorTemplate<AccessControlDescriptor, 0xF6>
	{
	public:
		AccessControlDescriptor();

	// DescriptorBase
		void Reset() override;

	// AccessControlDescriptor
		uint16_t GetCASystemID() const noexcept { return m_CASystemID; }
		uint8_t GetTransmissionType() const noexcept { return m_TransmissionType; }
		uint16_t GetPID() const noexcept { return m_PID; }
		const DataBuffer * GetPrivateData() const noexcept { return &m_PrivateData; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_CASystemID;      /**< CA_system_ID */
		uint8_t m_TransmissionType; /**< Transmission_type */
		uint16_t m_PID;             /**< PID */
		DataBuffer m_PrivateData;   /**< private_data_byte */
	};

	/** 地上分配システム記述子クラス */
	class TerrestrialDeliverySystemDescriptor
		: public DescriptorTemplate<TerrestrialDeliverySystemDescriptor, 0xFA>
	{
	public:
		TerrestrialDeliverySystemDescriptor();

	// DescriptorBase
		void Reset() override;

	// TerrestrialDeliverySystemDescriptor
		uint16_t GetAreaCode() const noexcept { return m_AreaCode; }
		uint8_t GetGuardInterval() const noexcept { return m_GuardInterval; }
		uint8_t GetTransmissionMode() const noexcept { return m_TransmissionMode; }
		int GetFrequencyCount() const;
		uint16_t GetFrequency(int Index) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_AreaCode;               /**< area_code */
		uint8_t m_GuardInterval;           /**< guard_interval */
		uint8_t m_TransmissionMode;        /**< transmission_mode */
		std::vector<uint16_t> m_Frequency; /**< frequency */
	};

	/** 部分受信記述子クラス */
	class PartialReceptionDescriptor
		: public DescriptorTemplate<PartialReceptionDescriptor, 0xFB>
	{
	public:
		PartialReceptionDescriptor();

	// DescriptorBase
		void Reset() override;

	// PartialReceptionDescriptor
		int GetServiceCount() const;
		uint16_t GetServiceID(int Index) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_ServiceCount;
		uint16_t m_ServiceList[3];
	};

	/** 緊急情報記述子クラス */
	class EmergencyInformationDescriptor
		: public DescriptorTemplate<EmergencyInformationDescriptor, 0xFC>
	{
	public:
		struct ServiceInfo {
			uint16_t ServiceID;                 /**< service_id */
			bool StartEndFlag;                  /**< start_end_flag */
			bool SignalLevel;                   /**< signal_level */
			std::vector<uint16_t> AreaCodeList; /**< area_code */
		};

	// DescriptorBase
		void Reset() override;

	// EmergencyInformationDescriptor
		int GetServiceCount() const;
		bool GetServiceInfo(int Index, ServiceInfo *pInfo) const;

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		std::vector<ServiceInfo> m_ServiceList;
	};

	/** データ符号化方式記述子クラス */
	class DataComponentDescriptor
		: public DescriptorTemplate<DataComponentDescriptor, 0xFD>
	{
	public:
		DataComponentDescriptor();

	// DescriptorBase
		void Reset() override;

	// DataComponentDescriptor
		uint16_t GetDataComponentID() const noexcept { return m_DataComponentID; }
		const DataBuffer * GetAdditionalDataComponentInfo() const noexcept { return &m_AdditionalDataComponentInfo; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint16_t m_DataComponentID;               /**< data_component_id */
		DataBuffer m_AdditionalDataComponentInfo; /**< additional_data_component_info */
	};

	/** システム管理記述子クラス */
	class SystemManagementDescriptor
		: public DescriptorTemplate<SystemManagementDescriptor, 0xFE>
	{
	public:
		SystemManagementDescriptor();

	// DescriptorBase
		void Reset() override;

	// SystemManagementDescriptor
		uint8_t GetBroadcastingFlag() const noexcept { return m_BroadcastingFlag; }
		uint8_t GetBroadcastingID() const noexcept { return m_BroadcastingID; }
		uint8_t GetAdditionalBroadcastingID() const noexcept { return m_AdditionalBroadcastingID; }

	protected:
		bool StoreContents(const uint8_t *pPayload) override;

		uint8_t m_BroadcastingFlag;         /**< broadcasting_flag */
		uint8_t m_BroadcastingID;           /**< broadcasting_identifier */
		uint8_t m_AdditionalBroadcastingID; /**< additional_broadcasting_identification */
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DESCRIPTORS_H
