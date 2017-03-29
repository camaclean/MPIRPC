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

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename Types, typename Alignments>
constexpr std::size_t alignment_padding = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, (std::tuple_size<Types>::value > 0) ? (std::tuple_size<Types>::value-1) : 0, Types, Alignments>::total_padding;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename Types, typename Alignments>
constexpr std::size_t aligned_buffer_size = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, (std::tuple_size<Types>::value > 0) ? (std::tuple_size<Types>::value-1) : 0, Types, Alignments>::total_size;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Index, typename Types, typename Alignments>
constexpr std::size_t aligned_buffer_address_offset = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, Index, Types, Alignments>::start_address_offset;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Index, typename Types, typename Alignments>
constexpr std::size_t aligned_buffer_delta = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, Index, Types, Alignments>::delta;

template<typename T1, typename T2>
using custom_alignments = typename detail::choose_custom_alignment<T1,T2>::type;

template<typename Alignment>
struct alignment_reader;

template<std::size_t Alignment>
struct alignment_reader<std::integral_constant<std::size_t,Alignment>>
{
    static constexpr std::size_t value = Alignment;
    //using type = std::conditional_t<value,std::true_type,std::false_type>;
    using type = std::integral_constant<std::size_t,value>;
};

template<typename Alignment, typename... Alignments>
struct alignment_reader<std::tuple<Alignment,Alignments...>>
{
    static constexpr std::size_t value = alignment_reader<Alignment>::value;
    using type = std::integral_constant<std::size_t,value>;
};

template<typename Alignment>
using alignment_reader_type = typename alignment_reader<Alignment>::type;

template<typename Alignment>
struct internal_alignments_tuple_type_helper;

template<std::size_t Alignment>
struct internal_alignments_tuple_type_helper<std::integral_constant<std::size_t,Alignment>>
{
    using type = std::tuple<>;
};

template<typename Alignment, typename... Alignments>
struct internal_alignments_tuple_type_helper<std::tuple<Alignment,Alignments...>>
{
    using type = std::tuple<Alignments...>;
};

template<typename Alignments>
using internal_alignments_tuple_type = typename internal_alignments_tuple_type_helper<Alignments>::type;

}

}

#endif /* MPIRPC__INTERNAL__ALIGNMENT_HPP */
