/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014-2016 Colin MacLean <cmaclean@illinois.edu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MPIRPC__INTERNAL__DETAIL__UTILITY_HPP
#define MPIRPC__INTERNAL__DETAIL__UTILITY_HPP

#include <utility>
#include <tuple>
/*#include "../alignment.hpp"
#include "../direct_initializer.hpp"
#include "../remarshaller.hpp"*/
#include "../function_attributes.hpp"

namespace mpirpc
{

namespace internal
{

namespace detail
{

template<typename Alignment, typename Allocator, typename T, typename Buffer, std::enable_if_t<is_buildtype_v<T,Buffer>>* = nullptr>
void get_from_buffer(Allocator &a, T*& t, Buffer&& s)
{
    direct_initializer<T,Alignment>::placementnew_construct(a,t,s);
}

template<typename Alignment, typename Allocator, typename T, typename Buffer, std::enable_if_t<!is_buildtype_v<T,Buffer>>* = nullptr>
void get_from_buffer(Allocator &a, T*& t, Buffer&& s)
{
    get_pointer_from_buffer<Alignment::value>(s,t);
}

template<typename Buffer, typename Alignment, typename Allocator, typename T, std::enable_if_t<is_buildtype_v<T,Buffer>>* = nullptr>
void cleanup(Allocator &a, T* t)
{
    direct_initializer<T,Alignment>::placementnew_destruct(a,t);
}

template<typename Buffer, typename Alignment, typename Allocator, typename T, std::enable_if_t<!is_buildtype_v<T,Buffer>>* = nullptr>
void cleanup(Allocator &a, T* t)
{}

template <typename F, typename Tuple, std::size_t... I>
decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return std::forward<F>(f)(static_cast<std::tuple_element_t<I,typename ::mpirpc::internal::function_parts<std::remove_reference_t<F>>::args_tuple_type>>(std::get<I>(std::forward<Tuple>(t)))...);
}

template <typename F, class Class, typename Tuple, size_t... I>
decltype(auto) apply_impl(F&& f, Class *c, Tuple&& t, std::index_sequence<I...>)
{
    return ((*c).*(std::forward<F>(f)))(static_cast<std::tuple_element_t<I,typename ::mpirpc::internal::function_parts<std::remove_reference_t<F>>::args_tuple_type>>(std::get<I>(std::forward<Tuple>(t)))...);
}

/*template<std::size_t Pos, typename Int, Int Max, Int...Is>
struct get_integer_sequence_clamped_impl;

template<std::size_t Pos, typename Int, Int Max, Int I, Int... Is>
struct get_integer_sequence_clamped_impl<Pos,Int,Max,I,Is...>
{
    constexpr static Int value = get_integer_sequence_clamped_impl<Pos-1,Int,Max,Is...>::value;
};

template<typename Int, Int I, Int Max, Int...Is>
struct get_integer_sequence_clamped_impl<0,Int,Max,I,Is...>
{
    constexpr static Int value = I;
};

template<std::size_t Pos, typename Int, Int Max>
struct get_integer_sequence_clamped_impl<Pos,Int,Max>
{
    constexpr static Int value = Max;
};*/

}

}

}

#endif /* MPIRPC__INTERNAL__DETAIL__UTILITY_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
