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

#ifndef MPIRPC__INTERNAL__DIRECT_INITIALIZER_HPP
#define MPIRPC__INTERNAL__DIRECT_INITIALIZER_HPP

#include "piecewise_allocator_traits.hpp"
#include "../buffer.hpp"

namespace mpirpc
{

namespace internal
{

template<typename T>
struct direct_initializer
{
    template<typename Allocator, typename Buffer>
    static void construct(Allocator& a, T* t, Buffer&& s)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        piecewise_allocator_traits<AllocatorType>::construct(na,t,mpirpc::get<T>(s,a));
    }

    template<typename Allocator>
    static void destruct(Allocator& a, T* t)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        std::allocator_traits<AllocatorType>::destroy(na, t);
    }

    template<typename Allocator, typename Buffer>
    static void placementnew_construct(Allocator& a, T* t, Buffer&& b)
    {
        std::allocator<T> na;
        construct(na,t,std::forward<Buffer>(b));
    }

    template<typename Allocator>
    static void placementnew_destruct(const Allocator&, T* t)
    {
        t->~T();
    }
};

}

}

#endif /* MPIRPC__INTERNAL__DIRECT_INITIALIZER_HPP */
