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

#ifndef MPIRPC__INTERNAL__PARAMETER_CONTAINER_HPP
#define MPIRPC__INTERNAL__PARAMETER_CONTAINER_HPP

#include <type_traits>
#include <utility>
#include "construction_info.hpp"
#include "aligned_type_holder.hpp"
#include "type_conversion.hpp"
#include "../../unmarshaller.hpp"
#include "../alignment.hpp"

namespace mpirpc
{

namespace internal
{

namespace reconstruction
{

template<typename T, typename Buffer, typename Alignment, typename=void>
struct parameter_aligned_storage;

template<typename Buffer, typename ArgsTuple, typename FArgsTuple, typename AlignmentsTuple>
class parameter_container;

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

template<typename Buffer, typename... Args, typename... FArgs, typename... Alignments>
class parameter_container<Buffer, std::tuple<Args...>, std::tuple<FArgs...>, std::tuple<Alignments...>>
{
public:
    //using storage_type = aligned_parameter_holder<std::tuple<Args...>,std::tuple<std::remove_reference_t<Args>...>,std::tuple<is_buildtype<std::remove_reference_t<Args>,Buffer>...>,std::tuple<Alignments...>>;
    using build_types = std::tuple<is_buildtype_type<std::remove_cv_t<std::remove_reference_t<Args>>,Buffer>...>;
    using stored_types = 
        internal::filter_tuple<
            std::tuple<Args...>,
            build_types
        >;
    using filtered_alignments = 
        internal::filter_tuple<
            std::tuple<Alignments...>,
            build_types
        >;
    using storage_tuple_type =
        internal::filter_tuple<
            storage_construction_types_type<
                std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>,
                Buffer,
                std::tuple<Alignments...>
            >,
            build_types
        >;
    
    using proxy_tuple_type = std::tuple<FArgs&&...>;
    
    template<std::size_t I>
    using proxy_type = std::tuple_element_t<I,proxy_tuple_type>;
    
    template<std::size_t I>
    using storage_index = std::tuple_element_t<I,filtered_tuple_indexes_type<build_types>>;
    
    template<std::size_t I>
    using alignment = std::tuple_element_t<I,std::tuple<Alignments...>>;
    
    template<std::size_t I>
    using arg_type = std::tuple_element_t<I,std::tuple<Args...>>;
    
    template<std::size_t I>
    using farg_type = std::tuple_element_t<I,std::tuple<FArgs...>>;
    
    template<std::size_t I>
    using storage_type = std::tuple_element_t<storage_index<I>::value,storage_tuple_type>;

protected:
    template<std::size_t I, typename Allocator,
        typename Type = std::remove_cv_t<std::remove_reference_t<arg_type<I>>>,
        std::enable_if_t<!is_buildtype_v<Type,Buffer>>* = nullptr>
    proxy_type<I> make_from_buffer(Buffer& b, Allocator&& a)
    {
        return static_cast<proxy_type<I>>(::mpirpc::get<arg_type<I>>(b,a));
    }
    
    template<std::size_t I, typename Allocator, 
        typename Type = std::remove_cv_t<std::remove_reference_t<arg_type<I>>>,
        std::enable_if_t<is_buildtype_v<Type,Buffer>>* = nullptr,
        std::enable_if_t<internal::reconstruction::is_aligned_type_holder_v<storage_type<I>>>* = nullptr>
    proxy_type<I> make_from_buffer(Buffer& b, Allocator&& a)
    {
        std::cout << "INDEXS: " << abi::__cxa_demangle(typeid(filtered_tuple_indexes_type<build_types>).name(),0,0,0) << std::endl;
        std::cout << "INDEX: " << abi::__cxa_demangle(typeid(storage_index<I>).name(),0,0,0) << std::endl;
        //using index_type = storage_index<I>;
        //constexpr auto index = index_type::value;
        auto &s = std::get<storage_index<I>::value>(m_storage);
        s.construct(mpirpc::get<arg_type<I>>(b,a).args());
        return static_cast<proxy_type<I>>(s.value());
    }
    
    template<std::size_t I, typename Allocator, 
        typename Type = std::remove_cv_t<std::remove_reference_t<arg_type<I>>>,
        std::enable_if_t<is_buildtype_v<Type,Buffer>>* = nullptr,
        std::enable_if_t<!is_aligned_type_holder_v<storage_type<I>>>* = nullptr>
    proxy_type<I> make_from_buffer(Buffer& b, Allocator&& a)
    {
        std::cout << "INDEXS: " << abi::__cxa_demangle(typeid(filtered_tuple_indexes_type<build_types>).name(),0,0,0) << std::endl;
        std::cout << "INDEX: " << abi::__cxa_demangle(typeid(storage_index<I>).name(),0,0,0) << std::endl;
        //using index_type = storage_index<I>;
        //constexpr auto index = index_type::value;
        Type* rawstorage = static_cast<Type*>(std::addressof(std::get<storage_index<I>::value>(m_storage)));
        new (rawstorage) Type(mpirpc::get<arg_type<I>>(b,a));
        return static_cast<proxy_type<I>>(*rawstorage);
    }
    
    template<typename Allocator, std::size_t...Is>
    parameter_container(Buffer& b, Allocator&& a, std::index_sequence<Is...>)
        : m_pt{make_from_buffer<Is>(b,a)...}
    {}
    
public:
    template<typename Allocator>
    parameter_container(Buffer& b, Allocator&& a)
        : parameter_container(b, a, std::index_sequence_for<Args...>{})
    {
    }
    
    template<std::size_t I>
    farg_type<I> get()
    {
        return static_cast<farg_type<I>>(std::get<I>(m_pt));
    }
    /*void prepare(Buffer& b)
    {
        //std::cout << 
    }*/
protected:
    storage_tuple_type m_storage;
    proxy_tuple_type m_pt;
};

}

}

}

#endif /* MPIRPC__INTERNAL__PARAMETER_CONTAINER_HPP */