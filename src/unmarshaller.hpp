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

#include "parameterstream.hpp"
#include "polymorphic.hpp"
#include "internal/direct_initializer.hpp"
#include "internal/parameterstream.hpp"
#include "internal/polymorphic.hpp"

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

template<typename T, typename Buffer, std::size_t alignment, typename = void>
struct unmarshaller;

template<typename T, typename Buffer, std::size_t Alignment>
struct unmarshaller<mpirpc::pointer_wrapper<T>,Buffer,Alignment>
{
    template<typename Allocator>
    static std::tuple<std::piecewise_construct_t,T*,std::size_t,bool,bool> unmarshal(Allocator& a, Buffer &b)
    {
        T *ptr;
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        bool pass_back = false, pass_ownership = false;

        if (pass_ownership || is_buildtype<T,Buffer>)
        {
            if (std::is_polymorphic<T>::value)
            {
                using VoidAllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;
                VoidAllocatorType va(a);
                uintptr_t type = mpirpc::get<uintptr_t>(b,a);
                ptr = static_cast<T*>(mpirpc::polymorphic_map<Buffer>.at(mpirpc::safe_type_index_map.at(type))->build(va,b,size));
            }
            else
            {
                using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
                AllocatorType na(a);
                ptr = std::allocator_traits<AllocatorType>::allocate(na,size);
                for (std::size_t i = 0; i < size; ++i)
                    internal::direct_initializer<T>::construct(na,&ptr[i],b);
            }
        }
        else
            get_pointer_from_buffer<alignof(T)>(b,ptr);
        return std::tuple<std::piecewise_construct_t,T*,std::size_t,bool,bool>{std::piecewise_construct,ptr,size,pass_back,pass_ownership};
    }
};

}

#endif /* MPIRPC__UNMARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
