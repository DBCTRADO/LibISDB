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
 @file   ReturnArg.hpp
 @brief  値を返す引数に使用するテンプレートクラス
 @author DBCTRADO
*/


#ifndef LIBISDB_RETURN_ARG_H
#define LIBISDB_RETURN_ARG_H


namespace LibISDB
{

	template<typename T> class ReturnArg
	{
	public:
		ReturnArg(T *ptr) noexcept : m_ptr(ptr) {}

		ReturnArg & operator = (const T &rhs)
		{
			if (m_ptr != nullptr)
				*m_ptr = rhs;
			return *this;
		}

		ReturnArg & operator = (T &&rhs)
		{
			if (m_ptr != nullptr)
				*m_ptr = std::move(rhs);
			return *this;
		}

		explicit operator bool() const noexcept { return m_ptr != nullptr; }

		T & operator * () { return *m_ptr; }
		const T & operator * () const { return *m_ptr; }
		T * operator -> () noexcept { return m_ptr; }
		const T * operator -> () const noexcept { return m_ptr; }

	protected:
		ReturnArg() noexcept : m_ptr(nullptr) {}

		T *m_ptr;
	};

	template<typename T> class OptionalReturnArg
		: public ReturnArg<T>
	{
	public:
		OptionalReturnArg() = default;
		OptionalReturnArg(std::nullopt_t) noexcept {}
		using ReturnArg<T>::ReturnArg;
		using ReturnArg<T>::operator =;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_RETURN_ARG_H
