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
 @file   PSISection.hpp
 @brief  PSI セクション
 @author DBCTRADO
*/


#ifndef LIBISDB_PSI_SECTION_H
#define LIBISDB_PSI_SECTION_H


#include "TSPacket.hpp"


namespace LibISDB
{

	/** PSI セクションクラス */
	class PSISection
		: public DataBuffer
	{
	public:
		PSISection();
		PSISection(size_t BufferSize);

		bool operator == (const PSISection &rhs) const noexcept;
		bool operator != (const PSISection &rhs) const noexcept { return !(*this == rhs); }

		bool ParseHeader(bool IsExtended = true, bool IgnoreSectionNumber = false);
		void Reset();

		const uint8_t * GetPayloadData() const;
		uint16_t GetPayloadSize() const;

		uint8_t GetTableID() const noexcept { return m_Header.TableID; }
		bool IsExtendedSection() const noexcept { return m_Header.SectionSyntaxIndicator; }
		bool GetPrivateIndicator() const noexcept { return m_Header.PrivateIndicator; }
		uint16_t GetSectionLength() const noexcept { return m_Header.SectionLength; }
		uint16_t GetTableIDExtension() const noexcept { return m_Header.TableIDExtension; }
		uint8_t GetVersionNumber() const noexcept { return m_Header.VersionNumber; }
		bool GetCurrentNextIndicator() const noexcept { return m_Header.CurrentNextIndicator; }
		uint8_t GetSectionNumber() const noexcept { return m_Header.SectionNumber; }
		uint8_t GetLastSectionNumber() const noexcept { return m_Header.LastSectionNumber; }

	protected:
		struct PSIHeader {
			uint8_t TableID;
			bool SectionSyntaxIndicator;
			bool PrivateIndicator;
			uint16_t SectionLength;
			uint16_t TableIDExtension;
			uint8_t VersionNumber;
			bool CurrentNextIndicator;
			uint8_t SectionNumber;
			uint8_t LastSectionNumber;

			bool operator == (const PSIHeader &rhs) const noexcept;
			bool operator != (const PSIHeader &rhs) const noexcept { return !(*this == rhs); }
		};

		PSIHeader m_Header;
	};

	/** PSI セクション解析クラス */
	class PSISectionParser
	{
	public:
		class PSISectionHandler
		{
		public:
			virtual ~PSISectionHandler() = default;

			virtual bool OnPSISection(const PSISectionParser *pPSISectionParser, const PSISection *pSection) = 0;
		};

		PSISectionParser(PSISectionHandler *pSectionHandler, bool IsExtended = true, bool IgnoreSectionNumber = false);

		void StorePacket(const TSPacket *pPacket);
		void Reset();
		void SetPSISectionHandler(PSISectionHandler *pSectionHandler);

		unsigned long GetCRCErrorCount() const;

	private:
		bool StoreHeader(const uint8_t *pData, uint8_t *pRemain);
		bool StorePayload(const uint8_t *pData, uint8_t *pRemain);

		PSISectionHandler *m_pPSISectionHandler;
		PSISection m_PSISection;
		bool m_IsExtended;
		bool m_IgnoreSectionNumber;

		bool m_IsPayloadStoring;
		uint16_t m_StoreSize;
		unsigned long m_CRCErrorCount;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_SPI_SECTION_H
