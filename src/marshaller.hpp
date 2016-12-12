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
#include <string>

namespace mpirpc
{

template<typename T, typename Buffer, std::size_t Alignment, typename = void>
struct marshaller;

/*
template<typename T, typename Buffer, std::size_t Alignment>
struct marshaller<T,Buffer,Alignment,std::enable_if_t<std::is_pointer<T>::value>>
{
    static void marshal(Buffer& b, const T& t)
    {
        parameterbuffer_marshaller<mpirpc::pointer_wrapper<std::remove_pointer_t<T>>,alignof(mpirpc::pointer_wrapper<std::remove_pointer_t<T>>)>::marshal(b,mpirpc::pointer_wrapper<std::remove_pointer_t<T>>(t));
    }
};
*/

template<typename Buffer, std::size_t Alignment>
struct marshaller<std::string,Buffer,Alignment>
{
    static void marshal(Buffer& b, const std::string& s)
    {
        b.template push<char*>(s.c_str());
    }
};

}

#endif /* MPIRPC__MARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
