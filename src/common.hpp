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

#ifndef COMMON_HPP
#define COMMON_HPP

#include<cstddef>
#include<type_traits>
#include<utility>
#include<iostream>
#include<memory>

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__  << " line " << __LINE__ << ": " << message << std::endl; \
            std::exit(ERR_ASSERT); \
        } \
    } while (false)
#else /* NDEBUG */
#define ASSERT(condition, message) do {} while(false)
#endif

namespace mpirpc
{

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
class pointer_wrapper;

using FnHandle = unsigned long long;
using TypeId = unsigned long long;
using ObjectId = unsigned long long;

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

template<bool...Vs>
struct bool_tuple{};

template<typename T>
struct is_pass_back
{
    constexpr static bool value = std::is_lvalue_reference<T>::value &&
                                  !std::is_const<typename std::remove_reference<T>::type>::value &&
                                  !std::is_pointer<typename std::remove_reference<T>::type>::value &&
                                  !std::is_array<typename std::remove_reference<T>::type>::value;
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct is_pass_back<pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    constexpr static bool value = PassBack;
};

template<typename T>
struct pass_back_false
{
    constexpr static bool value = false;
};

}

#endif // COMMON_HPP

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
