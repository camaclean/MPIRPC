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

#ifndef MPIRPC__INTERNAL__UTILITY_HPP
#define MPIRPC__INTERNAL__UTILITY_HPP

#include "detail/utility.hpp"

#include <utility>

namespace mpirpc
{

namespace internal
{

/**
 * Passer can be used along with uniform initialization to unpack parameter packs
 * and execute the parameters in the order in which they appear. This is necessary
 * for correctness when side effects are important.
 */
struct passer {
    passer(...) {}
};

template <typename F, typename Tuple>
decltype(auto) apply(F&& f, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
}

template <typename F, class Class, typename Tuple>
decltype(auto) apply(F&& f, Class *c, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return detail::apply_impl(std::forward<F>(f), c, std::forward<Tuple>(t), Indices{});
}

template<std::size_t Pos, typename Int, Int Max, Int... Is>
constexpr Int get_clamped(std::integer_sequence<Int,Is...>)
{
    return ::mpirpc::internal::detail::get_integer_sequence_clamped_impl<Pos,Int,Max,Is...>::value;
}

template<std::size_t Pos, std::size_t Max, std::size_t... Is>
constexpr std::size_t get_clamped(std::integer_sequence<std::size_t, Is...>)
{
    return get_clamped<Pos,std::size_t,Max>(std::integer_sequence<std::size_t,Is...>());
}

template<std::size_t Pos, typename Int, Int...Is>
constexpr Int get(std::integer_sequence<Int,Is...>)
{
    return ::mpirpc::internal::detail::get_integer_sequence_impl<Pos,Int,Is...>::value;
}

template<std::size_t Pos, typename IndexSequence>
struct index_sequence_element;

template<std::size_t Pos, std::size_t... Is>
struct index_sequence_element<Pos,std::index_sequence<Is...>>
{
    static constexpr std::size_t value = ::mpirpc::internal::detail::get_integer_sequence_impl<Pos,std::size_t,Is...>::value;
};

template<typename Is1, typename Is2>
struct integer_sequence_cat;

template<typename Int, Int... I1s, Int... I2s>
struct integer_sequence_cat<std::integer_sequence<Int, I1s...>, std::integer_sequence<Int, I2s...>>
{
    using type = std::integer_sequence<Int,I1s...,I2s...>;
};

template <typename Is1, typename Is2>
using integer_sequence_cat_type = typename integer_sequence_cat<Is1,Is2>::type;

template<typename Int, Int I, typename Is>
struct integer_sequence_prepend;

template<typename Int, Int I, Int... Is>
struct integer_sequence_prepend<Int,I,std::integer_sequence<Int,Is...>>
{
    using type = std::integer_sequence<Int,I,Is...>;
};

template<typename Int, Int I, typename Is>
using integer_sequence_prepend_type = typename integer_sequence_prepend<Int,I,Is>::type;

template<std::size_t I, typename Is>
using index_sequence_prepend_type = typename integer_sequence_prepend<std::size_t,I,Is>::type;

template<typename Int, typename Is, Int I>
struct integer_sequence_append;

template<typename Int, Int I, Int... Is>
struct integer_sequence_append<Int,std::integer_sequence<Int,Is...>,I>
{
    using type = std::integer_sequence<Int,Is...,I>;
};

template<typename Int, typename Is, Int I>
using integer_sequence_append_type = typename integer_sequence_append<Int,Is,I>::type;

template<typename Is, std::size_t I>
using index_sequence_append_type = typename integer_sequence_append<std::size_t,Is,I>::type;

template<bool Condition, typename Int, Int I, typename Is>
struct conditional_integer_sequence_prepend
{
    using type = std::conditional_t<Condition,integer_sequence_prepend_type<Int,I,Is>,Is>;
};

template<bool Condition, typename Int, Int I, typename Is>
using conditional_integer_sequence_prepend_type = typename conditional_integer_sequence_prepend<Condition,Int,I,Is>::type;

template<bool Condition, std::size_t I, typename Is>
using conditional_index_sequence_prepend_type = typename conditional_integer_sequence_prepend<Condition,std::size_t,I,Is>::type;

template<bool Condition, typename Int, typename Is, Int I>
struct conditional_integer_sequence_append
{
    using type = std::conditional_t<Condition,integer_sequence_append_type<Int,Is,I>,Is>;
};

template<bool Condition, typename Int, typename Is, Int I>
using conditional_integer_sequence_append_type = typename conditional_integer_sequence_append<Condition,Int,Is,I>::type;

template<bool Condition, typename Is, std::size_t I>
using conditional_index_sequence_append_type = typename conditional_integer_sequence_append<Condition,std::size_t,Is,I>::type;

template<typename T1, typename T2>
struct tuple_type_cat;

template<typename...T1s, typename...T2s>
struct tuple_type_cat<std::tuple<T1s...>,std::tuple<T2s...>>
{
    using type = std::tuple<T1s...,T2s...>;
};

template<typename T1, typename T2>
using tuple_type_cat_type = typename tuple_type_cat<T1,T2>::type;

template<typename T, typename Tuple>
struct tuple_type_prepend;

template<typename T, typename... Ts>
struct tuple_type_prepend<T,std::tuple<Ts...>>
{
    using type = std::tuple<T,Ts...>;
};

template<typename T, typename Tuple>
using tuple_type_prepend_type = typename tuple_type_prepend<T,Tuple>::type;

template<typename Tuple, typename T>
struct tuple_type_append;

template<typename... Ts, typename T>
struct tuple_type_append<std::tuple<Ts...>,T>
{
    using type = std::tuple<Ts...,T>;
};

template<typename Tuple, typename T>
using tuple_type_append_type = typename tuple_type_append<Tuple,T>::type;

template<bool Condition, typename T, typename Tuple>
struct conditional_tuple_type_prepend
{
    using type = std::conditional_t<Condition,tuple_type_prepend_type<T,Tuple>,Tuple>;
};

template<bool Condition, typename T, typename Tuple>
using conditional_tuple_type_prepend_type = typename conditional_tuple_type_prepend<Condition,T,Tuple>::type;

template<bool Condition, typename Tuple, typename T>
struct conditional_tuple_type_append
{
    using type = std::conditional_t<Condition,tuple_type_append_type<Tuple,T>,Tuple>;
};

template<bool Condition, typename Tuple, typename T>
using conditional_tuple_type_append_type = typename conditional_tuple_type_append<Condition,Tuple,T>::type;

} //internal

} //mpirpc

#endif /* MPIRPC__INTERNAL__UTILITY_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
