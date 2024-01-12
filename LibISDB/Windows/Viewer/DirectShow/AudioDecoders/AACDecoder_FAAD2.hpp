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
 @file   AACDecoder_FAAD2.hpp
 @brief  AAC デコーダ (FAAD2)
 @author DBCTRADO
*/


#ifndef LIBISDB_AAC_DECODER_FAAD2_H
#define LIBISDB_AAC_DECODER_FAAD2_H


#include "AACDecoder.hpp"
#include "../../../../MediaParsers/ADTSParser.hpp"


typedef void *NeAACDecHandle;


namespace LibISDB::DirectShow
{

	/** AAC デコーダクラス (FAAD2) */
	class AACDecoder_FAAD2
		: public AACDecoder
	{
	public:
		AACDecoder_FAAD2() noexcept;
		~AACDecoder_FAAD2();

	// AudioDecoder
		bool IsOpened() const override;

		bool GetChannelMap(int Channels, int *pMap) const override;
		bool GetDownmixInfo(ReturnArg<DownmixInfo> Info) const override;

	// AACDecoder_FAAD2
		static void GetVersion(std::string *pVersion);

	private:
		bool OpenDecoder() override;
		void CloseDecoder() override;
		bool DecodeFrame(const ADTSFrame *pFrame, ReturnArg<DecodeFrameInfo> Info) override;

		NeAACDecHandle m_hDecoder;
		bool m_InitRequest;
		uint8_t m_LastChannelConfig;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_AAC_DECODER_FAAD2_H
