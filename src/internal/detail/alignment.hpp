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

#ifndef MPIRPC__INTERNAL__DETAIL__ALIGNMENT_HPP
#define MPIRPC__INTERNAL__DETAIL__ALIGNMENT_HPP

#include <tuple>
#include "../../types.hpp"

namespace mpirpc
{

namespace internal
{

constexpr static std::size_t calculate_alignment_padding(std::size_t addr, std::size_t alignment);

namespace detail
{

template<typename Buffer,bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Pos, typename...Ts>
struct alignment_padding_helper
{
    using type = std::tuple_element_t<Pos,std::tuple<Ts...>>;
    static constexpr bool predicate = (mpirpc::is_buildtype<type,Buffer> && !SkipBuildTypes) || (!mpirpc::is_buildtype<type,Buffer> && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::end_address_offset;
    static constexpr std::size_t delta = (predicate) ? mpirpc::internal::calculate_alignment_padding(prev_end_address_offset,alignof(type)) : 0;
    static constexpr std::size_t start_address_offset = (predicate) ? prev_end_address_offset + delta : 0;
    static constexpr std::size_t end_address_offset = (predicate) ? start_address_offset + sizeof(type) : 0;
    static constexpr std::size_t total_padding = alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::total_padding + delta;
    static constexpr std::size_t total_size = alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,Ts...>::total_size + delta + sizeof(type);
};

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename T, typename...Ts>
struct alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,0,T,Ts...>
{
    using type = T;
    static constexpr bool predicate = (is_buildtype<type,Buffer> && !SkipBuildTypes) || (!is_buildtype<type,Buffer> && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = 0;
    static constexpr std::size_t delta = 0;
    static constexpr std::size_t start_address_offset = 0;
    static constexpr std::size_t end_address_offset = (predicate) ? sizeof(type) : 0;
    static constexpr std::size_t total_padding = 0;
    static constexpr std::size_t total_size = sizeof(type);
};

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes>
struct alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,0>
{
    using type = std::nullptr_t;
    static constexpr bool predicate = false;
    static constexpr std::size_t prev_end_address_offset = 0;
    static constexpr std::size_t delta = 0;
    static constexpr std::size_t start_address_offset = 0;
    static constexpr std::size_t end_address_offset = 0;
    static constexpr std::size_t total_padding = 0;
    static constexpr std::size_t total_size = 0;
};

template<typename T1, typename T2,typename = void>
struct choose_custom_alignment;

template<std::size_t...I1s,std::size_t...I2s>
struct choose_custom_alignment<std::integer_sequence<std::size_t,I1s...>,std::integer_sequence<std::size_t,I2s...>, std::enable_if_t<!!sizeof...(I2s)>>
{
    using type = std::integer_sequence<std::size_t,I2s...>;
};

template<std::size_t...I1s, std::size_t...I2s>
struct choose_custom_alignment<std::integer_sequence<std::size_t,I1s...>,std::integer_sequence<std::size_t,I2s...>, std::enable_if_t<!sizeof...(I2s)>>
{
    using type = std::integer_sequence<std::size_t,I1s...>;
};

}

}

}

#endif /* MPIRPC__INTERNAL__DETAIL__ALIGNMENT_HPP */
