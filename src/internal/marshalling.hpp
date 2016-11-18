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

#ifndef MPIRPC__INTERNAL__MARSHALLING_HPP
#define MPIRPC__INTERNAL__MARSHALLING_HPP

#include "function_attributes.hpp"
#include "marshalling.hpp"
#include "utility.hpp"
#include "../types.hpp"
#include "../parameter_buffer.hpp"
#include "../marshaller.hpp"

#include <functional>

namespace mpirpc
{

namespace internal
{

/**
 * \internal
 * Marshalls the arguments Args... into the stream Stream using the types expected by the function F
 */
template<typename F>
struct fn_type_marshaller;

template<typename R, typename... FArgs>
struct fn_type_marshaller<R(*)(FArgs...)>
{
    template<class Buffer, typename... Args>
    static void marshal(Buffer& ps, Args&&... args)
    {
        using swallow = int[];
        (void)swallow{(::mpirpc::marshaller<autowrapped_type<FArgs>,Buffer,alignof(autowrapped_type<FArgs>)>::marshal(ps, autowrap<FArgs,Args>(args)),0)...};
    }
};

template<typename R, typename Class, typename... FArgs>
struct fn_type_marshaller<R(Class::*)(FArgs...)>
{
    template<class Buffer, typename... Args>
    static void marshal(Buffer& ps, Args&&... args)
    {
        using swallow = int[];
        (void)swallow{(::mpirpc::marshaller<autowrapped_type<FArgs>,Buffer,alignof(autowrapped_type<FArgs>)>::marshal(ps, autowrap<FArgs,Args>(args)),0)...};
    }
};


template<typename R, typename... FArgs>
struct fn_type_marshaller<std::function<R(FArgs...)>>
{
    template<class Buffer, typename... Args>
    static void marshal(Buffer& ps, Args&&... args)
    {
        using swallow = int[];
        (void)swallow{(::mpirpc::marshaller<autowrapped_type<FArgs>,Buffer,alignof(autowrapped_type<FArgs>)>::marshal(ps, autowrap<FArgs,Args>(args)),0)...};
    }
};

}

}

#endif /* MPIRPC__INTERNAL__MARSHALLING_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
