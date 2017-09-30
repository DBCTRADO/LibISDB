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
 @file   MPEG2ParserFilter.hpp
 @brief  MPEG-2 解析フィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_MPEG2_PARSER_FILTER_H
#define LIBISDB_MPEG2_PARSER_FILTER_H


#include "VideoParser.hpp"
#include "../../../../MediaParsers/MPEG2VideoParser.hpp"


// InPlaceフィルタにする
#define MPEG2PARSERFILTER_INPLACE


namespace LibISDB::DirectShow
{

	/** MPEG-2 解析フィルタクラス */
	class __declspec(uuid("3F8400DA-65F1-4694-BB05-303CDE739680")) MPEG2ParserFilter
#ifndef MPEG2PARSERFILTER_INPLACE
		: public ::CTransformFilter
#else
		: public ::CTransInPlaceFilter
#endif
		, public VideoParser
		, protected MPEG2VideoParser::SequenceHandler
	{
	public:
		static IBaseFilter * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

		DECLARE_IUNKNOWN

#ifndef MPEG2PARSERFILTER_INPLACE
	// CTransformFilter
		HRESULT CheckInputType(const CMediaType *mtIn) override;
		HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut override);
		HRESULT DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop) override;
		HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) override;
#else
	// CTransInPlaceFilter
		HRESULT CheckInputType(const CMediaType *mtIn) override;
#endif
		HRESULT StartStreaming() override;
		HRESULT StopStreaming() override;
		HRESULT BeginFlush() override;

	protected:
		MPEG2ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);
		~MPEG2ParserFilter();

#ifndef MPEG2PARSERFILTER_INPLACE
	// CTransformFilter
		HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut) override;
#else
	// CTransInPlaceFilter
		HRESULT Transform(IMediaSample *pSample) override;
		HRESULT Receive(IMediaSample *pSample) override;
#endif

	// MPEG2VideoParser::SequenceHandler
		virtual void OnMPEG2Sequence(const MPEG2VideoParser *pParser, const MPEG2Sequence *pSequence) override;

		MPEG2VideoParser m_MPEG2Parser;
		IMediaSample *m_pOutSample;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_MPEG2_PARSER_FILTER_H
