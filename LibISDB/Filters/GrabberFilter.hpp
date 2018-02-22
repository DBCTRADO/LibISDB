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
 @file   GrabberFilter.hpp
 @brief  データ取り込みフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_GRABBER_FILTER_H
#define LIBISDB_GRABBER_FILTER_H


#include "FilterBase.hpp"
#include <vector>


namespace LibISDB
{

	/** データ取り込みフィルタクラス */
	class GrabberFilter
		: public SingleIOFilter
	{
	public:
		class Grabber
		{
		public:
			virtual ~Grabber() = default;

			virtual bool ReceiveData(DataBuffer *pData) { return true; }
			virtual void OnReset() {}
		};

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("GrabberFilter"); }

	// FilterBase
		void Reset() override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	// GrabberFilter
		bool AddGrabber(Grabber *pGrabber);
		bool RemoveGrabber(Grabber *pGrabber);

	protected:
		std::vector<Grabber *> m_GrabberList;
		std::vector<DataBuffer *> m_OutputSequence;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_GRABBER_FILTER_H
