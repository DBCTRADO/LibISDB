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
 @file   FilterBase.hpp
 @brief  フィルタ基底クラス
 @author DBCTRADO
*/


#ifndef LIBISDB_FILTER_BASE_H
#define LIBISDB_FILTER_BASE_H


#include "../Base/ObjectBase.hpp"
#include "../Base/DataStream.hpp"
#include "../Utilities/Lock.hpp"


namespace LibISDB
{

	/** データ受け取り基底クラス */
	class FilterSink
	{
	public:
		virtual ~FilterSink() = default;

		virtual bool ReceiveData(DataStream *pData) { return false; }
	};

	/** フィルタ基底クラス */
	class FilterBase
		: public ObjectBase
	{
	public:
		virtual ~FilterBase() = default;

		virtual bool Initialize();
		virtual void Finalize();
		virtual void Reset();
		virtual void ResetGraph();
		virtual bool StartStreaming();
		virtual bool StopStreaming();

		virtual int GetInputCount() const { return 0; }
		virtual int GetOutputCount() const { return 0; }

		virtual FilterSink * GetInputSink(int Index = 0) { return nullptr; }

		virtual bool SetOutputFilter(FilterBase *pFilter, FilterSink *pSink, int Index = 0) { return false; }
		virtual void ResetOutputFilters() {}
		virtual FilterBase * GetOutputFilter(int Index = 0) const { return nullptr; }
		virtual FilterSink * GetOutputSink(int Index = 0) const { return nullptr; }

		virtual void SetActiveServiceID(uint16_t ServiceID) {}
		virtual void SetActiveVideoPID(uint16_t PID, bool ServiceChanged) {}
		virtual void SetActiveAudioPID(uint16_t PID, bool ServiceChanged) {}

	protected:
		bool OutputData(DataStream *pData, int OutputIndex = 0);
		bool OutputData(DataBuffer *pData, int OutputIndex = 0);
		template<typename T> bool OutputData(T &Sequence, int OutputIndex = 0)
		{
			BasicDataStream<T> Stream(Sequence);
			return OutputData(&Stream, OutputIndex);
		}

		void ResetDownstreamFilters();
		bool StartDownstreamFilters();
		bool StopDownstreamFilters();

		struct OutputFilterInfo {
			FilterBase *pFilter = nullptr;
			FilterSink *pSink = nullptr;
		};

		mutable MutexLock m_FilterLock;
	};

	/** 単出力フィルタ基底クラス */
	class SingleOutputFilter
		: public FilterBase
	{
	public:
		int GetInputCount() const override { return 0; }
		int GetOutputCount() const override { return 1; }

		FilterSink * GetInputSink(int Index = 0) override { return nullptr; }

		bool SetOutputFilter(FilterBase *pFilter, FilterSink *pSink, int Index = 0) override;
		void ResetOutputFilters() override;
		FilterBase * GetOutputFilter(int Index = 0) const override;
		FilterSink * GetOutputSink(int Index = 0) const override;

	protected:
		OutputFilterInfo m_OutputFilter;
	};

	/** 単入力フィルタ基底クラス */
	class SingleInputFilter
		: public FilterBase
		, public FilterSink
	{
	public:
	// FilterBase
		int GetInputCount() const override { return 1; }

		FilterSink * GetInputSink(int Index = 0) override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	protected:
		virtual bool ProcessData(DataStream *pData) { return true; }
	};

	/** 単入出力フィルタ基底クラス */
	class SingleIOFilter
		: public SingleOutputFilter
		, public FilterSink
	{
	public:
	// FilterBase
		int GetInputCount() const override { return 1; }

		FilterSink * GetInputSink(int Index = 0) override;

	// FilterSink
		bool ReceiveData(DataStream *pData) override;

	protected:
		virtual bool ProcessData(DataStream *pData) { return true; }
	};

	/** 複数出力フィルタ基底クラス */
	template<int OutputCount> class MultiOutputFilter
		: public FilterBase
		, public FilterSink
	{
	public:
		int GetInputCount() const override { return 1; }
		int GetOutputCount() const override { return OutputCount; }

		FilterSink * GetInputSink(int Index = 0) override
		{
			if (Index != 0)
				return nullptr;
			return this;
		}

		bool SetOutputFilter(FilterBase *pFilter, FilterSink *pSink, int Index = 0) override
		{
			if ((Index < 0) || (Index >= OutputCount))
				return false;
			m_OutputFilterList[Index].pFilter = pFilter;
			m_OutputFilterList[Index].pSink = pSink;
			return true;
		}

		void ResetOutputFilters() override
		{
			for (auto &e : m_OutputFilterList) {
				e.pFilter = nullptr;
				e.pSink = nullptr;
			}
		}

		FilterBase * GetOutputFilter(int Index = 0) const override
		{
			if ((Index < 0) || (Index >= OutputCount))
				return nullptr;
			return m_OutputFilterList[Index].pFilter;
		}

		FilterSink * GetOutputSink(int Index = 0) const override
		{
			if ((Index < 0) || (Index >= OutputCount))
				return nullptr;
			return m_OutputFilterList[Index].pSink;
		}

	protected:
		OutputFilterInfo m_OutputFilterList[OutputCount];
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_FILTER_BASE_H
