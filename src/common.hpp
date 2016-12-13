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

#ifndef MPIRPC__COMMON_HPP
#define MPIRPC__COMMON_HPP

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define MPIRPC_EXPORT __attribute__ ((dllexport))
    #else
      #define MPIRPC_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define MPIRPC_EXPORT __attribute__ ((dllimport))
    #else
      #define MPIRPC_EXPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define MPIRPC_LOCAL
#else
  #if __GNUC__ >= 4
    #define MPIRPC_EXPORT __attribute__ ((visibility ("default")))
    #define MPIRPC_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define MPIRPC_EXPORT
    #define MPIRPC_LOCAL
  #endif
#endif

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

template<typename Allocator = std::allocator<char>>
class parameter_buffer;

template<typename MessageInterface, typename Allocator = std::allocator<char>, typename Buffer = parameter_buffer<Allocator>>
class manager;

using FnHandle = unsigned long long;
using TypeId = unsigned long long;
using ObjectId = unsigned long long;

template<typename T>
struct type_identifier
{
    static constexpr TypeId id() { return reinterpret_cast<TypeId>(&id); }
};

}

#endif // MPIRPC__COMMON_HPP

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
