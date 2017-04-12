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

#ifndef MPIRPC__DETAIL__UNMARSHALLER_HPP
#define MPIRPC__DETAIL__UNMARSHALLER_HPP

#include "../common.hpp"
#include "../alignment.hpp"

namespace mpirpc
{

namespace detail
{

template<typename T, typename Buffer, typename Allocator, typename=void>
struct has_unmarshaller_helper : std::false_type {};

template<typename T, typename Buffer, typename Allocator>
struct has_unmarshaller_helper<T,Buffer,Allocator,
    std::void_t<decltype(mpirpc::unmarshaller<T,Buffer,default_alignment_type<T>>::unmarshal(std::declval<Allocator&&>(),std::declval<Buffer&>()))>>
    : std::true_type
{};

template<typename T, typename Buffer, typename Allocator, typename=void>
struct unmarshaller_type_helper
{
    static_assert(std::is_same<void,T>::value, "ERROR: mpirpc::unmarshaller<T,Buffer,Alignment> not defined for this type");
    using type = decltype(mpirpc::unmarshaller<T,Buffer,default_alignment_type<T>>::unmarshal(std::declval<Allocator&&>(),std::declval<Buffer&>()));
};

template<typename T, typename Buffer, typename Allocator>
struct unmarshaller_type_helper<T,Buffer,Allocator,std::enable_if_t<has_unmarshaller_helper<T,Buffer,Allocator>::value>>
{
    using type = decltype(mpirpc::unmarshaller<T,Buffer,default_alignment_type<T>>::unmarshal(std::declval<Allocator>(),std::declval<Buffer&>()));
};

}

}

#endif /* MPIRPC__DETAIL__UNMARSHALLER_HPP */
