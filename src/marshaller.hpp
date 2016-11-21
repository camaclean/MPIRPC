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

#ifndef MPIRPC__MARSHALLER_HPP
#define MPIRPC__MARSHALLER_HPP

#include "types.hpp"
#include "pointerwrapper.hpp"

namespace mpirpc
{

template<typename T, typename Buffer, std::size_t Alignment, typename = void>
struct marshaller;

template<typename T, typename Buffer, std::size_t Alignment>
struct marshaller<mpirpc::pointer_wrapper<T>, Buffer, Alignment>
{
    template<typename U = T, std::enable_if_t<!std::is_polymorphic<U>::value>* = nullptr>
    static void marshal(Buffer& b, const mpirpc::pointer_wrapper<U>& val)
    {
        b.template push<std::size_t>(val.size());
        for (std::size_t i = 0; i < val.size(); ++i)
        {
            b.template push<U>(val[i]);
        }
    }

    template<typename U = T, std::enable_if_t<std::is_polymorphic<U>::value>* = nullptr>
    static void marshal(Buffer& b, const mpirpc::pointer_wrapper<U>& val)
    {
        b.template push<std::size_t>(val.size());
        b.template push<uintptr_t>(type_identifier<U>::id());
        for (std::size_t i = 0; i < val.size(); ++i)
        {
            b.template push<U>(val[i]);
        }
    }
};

}

#endif /* MPIRPC__MARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
