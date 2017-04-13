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

#ifndef MPIRPC__INTERNAL__ATH_DETAIL__CONSTRUCTION_INFO_HPP
#define MPIRPC__INTERNAL__ATH_DETAIL__CONSTRUCTION_INFO_HPP

#include <tuple>
#include "../type_conversion.hpp"
#include "../type_properties.hpp"
#include "../../utility.hpp"

namespace mpirpc
{

namespace internal
{

namespace reconstruction
{

namespace ath_detail
{

template<typename ConstructorArgumentsTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
struct stored_arguments_info;

template<std::size_t Size, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
struct stored_arguments_impl;

template<std::size_t Size, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
struct stored_arguments_impl2;

template<std::size_t Size, typename Argument, typename StoredArgument, typename Alignment>
struct stored_arguments_impl<Size,std::tuple<Argument>,std::tuple<StoredArgument>,std::tuple<Alignment>>
{
    using type = reconstruction_storage_type<Argument,StoredArgument,Alignment>;
    using types = std::conditional_t<
                        is_stored_v<Argument,StoredArgument,Alignment>
                      , std::tuple<std::remove_reference_t<type>>
                      , std::tuple<>
                  >;
    using tuple = std::conditional_t<
                        is_stored_v<Argument,StoredArgument,Alignment>
                      , std::tuple<typename std::aligned_storage<sizeof(std::remove_reference_t<type>), mpirpc::internal::alignment_reader<Alignment>::value>::type>
                      , std::tuple<>
                  >;
    static constexpr std::size_t index = Size-1;
    using indexes = std::conditional_t<
                        is_stored_v<Argument,StoredArgument,Alignment>
                      , std::index_sequence<index>
                      , std::index_sequence<>
                  >;
    using static_construct_types = std::conditional_t<
                        is_static_construct_temporary_v<Argument,StoredArgument,Alignment>
                      , std::tuple<std::remove_reference_t<type>>
                      , std::tuple<>
                  >;
    using static_construct_tuple = std::conditional_t<
                        is_static_construct_temporary_v<Argument,StoredArgument,Alignment>
                      , std::tuple<typename std::aligned_storage<sizeof(std::remove_reference_t<type>), mpirpc::internal::alignment_reader<Alignment>::value>::type>
                      , std::tuple<>
                  >;
    using static_construct_indexes = std::conditional_t<
                        is_static_construct_temporary_v<Argument,StoredArgument,Alignment>
                      , std::index_sequence<index>
                      , std::index_sequence<>
                  >;
};

template<std::size_t Size, typename Argument, typename... Arguments, typename StoredArgument, typename... StoredArguments, typename Alignment, typename... Alignments>
struct stored_arguments_impl<Size,std::tuple<Argument,Arguments...>,std::tuple<StoredArgument,StoredArguments...>,std::tuple<Alignment,Alignments...>>
{
    using type = reconstruction_storage_type<Argument,StoredArgument,Alignment>;
    using prev = stored_arguments_impl<Size,std::tuple<Arguments...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using prev_types = typename prev::types;
    using prev_tuple = typename prev::tuple;
    using prev_indexes = typename prev::indexes;
    static constexpr std::size_t index = Size-sizeof...(Arguments)-1;
    using types = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , std::remove_reference_t<type>
                      , prev_types
                  >;
    using tuple = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , typename std::aligned_storage<sizeof(std::remove_reference_t<type>),mpirpc::internal::alignment_reader<Alignment>::value>::type
                      , prev_tuple
                  >;

    using indexes = mpirpc::internal::conditional_integer_sequence_prepend_type<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , prev_indexes
                      , index
                  >;
    using static_construct_types = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , std::remove_reference_t<type>
                      , typename prev::static_construct_types
                  >;
    using static_construct_tuple = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , typename std::aligned_storage<sizeof(std::remove_reference_t<type>),mpirpc::internal::alignment_reader<Alignment>::value>::type
                      , typename prev::static_construct_tuple
                  >;
    using static_construct_indexes = mpirpc::internal::conditional_integer_sequence_prepend_type<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , typename prev::static_construct_indexes
                      , index
                  >;
};

template<std::size_t Size, typename Argument, typename StoredArgument, typename Alignment>
struct stored_arguments_impl2<Size,std::tuple<Argument>,std::tuple<StoredArgument>,std::tuple<Alignment>>
{
    using type = reconstruction_storage_type<Argument,StoredArgument,Alignment>;
    //static constexpr std::size_t index = (StoredArgument::value || is_construction_info<type>::value) ? Size-1 : Size;
    static constexpr std::size_t index = (is_stored_v<type,StoredArgument,Alignment>) ? Size-1 : Size;
    using tuple_indexes = std::conditional_t<
                              is_stored_v<Argument,StoredArgument,Alignment>
                            , std::tuple<std::integral_constant<std::size_t,Size-1>>
                            , std::tuple<decltype(std::ignore)>
                          >;
};

template<std::size_t Size, typename Argument, typename... Arguments, typename StoredArgument, typename... StoredArguments, typename Alignment, typename... Alignments>
struct stored_arguments_impl2<Size,std::tuple<Argument,Arguments...>,std::tuple<StoredArgument,StoredArguments...>,std::tuple<Alignment,Alignments...>>
{
    using type = reconstruction_storage_type<Argument,StoredArgument,Alignment>;
    using prev_stored_arguments = stored_arguments_impl2<Size,std::tuple<Arguments...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using prev_tuple_indexes = typename prev_stored_arguments::tuple_indexes;

    //constexpr static std::size_t index = (StoredArgument::value) ? prev_stored_arguments::index - 1 : prev_stored_arguments::index;
    constexpr static std::size_t index = (is_stored_v<Argument,StoredArgument,Alignment>) ? prev_stored_arguments::index - 1 : prev_stored_arguments::index;

    using tuple_indexes = std::conditional_t<
                              is_stored_v<Argument,StoredArgument,Alignment>
                            , mpirpc::internal::tuple_type_prepend_type<
                                std::integral_constant<std::size_t,index>,
                                prev_tuple_indexes
                              >
                            , mpirpc::internal::tuple_type_prepend_type<
                                decltype(std::ignore),
                                prev_tuple_indexes
                              >
                          >;
};

template<typename...ConstructorArguments, typename... Arguments, typename... StoredArguments, typename... Alignments>
struct stored_arguments_info<std::tuple<ConstructorArguments...>,std::tuple<Arguments...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
    using constructor_arguments_tuple_type = std::tuple<ConstructorArguments...>;
    using arguments_tuple_type = std::tuple<Arguments...>;
    using stored_arguments_tuple_type = std::tuple<StoredArguments...>;
    using alignments_tuple_type = std::tuple<Alignments...>;
    using storage_tuple_type = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::tuple;
    using storage_types = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::types;
    using storage_tuple_indexes_type = typename stored_arguments_impl2<std::tuple_size<storage_tuple_type>::value,arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::tuple_indexes;
    using storage_tuple_reverse_indexes_type = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::indexes;
    using proxy_tuple_type = std::tuple<ConstructorArguments&&...>;
    using static_construct_types = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::static_construct_types;
    using static_construct_tuple = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::static_construct_tuple;
    using static_construct_indexes = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::static_construct_indexes;
};

}

}

}

}

#endif /* MPIRPC__INTERNAL__ATH_DETAIL__CONSTRUCTION_INFO_HPP */
