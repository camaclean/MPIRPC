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

#ifndef MPIRPC__INTERNAL__UNMARSHALLING_HPP
#define MPIRPC__INTERNAL__UNMARSHALLING_HPP

#include <utility>
#include "../unmarshalling.hpp"

namespace mpirpc
{

class parameter_stream;

//template<typename R, typename Allocator>
//R unmarshal(mpirpc:: parameter_stream& s);


namespace internal
{

namespace detail {
template<typename Allocator, typename Stream, typename... Ts, std::size_t... Is>
std::tuple<storage_type<Ts>...> unmarshal_into_tuple_impl(Allocator &a, Stream &s, std::index_sequence<Is...>)
{
    using R = std::tuple<storage_type<Ts>...>;
    //std::cout << abi::__cxa_demangle(typeid(R).name(),0,0,0) << " " << abi::__cxa_demangle(typeid(std::tuple<Ts...>).name(),0,0,0) << std::endl;
    R ret{(::mpirpc::unmarshaller_remote<storage_type<Ts>>::unmarshal(a,s))...};
    return ret;
}

}

template<typename... Ts>
struct tuple_unmarshaller_remote
{
    template<typename Allocator, typename Stream>
    static std::tuple<storage_type<Ts>...> unmarshal(Allocator &a, Stream &s)
    {
        return detail::unmarshal_into_tuple_impl<Allocator, Stream, Ts...>(a, s, std::make_index_sequence<sizeof...(Ts)>{});
    }
};

}

}

#endif /*MPIRPC__INTERNAL__UNMARSHALLING_HPP */
