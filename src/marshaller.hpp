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
#include "internal/type_properties.hpp"
#include <string>
#include <utility>

namespace mpirpc
{

template<typename T, typename Buffer, typename Alignment, typename = void>
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

template<typename Buffer, typename Alignment>
struct marshaller<std::string,Buffer,Alignment>
{
    static void marshal(Buffer& b, const std::string& s)
    {
        b.template push<char*>(s.c_str());
    }
};

template<typename Buffer, typename Alignment>
struct marshaller<decltype(std::ignore),Buffer,Alignment>
{
    template<typename T>
    static void marshal(Buffer& b, T&& t) {}
};

template<typename Buffer, typename Alignment, typename... Ts>
struct marshaller<std::tuple<Ts...>,Buffer,Alignment>
{
    template<std::size_t... Is>
    static void marshal_impl(Buffer &b, const std::tuple<Ts...>& t, std::index_sequence<Is...>)
    {
        using swallow = int[];
        //(void)swallow{ (marshaller<RTs,Buffer,alignof(RTs)>::marshal(b,std::get<Is>(t)), 0)... };
        //(void)swallow{ (marshaller<NTs,Buffer,alignof(NTs)>::marshal(b,std::get<Is>(t)), 0)... };
        //(void)swallow{ (marshaller<std::remove_reference_t<Ts>,Buffer,alignof(Ts)>::marshal(b, std::get<Is>(t)), 0)... };
        (void)swallow{ (marshaller<std::remove_reference_t<Ts>,Buffer,std::integral_constant<std::size_t,alignof(Ts)>>::marshal(b, std::get<Is>(t)), 0)... };
    }

    static void marshal(Buffer& b, const std::tuple<Ts...>& t)
    {
        marshal_impl(b,t,std::index_sequence_for<Ts...>{});
    }
};

template<typename T, std::size_t N, typename Buffer, typename Alignment>
struct marshaller<T[N],Buffer,Alignment>
{
    static void marshal(Buffer& b, const T(&v)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
            marshaller<T,Buffer,Alignment>::marshal(b,v[i]);
    }
};

}

#endif /* MPIRPC__MARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
