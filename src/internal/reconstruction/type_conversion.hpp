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

#ifndef MPIRPC__INTERNAL__RECONSTRUCTION_TYPE_CONVERSION_HPP
#define MPIRPC__INTERNAL__RECONSTRUCTION_TYPE_CONVERSION_HPP

#include <tuple>
#include <../internal/alignment.hpp>

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

/*******************************************************************/


template<typename T, typename Alignment, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
class aligned_type_holder;

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
struct construction_info_to_aligned_type_holder<construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>,std::tuple<Alignments...>>
{
    using alignments = std::tuple<Alignments...>;
    using type_alignment = std::conditional_t<(sizeof...(Alignments) > 0),
                                              internal::alignment_reader_type<std::tuple<Alignments...>>,
                                              std::integral_constant<std::size_t,alignof(T)>
                                             >;
    using internal_alignments = std::conditional_t<(sizeof...(Alignments) == sizeof...(ArgumentTypes) +1),
                                                   internal::internal_alignments_tuple_type<std::tuple<Alignments...>>,
                                                   std::tuple<type_default_alignment<ArgumentTypes,alignof(ArgumentTypes)>...>
                                                  >;
    using type = aligned_type_holder<T,type_alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,internal_alignments>;
};

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, std::size_t Alignment>
struct construction_info_to_aligned_type_holder<construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>,std::integral_constant<std::size_t,Alignment>>
    : construction_info_to_aligned_type_holder<construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>,std::tuple<std::integral_constant<std::size_t,Alignment>>>
{};

template<typename T, typename Store, typename Alignment>
struct reconstruction_storage_type_helper
{
    using type = T;
    using constructor_type = T;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Alignment, typename... Alignments>
struct reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::true_type,std::tuple<Alignment,Alignments...>>
{
    using type = aligned_type_holder<T,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using constructor_type = aligned_type_holder<T,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>::type;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Alignment, typename... Alignments>
struct reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::false_type,std::tuple<Alignment,Alignments...>>
{
    using type = T;
    using constructor_type = aligned_type_holder<T,std::integral_constant<std::size_t,0ULL>,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>::type;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Stored, std::size_t Alignment>
struct reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,Stored,std::integral_constant<std::size_t, Alignment>>
    : reconstruction_storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,Stored,tuple_type_prepend_type<
        typename construction_info_to_aligned_type_holder<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::integral_constant<std::size_t,Alignment>>::type_alignment,
        typename construction_info_to_aligned_type_holder<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::integral_constant<std::size_t,Alignment>>::internal_alignments>>
{};

}

}

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION_TYPE_CONVERSION_HPP */

