/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014-2017 Colin MacLean <cmaclean@illinois.edu>
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

#ifndef MPIRPC__INTERNAL__RECONSTRUCTION__TYPE_PROPERTIES_HPP
#define MPIRPC__INTERNAL__RECONSTRUCTION__TYPE_PROPERTIES_HPP

#include <type_traits>
#include "../type_properties.hpp"

namespace mpirpc
{
    
template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple>
class construction_info;

namespace internal
{
    
namespace reconstruction
{
    
template<typename T, typename Alignment, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
class aligned_type_holder;



template<typename T>
struct is_construction_info : std::false_type {};

template<typename T>
using is_construction_info_type = typename is_construction_info<T>::type;

template<typename T>
constexpr bool is_construction_info_v = is_construction_info<T>::value;

template<typename T>
struct is_aligned_type_holder;

template<typename T>
using is_aligned_type_holder_type = typename is_aligned_type_holder<T>::type;

template<typename T>
constexpr bool is_aligned_type_holder_v = is_aligned_type_holder<T>::value;

template<typename T, typename Store, typename Alignment, typename = void>
struct is_stored : std::false_type {};

template<typename T, typename Store, typename Alignment>
using is_stored_type = typename is_stored<T,Store,Alignment>::type;

template<typename T, typename Store, typename Alignment>
constexpr bool is_stored_v = is_stored<T,Store,Alignment>::value;

template<typename T, typename Store, typename Alignment, typename = void>
struct is_static_construct_temporary : std::false_type {};

template<typename T, typename Store, typename Alignment>
using is_static_construct_temporary_type = typename is_static_construct_temporary<T,Store,Alignment>::type;

template<typename T, typename Store, typename Alignment>
constexpr bool is_static_construct_temporary_v = is_static_construct_temporary<T,Store,Alignment>::value;


template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple>
struct is_construction_info<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>> : std::true_type {};

template<typename T, typename Store, typename Alignment>
struct is_stored<T,Store,Alignment,
                      std::enable_if_t<
                          Store::value ||
                          is_construction_info<T>::value ||
                          (
                              std::is_reference<T>::value &&
                              mpirpc::internal::is_overaligned_type_v<T,Alignment>
                          )
                      >
                     >
        : std::true_type {};

template<typename T>
struct is_aligned_type_holder : std::false_type {};

template<typename T, typename Alignment, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
struct is_aligned_type_holder<aligned_type_holder<T,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,AlignmentsTuple>> : std::true_type {};

template<typename T, typename Store, typename Alignment>
struct is_static_construct_temporary<T,Store,Alignment,
                      std::enable_if_t<
                          is_construction_info<T>::value ||
                          (
                              std::is_reference<T>::value &&
                              is_overaligned_type_v<T,Alignment>
                          )
                      >
                     >
        : std::true_type {};

}

}

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__TYPE_PROPERTIES_HPP */
