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

#ifndef MPIRPC__INTERNAL__RECONSTRUCTION__TYPE_CONVERSION_HPP
#define MPIRPC__INTERNAL__RECONSTRUCTION__TYPE_CONVERSION_HPP

#include <tuple>
#include "alignment.hpp"
#include "type_properties.hpp"

namespace mpirpc
{

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple>
class construction_info;

namespace internal
{

namespace reconstruction
{
    
template<typename ConstructionInfo, typename AlignmentsTuple>
struct construction_info_to_aligned_type_holder;

template<typename ConstructionInfo, typename AlignmentsTuple>
using construction_info_to_aligned_type_holder_type = typename construction_info_to_aligned_type_holder<ConstructionInfo,AlignmentsTuple>::type;

template<typename T, typename Store, typename Alignment>
struct reconstruction_storage_type_helper;

template<typename T, typename Store, typename Alignment>
using reconstruction_storage_type = typename reconstruction_storage_type_helper<T,Store,Alignment>::type;

template<typename T, typename Store, typename Alignment>
using reconstruction_storage_constructor_type = typename reconstruction_storage_type_helper<T,Store,Alignment>::constructor_type;

template<typename T, typename Store, typename Alignment>
using reconstruction_storage_aligned_storage_type = typename reconstruction_storage_type_helper<T,Store,Alignment>::aligned_storage_type;

template<typename T, typename Store, typename Alignment>
using reconstruction_proxy_type = typename reconstruction_storage_type_helper<T,Store,Alignment>::proxy_type;

/*************************************************************************************/
/*************************************************************************************/
/*                                  Implementation                                   */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*     mpirpc::internal::reconstruction::construction_info_to_aligned_type_holder    */
/*************************************************************************************/

template<typename T, typename Alignment, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
class aligned_type_holder;

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentTypesTuple, typename StoredArgumentsTuple, typename Alignments>
struct construction_info_to_aligned_type_holder<construction_info<T,ConstructorArgumentTypesTuple,ArgumentTypesTuple,StoredArgumentsTuple>,Alignments>
{
    using type = aligned_type_holder<
                    T,
                    construction_type_alignment_type<T,ArgumentTypesTuple,Alignments>,
                    ConstructorArgumentTypesTuple,
                    ArgumentTypesTuple,
                    StoredArgumentsTuple,
                    construction_arg_alignments_type<T,ArgumentTypesTuple,Alignments>
                >;
};

template<typename T, typename Store, typename Alignment>
struct reconstruction_storage_type_helper
{
    using type = std::remove_reference_t<T>;
    using proxy_type = T&&;
    using constructor_type = T;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>::type;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Alignment, typename... Alignments>
struct reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::true_type,std::tuple<Alignment,Alignments...>>
{
    using type = aligned_type_holder<std::remove_reference_t<T>,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using proxy_type = T&&;
    using constructor_type = aligned_type_holder<std::remove_reference_t<T>,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>::type;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Alignment, typename... Alignments>
struct reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::false_type,std::tuple<Alignment,Alignments...>>
{
    using type = std::remove_reference_t<T>;
    using proxy_type = T&&;
    using constructor_type = aligned_type_holder<std::remove_reference_t<T>,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>::type;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Stored, std::size_t Alignment>
struct reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,Stored,std::integral_constant<std::size_t, Alignment>>
    : reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,Stored,
        construction_alignments_type<T,ArgumentsTuple,std::integral_constant<std::size_t, Alignment>>>
{};

}

}

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__TYPE_CONVERSION_HPP */

