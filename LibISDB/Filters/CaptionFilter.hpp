/*
  LibISDB
  Copyright(c) 2017-2018 DBCTRADO

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
 @file   CaptionFilter.hpp
 @brief  字幕フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_CAPTION_FILTER_H
#define LIBISDB_CAPTION_FILTER_H


#include "FilterBase.hpp"
#include "../TS/CaptionParser.hpp"
#include "../TS/PIDMap.hpp"
#include "../TS/PSITable.hpp"
#include <vector>


namespace LibISDB
{

	/** 字幕フィルタクラス */
	class CaptionFilter
		: public SingleIOFilter
		, protected CaptionParser::CaptionHandler
	{
	public:
		class Handler
		{
		public:
			virtual ~Handler() = default;

			virtual void OnLanguageUpdate(CaptionFilter *pFilter, CaptionParser *pParser) {}
			virtual void OnCaption(
				CaptionFilter *pFilter, CaptionParser *pParser,
				uint8_t Language, const CharType *pText,
				const ARIBStringDecoder::FormatList *pFormatList) {}
		};

		typedef CaptionParser::DRCSMap DRCSMap;

		CaptionFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("CaptionFilter"); }

	// FilterBase
		void Reset() override;
		void SetActiveServiceID(uint16_t ServiceID) override;

	// SingleIOFilter
		bool ProcessData(DataStream *pData) override;

	// CaptionFilter
		bool SetTargetStream(uint16_t ServiceID, uint8_t ComponentTag = 0xFF);
		uint16_t GetTargetServiceID() const noexcept { return m_TargetServiceID; }
		uint8_t GetTargetComponentTag() const noexcept { return m_TargetComponentTag; }
		void SetFollowActiveService(bool Follow);
		bool GetFollowActiveService() const noexcept { return m_FollowActiveService; }
		void SetCaptionHandler(Handler *pHandler);
		Handler * GetCaptionHandler() const { return m_pCaptionHandler; }
		void SetDRCSMap(DRCSMap *pDRCSMap);
		DRCSMap * GetDRCSMap() const { return m_pDRCSMap; }
		int GetLanguageCount() const;
		uint32_t GetLanguageCode(uint8_t LanguageTag) const;

	protected:
	// CaptionParser::CaptionHandler
		void OnLanguageUpdate(CaptionParser *pParser);
		void OnCaption(
			CaptionParser *pParser, uint8_t Language, const CharType *pText,
			const ARIBStringDecoder::FormatList *pFormatList);

		int GetServiceIndexByID(uint16_t ServiceID) const;
		CaptionParser * GetCurrentCaptionParser() const;
		void OnPATSection(const PSITableBase *pTable, const PSISection *pSection);
		void OnPMTSection(const PSITableBase *pTable, const PSISection *pSection);

		struct CaptionESInfo {
			uint16_t PID;
			uint8_t ComponentTag;
		};

		struct ServiceInfo {
			uint16_t ServiceID;
			uint16_t PMTPID;
			std::vector<CaptionESInfo> CaptionESList;
		};

		PIDMapManager m_PIDMapManager;

		std::vector<ServiceInfo> m_ServiceList;

		bool m_FollowActiveService;
		uint16_t m_TargetServiceID;
		uint8_t m_TargetComponentTag;
		uint16_t m_TargetESPID;
		Handler *m_pCaptionHandler;
		DRCSMap *m_pDRCSMap;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_CAPTION_FILTER_H
