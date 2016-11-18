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

#ifndef MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP
#define MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP

#include <tuple>
#include <utility>
#include "../pointerwrapper.hpp"
#include "../unmarshalling.hpp"
#include <memory>

namespace mpirpc
{

namespace internal
{

namespace detail
{

template<typename T>
struct arg_cleanup
{
    template<typename Allocator>
    static void apply(Allocator&& a, typename std::remove_reference<T>::type&)
    {
        std::cout << "blank clean up for " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
    }

    template<typename Allocator>
    static void apply(Allocator&& a, typename std::remove_reference<T>::type&&)
    {
        std::cout << "generic rvalue " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
    }
};

template<typename T>
struct arg_cleanup<pointer_wrapper<T>>
{
    template<typename Allocator, typename U, std::enable_if_t<std::is_same<std::decay_t<pointer_wrapper<T>>,std::decay_t<U>>::value>* = nullptr>
    static void apply(Allocator&& a, U&& t)
    {
        std::cout << "cleaned up C Array of " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
        t.free(a);
    }
};

template<typename T, std::size_t N>
struct arg_cleanup<T(&)[N]>
{
    template<typename Allocator>
    static void apply(Allocator&& a, T(&v)[N])
    {
        //std::cout << "cleaning up reference to array" << std::endl;
        using NA = typename std::allocator_traits<std::remove_reference_t<Allocator>>::template rebind_alloc<std::remove_const_t<T>>;
        NA na(a);
        for (std::size_t i = 0; i < N; ++i)
            mpirpc::array_destroy_helper<T>::destroy(a,v[i]);
        std::allocator_traits<NA>::deallocate(na,(std::remove_const_t<T>*) &v,N);
    }
};

template<typename T, std::size_t N>
struct arg_cleanup<T(&&)[N]>
{
    template<typename Allocator>
    static void apply(Allocator&& a, T(&&v)[N])
    {
        std::cout << "cleaning up rvalue reference to array" << std::endl;
        using NA = typename std::allocator_traits<std::remove_reference_t<Allocator>>::template rebind_alloc<std::remove_const_t<T>>;
        NA na(a);
        for (std::size_t i = 0; i < N; ++i)
            mpirpc::array_destroy_helper<T>::destroy(a,v[i]);
        std::allocator_traits<NA>::deallocate(na,(std::remove_const_t<T>*) &v,N);
    }
};

template<>
struct arg_cleanup<char*>
{
    template<typename Allocator>
    static void apply(Allocator&& a, char*&& s) { delete[] s; std::cout << "deleted char*" << std::endl; }
};

template<>
struct arg_cleanup<const char*>
{
    template<typename Allocator>
    static void apply(Allocator&& a, const char*&& s)
    {
        delete[] s;
        std::cout << "deleted const char*" << std::endl;
    }
};


} //detail

} //internal

} //mpirpc

#endif /* MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP */
