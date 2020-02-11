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
 @file   cstring_view.hpp
 @brief  c_str() のある string_view
 @author DBCTRADO
*/


#ifndef LIBISDB_C_STRING_VIEW_H
#define LIBISDB_C_STRING_VIEW_H


namespace LibISDB
{

	/** c_str() のある string_view */
	template<typename TChar, typename TTraits = std::char_traits<TChar>> class basic_cstring_view
		: public std::basic_string_view<TChar, TTraits>
	{
	public:
		constexpr basic_cstring_view() noexcept = default;
		constexpr basic_cstring_view(const TChar *s)
			: std::basic_string_view<TChar, TTraits>(s) {}
		template<typename TStringChar, typename TStringTraits, typename TStringAlloc>
		constexpr basic_cstring_view(const std::basic_string<TStringChar, TStringTraits, TStringAlloc> &s) noexcept
			: std::basic_string_view<TChar, TTraits>(s.c_str(), s.length()) {}

		constexpr const TChar * c_str() const noexcept { return this->data(); }
	};

	typedef basic_cstring_view<char> cstring_view;
	typedef basic_cstring_view<wchar_t> wcstring_view;
	typedef basic_cstring_view<char16_t> u16cstring_view;
	typedef basic_cstring_view<char32_t> u32cstring_view;

}	// namespace LibISDB


#endif	// ifndef LIBISDB_C_STRING_VIEW_H
