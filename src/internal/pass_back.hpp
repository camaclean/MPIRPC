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

#ifndef MPIRPC__INTERNAL__PASS_BACK_HPP
#define MPIRPC__INTERNAL__PASS_BACK_HPP

#include <type_traits>

namespace mpirpc
{

namespace internal
{

template<bool...Vs>
struct bool_template_list{};

template<typename T>
struct is_pass_back
{
    constexpr static bool value = std::is_lvalue_reference<T>::value &&
                                  !std::is_const<typename std::remove_reference<T>::type>::value &&
                                  !std::is_pointer<typename std::remove_reference<T>::type>::value &&
                                  !std::is_array<typename std::remove_reference<T>::type>::value;
};

template<typename T, std::size_t N>
struct is_pass_back<T(&)[N]>
{
    constexpr static bool value = true;
};

template<typename T, std::size_t N>
struct is_pass_back<T(*)[N]>
{
    constexpr static bool value = true;
};

template<typename T>
struct is_pass_back<::mpirpc::pointer_wrapper<T>>
{
    constexpr static bool value = true;
};

template<typename T>
struct pass_back_false
{
    using type = int;
    constexpr static bool value = false;
};

template<bool... Bs>
struct any_true;

template<bool B1, bool... Bs>
struct any_true<B1, Bs...>
{
    constexpr static bool value = B1 || any_true<Bs...>::value;
};

template<bool B>
struct any_true<B>
{
    constexpr static bool value = B;
};

template<>
struct any_true<>
{
    constexpr static bool value = false;
};

template<typename Buffer, typename FArg>
struct pass_back_unmarshaller
{
    template<typename Allocator, typename Arg>
    inline static void unmarshal(Buffer &s, Allocator&& a, Arg& arg)
    {
        arg = get<FArg>(s,a);
    }
};

template<typename T, typename U>
struct can_realloc
{
    constexpr static bool value = false;
};

template<typename T, typename U>
struct can_realloc<T*&,U*&>
{
    constexpr static bool value = std::is_same<std::remove_const_t<T>,std::remove_const_t<U>>::value;
};

}

}

#endif /* MPIRPC__INTERNAL__PASS_BACK_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
