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
 @file   StreamBufferFilter.hpp
 @brief  ストリームバッファフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_BUFFER_FILTER_H
#define LIBISDB_STREAM_BUFFER_FILTER_H


#include "FilterBase.hpp"
#include "../Base/StreamBufferDataStreamer.hpp"


namespace LibISDB
{

	/** ストリームバッファフィルタクラス */
	class StreamBufferFilter
		: public SingleIOFilter
	{
	public:
		StreamBufferFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("StreamBufferFilter"); }

	// FilterBase
		void Reset() override;

	// SingleIOFilter
		bool ProcessData(DataStream *pData) override;

	// StreamBufferFilter
		bool CreateMemoryBuffer(
			size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount);
		void DeleteBuffer();
		bool IsBufferCreated() const;
		void ClearBuffer();
		bool SetBuffer(const std::shared_ptr<StreamBuffer> &Buffer);
		std::shared_ptr<StreamBuffer> GetBuffer() const;
		std::shared_ptr<StreamBuffer> DetachBuffer();

		bool SetPendingBufferSize(size_t BlockSize, size_t MaxBlockCount);

		bool SetBufferingEnabled(bool Enabled);
		bool GetBufferingEnabled() const noexcept { return m_BufferingEnabled; }
		void SetClearOnReset(bool Clear);
		bool GetClearOnReset() const noexcept { return m_ClearOnReset; }

	protected:
		StreamBufferDataStreamer m_DataStreamer;
		bool m_BufferingEnabled;
		bool m_ClearOnReset;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_BUFFER_FILTER_H
