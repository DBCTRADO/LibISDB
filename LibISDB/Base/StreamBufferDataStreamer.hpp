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
 @file   StreamBufferDataStreamer.hpp
 @brief  ストリームバッファデータストリーマ
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_BUFFER_DATA_STREAMER_H
#define LIBISDB_STREAM_BUFFER_DATA_STREAMER_H


#include "DataStreamer.hpp"


namespace LibISDB
{

	/** ストリームバッファデータストリーマクラス */
	class StreamBufferDataStreamer
		: public DataStreamer
	{
	public:
		~StreamBufferDataStreamer();

		bool SetOutputBuffer(const std::shared_ptr<StreamBuffer> &Buffer);
		std::shared_ptr<StreamBuffer> GetOutputBuffer() const;
		std::shared_ptr<StreamBuffer> DetachOutputBuffer();
		void FreeOutputBuffer();
		bool ClearOutputBuffer();
		bool HasOutputBuffer() const;

	protected:
	// DataStreamer
		size_t OutputData(const uint8_t *pData, size_t DataSize) override;
		bool IsOutputValid() const override;
		void ClearOutput() override;

		std::shared_ptr<StreamBuffer> m_OutputBuffer;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_BUFFER_DATA_STREAMER_H
