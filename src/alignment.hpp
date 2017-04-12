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

#ifndef MPIRPC__ALIGNMENT_HPP
#define MPIRPC__ALIGNMENT_HPP

#include <tuple>

namespace mpirpc
{

template<typename T>
struct default_alignment_helper : std::integral_constant<std::size_t,alignof(T)> {};

template<typename T>
using default_alignment_type = typename default_alignment_helper<T>::type;

/*template<std::size_t Alignment>
using alignment_type = std::integral_constant<std::size_t,Alignment>;*/

template<std::size_t... Alignments>
struct alignment_data
{
    using type = std::tuple<std::integral_constant<std::size_t,Alignments>...>;
};

template<std::size_t Alignment>
struct alignment_data<Alignment> : std::integral_constant<std::size_t,Alignment> {};

template<std::size_t... Alignments>
using align = typename alignment_data<Alignments...>::type;

}

#endif /* MPIRPC__ALIGNMENT_HPP */
