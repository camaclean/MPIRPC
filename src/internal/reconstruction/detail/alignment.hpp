/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014-2017 Colin MacLean <cmaclean@illinois.edu>
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

#ifndef MPIRPC__INTERNAL__RECONSTRUCTION__DETAIL__ALIGNMENT_HPP
#define MPIRPC__INTERNAL__RECONSTRUCTION__DETAIL__ALIGNMENT_HPP

#include <tuple>
#include "../internal/alignment.hpp"

namespace mpirpc
{
    
namespace internal
{
    
namespace reconstruction
{

namespace detail
{
    
/**
 * \internal
 * Helper class for preparing alignment data tuples for aligned_type_holder.
 * 
 * Typedefs:
 *      alignments:
 *          The full tuple representation of the alignments for the type and
 *          its constructor arguments. This ensures fully populated alignment
 *          info for creating an aligned_type_holder from a construction_info
 *          object.
 *      arg_alignments:
 *          A tuple of alignment info for the constructor arguments, populated
 *          with default values if data is incomplete.
 *      type_alignment:
 *          The alignment of the type to be constructed. Uses the default 
 *          alignment if the data is incomplete.
 */
template<typename T, typename ArgumentsTuple, typename Alignments,typename=void>
struct construction_alignments;

/*************************************************************************************/
/*************************************************************************************/
/*                                  Implementation                                   */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*         mpirpc::internal::reconstruction::detail::construction_alignments         */
/*************************************************************************************/

template<typename T, typename... Arguments, std::size_t Alignment, typename... Alignments>
struct construction_alignments<T, std::tuple<Arguments...>, std::tuple<std::integral_constant<std::size_t,Alignment>,Alignments...>,
    std::enable_if_t<sizeof...(Alignments) == sizeof...(Arguments) && (sizeof...(Alignments) > 0) >>
{
    using alignments     = std::tuple<std::integral_constant<std::size_t,Alignment>,Alignments...>;
    using arg_alignments = std::tuple<Alignments...>;
    using type_alignment = std::integral_constant<std::size_t,Alignment>;
};

template<typename T, typename... Arguments, std::size_t Alignment>
struct construction_alignments<T, std::tuple<Arguments...>, std::tuple<std::integral_constant<std::size_t,Alignment>>>
{
    using alignments     = std::tuple<std::integral_constant<std::size_t,Alignment>, type_default_alignment<Arguments,alignof(Arguments)>...>;
    using arg_alignments = std::tuple<type_default_alignment<Arguments,alignof(Arguments)>...>;
    using type_alignment = std::integral_constant<std::size_t,Alignment>;
};

template<typename T, typename... Arguments>
struct construction_alignments<T, std::tuple<Arguments...>, std::tuple<>>
{
    using alignments     = std::tuple<std::integral_constant<std::size_t,alignof(T)>, type_default_alignment<Arguments,alignof(Arguments)>...>;
    using arg_alignments = std::tuple<type_default_alignment<Arguments,alignof(Arguments)>...>;
    using type_alignment = std::integral_constant<std::size_t,alignof(T)>;
};

template<typename T, typename... Arguments, std::size_t Alignment>
struct construction_alignments<T, std::tuple<Arguments...>, std::integral_constant<std::size_t,Alignment>>
{
    using alignments     = std::tuple<std::integral_constant<std::size_t,Alignment>, type_default_alignment<Arguments,alignof(Arguments)>...>;
    using arg_alignments = std::tuple<type_default_alignment<Arguments,alignof(Arguments)>...>;
    using type_alignment = std::integral_constant<std::size_t,Alignment>;
};
    
}

}

}

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__DETAIL__ALIGNMENT_HPP */
