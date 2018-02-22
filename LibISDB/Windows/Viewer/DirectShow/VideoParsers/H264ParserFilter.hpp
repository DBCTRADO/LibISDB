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
 @file   H264ParserFilter.hpp
 @brief  H.264 解析フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_H264_PARSER_FILTER_H
#define LIBISDB_H264_PARSER_FILTER_H


#include "VideoParser.hpp"
#include "../../../../MediaParsers/H264Parser.hpp"
#include <deque>


namespace LibISDB::DirectShow
{

	/** H.264 解析フィルタクラス */
	class __declspec(uuid("46941C5F-AD0A-47fc-A35A-155ECFCEB4BA")) H264ParserFilter
		: public ::CTransformFilter
		, public VideoParser
		, protected H264Parser::AccessUnitHandler
	{
	public:
		static IBaseFilter * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

		DECLARE_IUNKNOWN

	// CTransInplaceFilter
		HRESULT CheckInputType(const CMediaType *mtIn) override;
		HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut) override;
		HRESULT DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop) override;
		HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
		HRESULT StartStreaming() override;
		HRESULT StopStreaming() override;
		HRESULT BeginFlush() override;

	// VideoParser
		bool SetAdjustSampleOptions(AdjustSampleFlag Flags) override;

	protected:
		H264ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);
		~H264ParserFilter();

	// CTransformFilter
		HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut) override;
		HRESULT Receive(IMediaSample *pSample) override;

	// H264Parser::AccessUnitHandler
		virtual void OnAccessUnit(const H264Parser *pParser, const H264AccessUnit *pAccessUnit) override;

	// H264ParserFilter
		class SampleData : public DataBuffer
		{
		public:
			REFERENCE_TIME m_StartTime;
			REFERENCE_TIME m_EndTime;
			bool m_ChangeSize;
			int m_Width;
			int m_Height;

			SampleData(const DataBuffer &Data);
			void ResetProperties();
			void SetTime(REFERENCE_TIME StartTime, REFERENCE_TIME EndTime);
			bool HasTimestamp() const { return m_StartTime >= 0; }
			void ChangeSize(int Width, int Height);
		};

		typedef std::deque<SampleData*> SampleDataQueue;

		class SampleDataPool
		{
		public:
			SampleDataPool();
			~SampleDataPool();
			void Clear();
			SampleData * Get(const DataBuffer &Data);
			void Restore(SampleData *pData);

		private:
			size_t m_MaxData;
			size_t m_DataCount;
			SampleDataQueue m_Queue;
			CCritSec m_Lock;
		};

		void Reset();
		void ClearSampleDataQueue(SampleDataQueue *pQueue);
		HRESULT AttachMediaType(IMediaSample *pSample, int Width, int Height);

		CMediaType m_MediaType;
		H264Parser m_H264Parser;
		SampleDataPool m_SampleDataPool;
		SampleDataQueue m_OutSampleQueue;
		bool m_AdjustTime;
		bool m_AdjustFrameRate;
		bool m_Adjust1Seg;
		REFERENCE_TIME m_PrevTime;
		DWORD m_SampleCount;
		SampleDataQueue m_SampleQueue;
		bool m_ChangeSize;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_H264_PARSER_FILTER_H
