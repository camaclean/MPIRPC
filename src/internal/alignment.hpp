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

#ifndef MPIRPC__INTERNAL__ALIGNMENT_HPP
#define MPIRPC__INTERNAL__ALIGNMENT_HPP

#include "detail/alignment.hpp"

namespace mpirpc
{

namespace internal
{

constexpr static std::size_t calculate_alignment_padding(std::size_t addr, std::size_t alignment)
{
    return (addr % alignment) ? alignment - addr % alignment : 0;
}

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts>
constexpr std::size_t alignment_padding = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, (sizeof...(Ts) > 0) ? (sizeof...(Ts)-1) : 0,Ts...>::total_padding;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts>
constexpr std::size_t aligned_buffer_size = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, (sizeof...(Ts) > 0) ? (sizeof...(Ts)-1) : 0,Ts...>::total_size;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t index, typename...Ts>
constexpr std::size_t aligned_buffer_address_offset = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, index, Ts...>::start_address_offset;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t index, typename...Ts>
constexpr std::size_t aligned_buffer_delta = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, index, Ts...>::delta;

template<typename T1, typename T2>
using custom_alignments = typename detail::choose_custom_alignment<T1,T2>::type;

}

}

#endif /* MPIRPC__INTERNAL__ALIGNMENT_HPP */
