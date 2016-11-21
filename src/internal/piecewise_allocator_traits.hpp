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

#ifndef MPIRPC__INTERNAL__PIECEWISE_ALLOCATOR_TRAITS_HPP
#define MPIRPC__INTERNAL__PIECEWISE_ALLOCATOR_TRAITS_HPP

namespace mpirpc
{

namespace internal
{

template<typename Allocator>
struct piecewise_allocator_traits
{
    template<typename T, typename Tuple, std::size_t...Is>
    static void tuple_construct(Allocator& a, T* p, Tuple&& tup, std::index_sequence<Is...>)
    {
        std::allocator_traits<Allocator>::construct(a,p,std::move(std::get<Is+1>(tup))...);
    }

    template<typename T,
             typename... Args,
             std::enable_if_t<std::is_constructible<T,Args...>::value>* = nullptr>
    static void construct(Allocator& a, T* p, std::tuple<std::piecewise_construct_t,Args...>&& val)
    {
        tuple_construct(a,p,std::move(val),std::make_index_sequence<sizeof...(Args)>{});
    }

    template<typename T,
             typename U,
             std::enable_if_t<std::is_constructible<T,U>::value>* = nullptr>
    static void construct(Allocator& a, T *p, U &&val)
    {
        std::allocator_traits<Allocator>::construct(a,p,std::forward<U>(val));
    }
};

}

}

#endif /* MPIRPC__INTERNAL__PIECEWISE_ALLOCATOR_TRAITS_HPP */
