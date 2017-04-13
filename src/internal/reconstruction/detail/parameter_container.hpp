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

#ifndef MPIRPC__INTERNAL__RECONSTRUCTION__DETAIL__PARAMETER_CONTAINER_HPP
#define MPIRPC__INTERNAL__RECONSTRUCTION__DETAIL__PARAMETER_CONTAINER_HPP

#include "../../../unmarshaller.hpp"

namespace mpirpc
{

namespace internal
{

namespace reconstruction
{

namespace detail
{


template<typename T, typename Buffer, typename Alignment, typename=void>
struct parameter_aligned_storage;

template<typename T, typename Buffer, typename Alignment>
using parameter_aligned_storage_type = typename parameter_aligned_storage<T,Buffer,Alignment>::type;

template<typename T, typename Buffer, typename Alignment, typename>
struct parameter_aligned_storage
    : std::aligned_storage<sizeof(std::remove_reference_t<T>),alignment_reader<Alignment>::value>
{};

template<typename T, typename Buffer, typename Alignments>
struct parameter_aligned_storage<T,Buffer,Alignments,std::enable_if_t<is_construction_info_v<unmarshaller_type<T,Buffer,std::allocator<char>>>>>
    : std::aligned_storage<
        sizeof(construction_info_to_aligned_type_holder_type<unmarshaller_type<T,Buffer,std::allocator<char>>,Alignments>),
        alignof(construction_info_to_aligned_type_holder_type<unmarshaller_type<T,Buffer,std::allocator<char>>,Alignments>)
    >
{};

template<typename T, typename Buffer, typename Alignment, typename = void>
struct storage_type_conversion
{
    using type = std::aligned_storage_t<sizeof(T),mpirpc::internal::alignment_reader<Alignment>::value>;
};

template<typename T, typename Buffer, typename Alignments>
struct storage_type_conversion<T,Buffer,Alignments,std::enable_if_t<internal::reconstruction::is_construction_info_v<unmarshaller_type<T,Buffer,std::allocator<char>>>>>
{
    using type = typename unmarshaller_type<T,Buffer,std::allocator<char>>::template aligned_type_holder<Alignments>;
};

template<typename T, typename Buffer, typename Alignment>
using storage_type_conversion_type = typename storage_type_conversion<T,Buffer,Alignment>::type;

template<typename Tuple, typename Buffer, typename AlignmentsTuple>
struct storage_tuple_from_types;

template<typename Types, typename Buffer, typename Alignments>
struct storage_construction_types;

template<typename T, typename... Ts, typename Buffer, typename Alignment, typename... Alignments>
struct storage_construction_types<std::tuple<T,Ts...>, Buffer,std::tuple<Alignment,Alignments...>>
    : internal::tuple_type_prepend<storage_type_conversion_type<std::remove_reference_t<std::remove_cv_t<T>>,Buffer,Alignment>,
        typename storage_construction_types<std::tuple<Ts...>,Buffer,std::tuple<Alignments...>>::type>
{};

template<typename T, typename Buffer, typename Alignment>
struct storage_construction_types<std::tuple<T>,Buffer,std::tuple<Alignment>>
//    : internal::tuple_type_prepend<storage_type_conversion_type<T,Buffer,Alignment>,std::tuple<>>
{
    using stype = storage_type_conversion_type<T,Buffer,Alignment>;
    using type = internal::tuple_type_prepend_type<storage_type_conversion_type<T,Buffer,Alignment>,std::tuple<>>;
};

template<typename Types, typename Buffer, typename Alignments>
using storage_construction_types_type = typename storage_construction_types<Types,Buffer,Alignments>::type;

}

}

}

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__DETAIL__PARAMETER_CONTAINER_HPP */
