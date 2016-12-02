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

#ifndef MPIRPC__POLYMORPHIC_HPP
#define MPIRPC__POLYMORPHIC_HPP

#include <map>
#include <typeindex>
#include "internal/polymorphic.hpp"

namespace mpirpc
{

static std::map<uintptr_t,std::type_index> safe_type_index_map;

template<typename Buffer>
static std::map<std::type_index,internal::polymorphic_factory_base<std::allocator<char>,Buffer>*> polymorphic_map;

template<class Allocator>
class parameter_buffer;

template<typename T, typename Buffer=parameter_buffer<std::allocator<char>>>
void register_polymorphism()
{
    safe_type_index_map.insert({mpirpc::type_identifier<T>::id(),std::type_index{typeid(T)}});
    polymorphic_map<Buffer>[std::type_index{typeid(T)}] = new internal::polymorphic_factory<T,std::allocator<char>,Buffer>();
}

}

#endif /* MPIRPC__POLYMORPHIC_HPP */
