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
 @file   VideoParser.hpp
 @brief  映像解析
 @author DBCTRADO
*/


#ifndef LIBISDB_VIDEO_PARSER_H
#define LIBISDB_VIDEO_PARSER_H


#include "../DirectShowBase.hpp"
#include "../../../../Base/EventListener.hpp"


namespace LibISDB::DirectShow
{

	/** 映像解析基底クラス */
	class VideoParser
	{
	public:
		class VideoInfo
		{
		public:
			struct Rational {
				int Num = 0;
				int Denom = 0;

				bool operator == (const Rational &rhs) const noexcept
				{
					return (Num == rhs.Num)
						&& (Denom == rhs.Denom);
				}

				bool operator != (const Rational &rhs) const noexcept
				{
					return !(*this == rhs);
				}
			};

			int OriginalWidth = 0;
			int OriginalHeight = 0;
			int DisplayWidth = 0;
			int DisplayHeight = 0;
			int DisplayPosX = 0;
			int DisplayPosY = 0;
			int AspectRatioX = 0;
			int AspectRatioY = 0;
			Rational FrameRate;

			VideoInfo() = default;
			VideoInfo(
				int OrigWidth, int OrigHeight,
				int DispWidth, int DispHeight,
				int AspectX, int AspectY) noexcept
				: OriginalWidth(OrigWidth)
				, OriginalHeight(OrigHeight)
				, DisplayWidth(DispWidth)
				, DisplayHeight(DispHeight)
				, DisplayPosX((OrigWidth - DispWidth) / 2)
				, DisplayPosY((OrigHeight - DispHeight) / 2)
				, AspectRatioX(AspectX)
				, AspectRatioY(AspectY)
			{
			}

			bool operator == (const VideoInfo &rhs) const noexcept
			{
				return (OriginalWidth == rhs.OriginalWidth)
					&& (OriginalHeight == rhs.OriginalHeight)
					&& (DisplayWidth == rhs.DisplayWidth)
					&& (DisplayHeight == rhs.DisplayHeight)
					&& (DisplayPosX == rhs.DisplayPosX)
					&& (DisplayPosY == rhs.DisplayPosY)
					&& (AspectRatioX == rhs.AspectRatioX)
					&& (AspectRatioY == rhs.AspectRatioY)
					&& (FrameRate == rhs.FrameRate);
			}

			bool operator != (const VideoInfo &rhs) const noexcept
			{
				return !(*this == rhs);
			}

			void Reset() noexcept
			{
				*this = VideoInfo();
			}
		};

		class EventListener : public LibISDB::EventListener
		{
		public:
			virtual void OnVideoInfoUpdated(const VideoInfo &Info) {}
		};

		class StreamCallback
		{
		public:
			virtual ~StreamCallback() = default;
			virtual void OnStream(DWORD Format, const void *pData, size_t Size) = 0;
		};

		enum class AdjustSampleFlag : unsigned int {
			None      = 0x0000U,
			Time      = 0x0001U,
			FrameRate = 0x0002U,
			OneSeg    = 0x0004U,
		};

		VideoParser() noexcept;
		virtual ~VideoParser();

		bool GetVideoInfo(VideoInfo *pInfo) const;
		void ResetVideoInfo();

		bool AddEventListener(EventListener *pEventListener);
		bool RemoveEventListener(EventListener *pEventListener);
		void SetStreamCallback(StreamCallback *pCallback);

		void SetAttachMediaType(bool Attach);
		virtual bool SetAdjustSampleOptions(AdjustSampleFlag Flags) { return false; }

	protected:
		void NotifyVideoInfo() const;

		static bool SARToDAR(
			int SARX, int SARY, int Width, int Height,
			int *pDARX, int *pDARY);

		VideoInfo m_VideoInfo;
		EventListenerList<EventListener> m_EventListenerList;
		StreamCallback *m_pStreamCallback;
		mutable CCritSec m_ParserLock;
		bool m_AttachMediaType;
	};

	LIBISDB_ENUM_FLAGS(VideoParser::AdjustSampleFlag)

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_VIDEO_PARSER_H
