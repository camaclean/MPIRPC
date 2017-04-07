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
