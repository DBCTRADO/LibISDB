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
 @file   MPEGVideoParser.hpp
 @brief  MPEG 系映像解析
 @author DBCTRADO
*/


#ifndef LIBISDB_MPEG_VIDEO_PARSER_H
#define LIBISDB_MPEG_VIDEO_PARSER_H


#include "../TS/PESPacket.hpp"


namespace LibISDB
{

	/** MPEG 系映像解析基底クラス */
	class MPEGVideoParserBase
		: public PESParser::PacketHandler
	{
	public:
		MPEGVideoParserBase();

		bool StorePacket(const PESPacket *pPacket);
		virtual bool StoreES(const uint8_t *pData, size_t Size) = 0;
		virtual void Reset();

	protected:
	// PESParser::PacketHandler
		void OnPESPacket(const PESParser *pParser, const PESPacket *pPacket) override;

		bool ParseSequence(const uint8_t *pData, size_t Size, uint32_t StartCode, uint32_t StartCodeMask, DataBuffer *pSequenceData);
		virtual void OnSequence(DataBuffer *pSequenceData) {}

		uint32_t m_SyncState;
	};

	size_t EBSPToRBSP(uint8_t *pData, size_t DataSize);

}	// namespace LibISDB


#endif	// ifndef LIBISDB_MPEG_VIDEO_PARSER_H
