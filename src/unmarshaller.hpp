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

#ifndef MPIRPC__UNMARSHALLER_HPP
#define MPIRPC__UNMARSHALLER_HPP

#include <utility>
#include "internal/type_properties.hpp"

namespace mpirpc
{

/*
 * Unmarshalling is a significantly greater challenge than marshalling. When marshalling,
 * the input is already in a valid constructed state.
 *
 * However, when unmarshalling there may be one or more different limitations of the types
 * which need to be handled. First of all, there is the issue of allocating memory. This
 * will often be different for local and remote unmarshalling, since local unmarshalling
 * will already have references or pointers to already-existing memory pass in as parameters.
 * Remote unmarshalling, on the other hand, often requires allocating memory.
 */

template<typename T, typename Buffer, typename Alignment, typename = void>
struct unmarshaller;

template<typename R, typename...Ts, std::size_t...Is>
R construct_impl(const std::tuple<std::piecewise_construct_t,Ts...>& t, std::index_sequence<Is...>)
{
    return R(std::get<Is+1>(t)...);
}

template<typename R, typename...Ts>
R construct(const std::tuple<std::piecewise_construct_t,Ts...>& t)
{
    return construct_impl<R>(t,std::index_sequence_for<Ts...>{});
}

template<typename R, typename T, std::enable_if_t<!internal::is_piecewise_construct_tuple<std::remove_reference_t<T>>::value>* = nullptr>
R construct(T&& t)
{
    return R(std::forward<T>(t));
}

template<typename Buffer, typename Alignment, typename... Ts>
struct unmarshaller<std::tuple<Ts...>,Buffer,Alignment>
{
    template<typename Allocator>
    static std::tuple<std::remove_reference_t<Ts>...> unmarshal(Allocator&& a, Buffer& b)
    {
        //return std::tuple<std::remove_reference_t<Ts>...>{ construct<std::remove_reference_t<Ts>>(unmarshaller<Ts,Buffer,alignof(Ts)>::unmarshal(std::forward<Allocator>(a),b))... };
    }
};

}

#endif /* MPIRPC__UNMARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
