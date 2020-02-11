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
 @file   DataStream.hpp
 @brief  データストリーム
 @author DBCTRADO
*/


#ifndef LIBISDB_DATA_STREAM_H
#define LIBISDB_DATA_STREAM_H


#include "DataBuffer.hpp"
#include <vector>


namespace LibISDB
{

	/** データストリームクラス */
	class DataStream
	{
	public:
		virtual DataBuffer * GetData() const noexcept = 0;
		virtual bool Next() noexcept = 0;
		virtual void Rewind() noexcept = 0;
		virtual unsigned long GetTypeID() const noexcept { return GetData()->GetTypeID(); }

		template<typename T> bool Is() const noexcept
		{
			return GetTypeID() == T::TypeID;
		}

		template<typename T> T * Get() const
		{
			LIBISDB_ASSERT(Is<T>());
			return static_cast<T *>(GetData());
		}
	};

	template<typename T> class SingleDataStream
		: public DataStream
	{
	public:
		SingleDataStream(T *pData) noexcept
			: m_pData(pData)
		{
		}

	// DataStream
		DataBuffer * GetData() const noexcept override { return m_pData; }
		bool Next() noexcept override { return false; }
		void Rewind() noexcept override {}

	protected:
		T *m_pData;
	};

	template<typename T, typename TContainer = std::vector<T>> class DataStreamSequence
	{
	public:
		typedef T value_type;
		typedef typename TContainer::iterator iterator;
		typedef typename TContainer::const_iterator const_iterator;

		iterator begin() { return m_DataList.begin(); }
		iterator end() { return m_DataList.begin() + m_ValidCount; }
		const_iterator begin() const { return m_DataList.begin(); }
		const_iterator end() const { return m_DataList.begin() + m_ValidCount; }
		const_iterator cbegin() const { return m_DataList.begin(); }
		const_iterator cend() const { return m_DataList.begin() + m_ValidCount; }

		T & operator [] (size_t Index) { return m_DataList[Index]; }
		const T & operator [] (size_t Index) const { return m_DataList[Index]; }

		void Clear() noexcept
		{
			m_DataList.clear();
			m_ValidCount = 0;
		}

		void Allocate(size_t Count)
		{
			m_DataList.resize(Count);
			m_ValidCount = 0;
		}

		size_t GetDataCount() const noexcept
		{
			return m_ValidCount;
		}

		void SetDataCount(size_t Count)
		{
			if (Count > m_DataList.size())
				m_DataList.resize(Count);
			m_ValidCount = Count;
		}

		void AddData(const T &Data)
		{
			if (m_ValidCount < m_DataList.size())
				m_DataList[m_ValidCount] = Data;
			else
				m_DataList.push_back(Data);
			m_ValidCount++;
		}

	protected:
		TContainer m_DataList;
		size_t m_ValidCount = 0;
	};

	template<typename T> class BasicDataStream
		: public DataStream
	{
	public:
		typedef typename T::iterator Iterator;

		BasicDataStream(Iterator Begin, Iterator End)
			: m_Begin(Begin)
			, m_End(End)
			, m_Current(Begin)
		{
			LIBISDB_ASSERT(m_Begin != m_End);
		}

		BasicDataStream(T &Sequence)
			: BasicDataStream(Sequence.begin(), Sequence.end())
		{
		}

	// DataStream
		DataBuffer * GetData() const noexcept override
		{
			if constexpr (std::is_pointer<typename T::value_type>::value)
				return *m_Current;
			else
				return &*m_Current;
		}

		bool Next() noexcept override { return ++m_Current != m_End; }
		void Rewind() noexcept override { m_Current = m_Begin; }

	protected:
		Iterator m_Begin;
		Iterator m_End;
		Iterator m_Current;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_DATA_H
