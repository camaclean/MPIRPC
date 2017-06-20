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
      #define PACKED __attribute__((packed))
    #else
      #define MPIRPC_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define MPIRPC_EXPORT __attribute__ ((dllimport))
      #define PACKED __attribute__((packed))
    #else
      #define MPIRPC_EXPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define MPIRPC_LOCAL
#else
  #if __GNUC__ >= 4
    #define MPIRPC_EXPORT __attribute__ ((visibility ("default")))
    #define MPIRPC_LOCAL  __attribute__ ((visibility ("hidden")))
    #define PACKED __attribute__((packed))
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

template<typename T, typename Buffer, typename Alignment,typename=void>
struct marshaller;

/**
 * Constructor: References are kept valid for the duration of the object's constructor.
 *      The object is expected to either copy the reference or not need further use of the
 *      reference.
 *
 * function: References are kept valid for the duration of the function execution.
 *
 * function_group_shared: When constructing an array or container type, one instance of the
 *      reference is used for the construction of all members of the array or container.
 *      The array and any references it contains are valid for the duration of the function
 *      execution.
 *
 * manager: Reference storage is registered with the manager class. The reference is valid
 *      until deleted by the user through the manager instance.
 *
 * manager_group_shared: Like function_group_shared, one object is passed as a reference
 *      to the constructors of all members of the array or container. The array or container
 *      is valid until deleted by the user through the manager instance.
 */
enum class storage_duration_tag
{
    constructor,
    function,
    function_group_shared,
    manager,
    manager_group_shared
};

using constructor_storage_duration_tag_type = std::integral_constant<storage_duration_tag, storage_duration_tag::constructor>;
using function_storage_duration_tag_type = std::integral_constant<storage_duration_tag, storage_duration_tag::function>;
using function_group_shared_storage_duration_tag_type = std::integral_constant<storage_duration_tag, storage_duration_tag::function_group_shared>;
using manager_storage_duration_tag_type = std::integral_constant<storage_duration_tag, storage_duration_tag::manager>;
using manager_group_shared_storage_duration_tag_type = std::integral_constant<storage_duration_tag, storage_duration_tag::manager_group_shared>;

/**
 * unmarshaller should define a unmarshaller<T,Buffer,Alignment,void>::unmarshal<Allocator,Buffer>(Allocator,Buffer)
 * function which returns one of two types: T or construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<Arguments...>,std::tuple<StoredArguments...>>
 *
 * For example, to construct the type: std::tuple<int,int&,int&&,std::tuple<double,double&,double&&>>, the return type should be:
 * construction_info<std::tuple<int,int&,int&&,std::tuple<double,double&,double&&>>,
 *                             std::tuple<int,int,int,construction_info<std::tuple<double,double&,double&&>,
 *                                                                     std::tuple<double,double,double>,
 *                                                                     std::tuple<std::false_type,std::true_type,std::true_type>>,
 *                             std::tuple<std::false_type,std::true_type,std::true_type,std::false_type>>
 */
template<typename T, typename Buffer, typename Alignment, typename Options, typename = void>
struct unmarshaller;

using FnHandle = unsigned long long;
using TypeId = uintptr_t;
using ObjectId = unsigned long long;

}

#endif // MPIRPC__COMMON_HPP

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
