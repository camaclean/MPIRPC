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

#ifndef MPIRPC__INTERNAL__TYPE_PROPERTIES_HPP
#define MPIRPC__INTERNAL__TYPE_PROPERTIES_HPP

#include <type_traits>
#include <tuple>
#include "../buffer.hpp"
#include "../types.hpp"

namespace mpirpc
{

namespace internal
{

template<typename...Ts>
struct type_pack {};

template<typename T, typename TP>
struct prepend_type_pack;

template<typename T, typename... Us>
struct prepend_type_pack<T,type_pack<Us...>>
{
    using type = type_pack<T,Us...>;
};

template<typename T, typename TP>
using prepend_type_pack_type = typename prepend_type_pack<T,TP>::type;

template<typename T>
struct is_piecewise_construct_tuple : std::false_type {};

template<typename...Ts>
struct is_piecewise_construct_tuple<std::tuple<std::piecewise_construct_t,Ts...>> : std::true_type{};

template<typename Buffer, typename T, typename Alignment>
constexpr std::size_t unpacked_size_v;

template<typename... Ts>
struct tuple_reference_storage_types_helper;

template<typename T, typename...Ts>
struct tuple_reference_storage_types_helper<T,Ts...>
{
    using type = std::conditional_t<std::is_reference<T>::value,
                                    prepend_type_pack_type<std::remove_reference_t<T>,typename tuple_reference_storage_types_helper<Ts...>::type>,
                                    prepend_type_pack_type<decltype(std::ignore),typename tuple_reference_storage_types_helper<Ts...>::type>
                                   >;
};

template<typename T>
struct tuple_reference_storage_types_helper<T>
{
    using type = std::conditional_t<std::is_reference<T>::value,
                                    type_pack<std::remove_reference_t<T>>,
                                    type_pack<decltype(std::ignore)>
                                   >;
};

template<typename... Ts>
using tuple_reference_types = typename tuple_reference_storage_types_helper<Ts...>::type;

template<typename...Ts>
struct tuple_nonreference_storage_types_helper;

template<typename T, typename...Ts>
struct tuple_nonreference_storage_types_helper<T,Ts...>
{
    using type = std::conditional_t<!std::is_reference<T>::value,
                                    prepend_type_pack_type<T,typename tuple_nonreference_storage_types_helper<Ts...>::type>,
                                    prepend_type_pack_type<decltype(std::ignore),typename tuple_nonreference_storage_types_helper<Ts...>::type>
                                   >;
};

template<typename T>
struct tuple_nonreference_storage_types_helper<T>
{
    using type = std::conditional_t<!std::is_reference<T>::value,
                                    type_pack<T>,
                                    type_pack<decltype(std::ignore)>
                                   >;
};

template<typename... Ts>
using tuple_nonreference_types = typename tuple_nonreference_storage_types_helper<Ts...>::type;

constexpr static std::size_t calculate_alignment_padding(std::size_t addr, std::size_t alignment);

template<std::size_t Pos, std::size_t StartOffset, typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename T, typename AlignmentInfo>
struct tuple_reference_storage_info;

template<std::size_t Pos, std::size_t StartOffset, typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts, typename... Alignments>
struct tuple_reference_storage_info<Pos, StartOffset, Buffer, SkipBuildTypes, SkipNonBuildTypes, std::tuple<Ts...>, std::tuple<Alignments...>>
{
    using type = std::tuple_element_t<Pos,std::tuple<Ts...>>;
    static constexpr bool predicate = (is_buildtype<std::remove_reference_t<type>,Buffer>() && !SkipBuildTypes) || (!is_buildtype<std::remove_reference_t<type>,Buffer>() && !SkipNonBuildTypes) && std::is_reference<type>::value;
    static constexpr std::size_t prev_end_address_offset = tuple_reference_storage_info<Pos-1,StartOffset,Buffer,SkipBuildTypes,SkipNonBuildTypes,std::tuple<Ts...>,std::tuple<Alignments...>>::end_address_offset;
    static constexpr std::size_t padding = (predicate) ? mpirpc::internal::calculate_alignment_padding(prev_end_address_offset,std::tuple_element_t<Pos,std::tuple<Alignments...>>::value) : 0;
    static constexpr std::size_t start_address_offset = (predicate) ? prev_end_address_offset + padding : prev_end_address_offset;
    static constexpr std::size_t end_address_offset = (predicate) ? start_address_offset + unpacked_size_v<std::remove_reference_t<type>> : start_address_offset;
    static constexpr std::size_t total_padding = tuple_reference_storage_info<Pos-1,StartOffset,Buffer,SkipBuildTypes,SkipNonBuildTypes,std::tuple<Ts...>,std::tuple<Alignments...>>::total_padding + padding;
    static constexpr std::size_t total_size = (predicate) ? tuple_reference_storage_info<Pos-1,StartOffset,Buffer,SkipBuildTypes,SkipNonBuildTypes,std::tuple<Ts...>,std::tuple<Alignments...>>::total_size + padding + unpacked_size_v<std::remove_reference_t<type>> :
                                                            tuple_reference_storage_info<Pos-1,StartOffset,Buffer,SkipBuildTypes,SkipNonBuildTypes,std::tuple<Ts...>,std::tuple<Alignments...>>::total_size;
};

template<typename Buffer, std::size_t StartOffset, bool SkipBuildTypes, bool SkipNonBuildTypes, typename... Ts, typename... Alignments>
struct tuple_reference_storage_info<0, StartOffset, Buffer, SkipBuildTypes, SkipNonBuildTypes, std::tuple<Ts...>, std::tuple<Alignments...>>
{
    using type = std::tuple_element_t<0,std::tuple<Ts...>>;
    static constexpr bool predicate = (is_buildtype<std::remove_reference_t<type>,Buffer>() && !SkipBuildTypes) || (!is_buildtype<std::remove_reference_t<type>,Buffer>() && !SkipNonBuildTypes) && std::is_reference<type>::value;
    static constexpr std::size_t prev_end_address_offset = StartOffset;
    static constexpr std::size_t padding = (predicate) ? calculate_alignment_padding(prev_end_address_offset,std::tuple_element_t<0,std::tuple<Alignments...>>::value) : 0;
    static constexpr std::size_t start_address_offset = (predicate) ? prev_end_address_offset + padding : prev_end_address_offset;
    static constexpr std::size_t end_address_offset = (predicate) ? start_address_offset + unpacked_size_v<std::remove_reference_t<type>> : start_address_offset;
    static constexpr std::size_t total_padding = padding;
    static constexpr std::size_t total_size = (predicate) ? unpacked_size_v<std::remove_reference_t<type>> : 0;
};

template<typename Buffer, std::size_t StartOffset, typename T, typename Alignment>
struct tuple_storage_info;

template<typename Buffer, std::size_t StartOffset, typename... Ts, typename Alignment, typename... TypeAlignments>
struct tuple_storage_info<Buffer, StartOffset, std::tuple<Ts...>,std::tuple<Alignment,TypeAlignments...>>
{
    using reference_types = tuple_reference_types<Ts...>;
    static constexpr std::size_t size = tuple_reference_storage_info<sizeof...(Ts)-1,StartOffset,Buffer,false,true,reference_types,std::tuple<TypeAlignments...>>::total_size;

    template<std::size_t Pos>
    static constexpr std::size_t address = tuple_reference_storage_info<Pos,StartOffset, Buffer,false,true,reference_types,std::tuple<TypeAlignments...>>::start_address_offset;

    static constexpr std::size_t size_with_end_padding = size + mpirpc::internal::calculate_alignment_padding(size,Alignment::value);
};

template<typename Tuple>
struct tuple_reference_storage_default_alignments_impl;

template<typename... Ts>
struct tuple_reference_storage_default_alignments_impl<std::tuple<Ts...>>
{
    using type = std::tuple<type_default_alignment<Ts,alignof(Ts)>...>;
};

template<typename Tuple>
struct tuple_reference_storage_default_alignments;

template<typename...Ts>
struct tuple_reference_storage_default_alignments<std::tuple<Ts...>>
{
    using type = typename tuple_reference_storage_default_alignments_impl<tuple_reference_types<Ts...>>::type;
};

/**
 * std::tuple<float&,bool,std::tuple<double&,std::string,int&>,std::tuple<int&>&,int&,double>
 *
 * buffer: padding + sizeof(tuple) + {padding + sizeof(float) + {} + {{padding(4)} + sizeof(double) + padding(0) + sizeof(int)} + {padding(4) + sizeof(std::tuple<int&>) + padding(0) + sizeof(int)} + {padding(0) + sizeof(int)} + {}
 */
template<typename Buffer, typename T, typename Alignment, std::size_t StartOffset>
struct storage_properties
{
    using type = T;
    static constexpr std::size_t padding = mpirpc::internal::calculate_alignment_padding(StartOffset,Alignment::value);
    static constexpr std::size_t size = sizeof(T);
    static constexpr std::size_t total_size = size;

};
template<typename Buffer, typename... Ts, typename Alignment, typename... Alignments, std::size_t StartOffset>
struct storage_properties<Buffer, std::tuple<Ts...>, std::tuple<Alignment,Alignments...>, StartOffset>
{
    using type = std::tuple<Ts...>;
    static constexpr std::size_t padding = calculate_alignment_padding(StartOffset,Alignment::value);
    static constexpr std::size_t size = sizeof(std::tuple<Ts...>) + tuple_reference_storage_info<sizeof...(Ts)-1,sizeof(std::tuple<Ts...>),Buffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>::total_size;

};


template<typename Tuple, typename... Us>
struct tuple_constructor_properties;

template<typename T, typename... Ts, typename U, typename... Us>
struct tuple_constructor_properties<std::tuple<T,Ts...>,U,Us...>
{
    static constexpr bool convert_constructible = std::is_constructible<T,U&&>::value && tuple_constructor_properties<std::tuple<Ts...>,Us...>::convert_constructible;
    static constexpr bool copy_constructible = std::is_copy_constructible<T>::value && tuple_constructor_properties<std::tuple<Ts...>,Us...>::copy_constructible;
    static constexpr bool default_constructible = std::is_default_constructible<T>::value && tuple_constructor_properties<std::tuple<Ts...>,Us...>::default_constructible;
};

template<typename Buffer, typename T, typename Alignment>
constexpr std::size_t unpacked_size_v = sizeof(T);

template<typename Buffer, typename... Ts, typename... Alignments>
constexpr std::size_t unpacked_size_v<Buffer, std::tuple<Ts...>,std::tuple<Alignments>> = sizeof(std::tuple<Ts...>) + tuple_reference_storage_info<sizeof...(Ts)-1,sizeof(std::tuple<Ts...>),Buffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>::total_size;

}

}

#endif /* MPIRPC__INTERNAL__TYPE_PROPERTIES_HPP */
