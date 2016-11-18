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

#ifndef MPIRPC__INTERNAL__POLYMORPHIC_HPP
#define MPIRPC__INTERNAL__POLYMORPHIC_HPP

namespace mpirpc
{

namespace internal
{

template<typename Allocator, typename Buffer>
struct polymorphic_factory_base
{
    virtual void* build(Allocator& a, Buffer& b, std::size_t count) = 0;
};

template<typename T, typename Allocator, typename Buffer>
struct polymorphic_factory : polymorphic_factory_base<Allocator,Buffer>
{
    virtual void* build(Allocator&a, Buffer& b, std::size_t count)
    {
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        AllocatorType na(a);
        T *ptr = std::allocator_traits<AllocatorType>::allocate(na,count);
        for (std::size_t i = 0; i < count; ++i)
        {
            piecewise_allocator_traits<AllocatorType>::construct(na,&ptr[i],mpirpc::get<T>(b,na));
        }
        return static_cast<void*>(ptr);
    }
};

}

}

#endif /* MPIRPC__INTERNAL__POLYMORPHIC_HPP */
