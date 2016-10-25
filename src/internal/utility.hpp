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

template<typename T1, typename T2>
struct tuple_cat_type_helper;

template<typename...T1s, typename...T2s>
struct tuple_cat_type_helper<std::tuple<T1s...>,std::tuple<T2s...>>
{
    using type = std::tuple<T1s...,T2s...>;
};

template<typename T1, typename T2>
using tuple_cat_type = typename tuple_cat_type_helper<T1,T2>::type;

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

template<typename Is1, typename Is2>
struct integer_sequence_cat;

template<typename Int, Int... I1s, Int... I2s>
struct integer_sequence_cat<std::integer_sequence<Int, I1s...>, std::integer_sequence<Int, I2s...>>
{
    using type = std::integer_sequence<Int,I1s...,I2s...>;
};

template <typename Is1, typename Is2>
using integer_sequence_cat_type = typename integer_sequence_cat<Is1,Is2>::type;

} //internal

} //mpirpc

#endif /* MPIRPC__INTERNAL__UTILITY_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
