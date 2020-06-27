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
 @file   AACDecoder.hpp
 @brief  AAC デコーダ
 @author DBCTRADO
*/


#ifndef LIBISDB_AAC_DECODER_H
#define LIBISDB_AAC_DECODER_H


#include "AudioDecoder.hpp"
#include "../../../../MediaParsers/ADTSParser.hpp"


namespace LibISDB::DirectShow
{

	/** AAC デコーダ基底クラス */
	class AACDecoder
		: public AudioDecoder
	{
	public:
		AACDecoder() noexcept;
		~AACDecoder();

	// AudioDecoder
		bool Open() override;
		void Close() override;
		bool Reset() override;
		bool Decode(const uint8_t *pData, size_t *pDataSize, ReturnArg<DecodeFrameInfo> Info) override;

		bool IsSPDIFSupported() const override { return true; }
		bool GetSPDIFFrameInfo(ReturnArg<SPDIFFrameInfo> Info) const override;
		int GetSPDIFBurstPayload(uint8_t *pBuffer, size_t BufferSize) const override;

	protected:
		virtual bool OpenDecoder() = 0;
		virtual void CloseDecoder() = 0;
		bool ResetDecoder();
		virtual bool DecodeFrame(const ADTSFrame *pFrame, ReturnArg<DecodeFrameInfo> Info) = 0;

		ADTSParser m_ADTSParser;
		const ADTSFrame *m_pADTSFrame;
		bool m_DecodeError;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_AAC_DECODER_H
