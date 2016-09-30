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

namespace mpirpc
{

namespace internal
{

/**
 * \internal
 * Marshalls the arguments Args... into the stream Stream using the types expected by the function F
 */
template<typename F>
struct fn_type_marshaller
{
    template<class Stream, typename... Args>
    static void marshal(Stream& ps, Args&&... args)
    {
        using Applier = typename ::mpirpc::internal::marshaller_function_signature<F,Args...>::applier;
        Applier apply = [&](auto&&... a) { (void)::mpirpc::internal::passer{(::mpirpc::marshal(ps, std::forward<decltype(a)>(a)), 0)...}; };
        apply(autowrap<Args>(args)...);
    }
};

}

}

#endif /* MPIRPC__INTERNAL__MARSHALLING_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
