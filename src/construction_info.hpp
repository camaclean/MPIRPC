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

#ifndef MPIRPC__INTERNAL__CONSTRUCTION_INFO_HPP
#define MPIRPC__INTERNAL__CONSTRUCTION_INFO_HPP

#include "internal/reconstruction/type_conversion.hpp"
#include <experimental/type_traits>
#include "alignment.hpp"

namespace mpirpc
{


template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple>
class construction_info;

template<typename T, typename ConstructorArgumentTypesTuple, typename... ArgumentTypes, typename StoredArgumentsTuple>
struct default_alignment_helper<construction_info<T,ConstructorArgumentTypesTuple,std::tuple<ArgumentTypes...>,StoredArgumentsTuple>>
{
    using type = std::tuple<std::integral_constant<std::size_t,alignof(T)>,default_alignment_type<ArgumentTypes>...>;
};

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments>
class construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>
{
public:
    using constructor_argument_types_tuple = std::tuple<ConstructorArgumentTypes...>;
    using arguments_tuple = std::tuple<ArgumentTypes...>;
    using stored_arguments_tuple = std::tuple<StoredArguments...>;

    template<typename AlignmentsTuple>
    using aligned_type_holder = internal::reconstruction::construction_info_to_aligned_type_holder_type<construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>,AlignmentsTuple>;

    template<typename... Args, std::enable_if_t<std::is_constructible<arguments_tuple,Args...>::value>* = nullptr>
    construction_info(Args&&... args)
        : arguments{std::forward<Args>(args)...}
    {
        ////using swallow = int[];
        //(void)swallow{(std::cout << abi::__cxa_demangle(typeid(args).name(),0,0,0) << std::endl, 0)...};
        //static_assert(sizeof...(Args) == std::tuple_size<ArgumentsTuple>::value, "Must have the same number of arguments");
        //std::cout << sizeof...(Args) << std::endl;
        //std::cout << std::tuple_size<ArgumentsTuple>::value << std::endl;
        //std::cout << abi::__cxa_demangle(typeid(ArgumentsTuple).name(),0,0,0) << std::endl;
    }

    arguments_tuple& args() { return arguments; }

    static constexpr std::size_t num_args() { return sizeof...(ArgumentTypes); }
private:
    arguments_tuple arguments;
};

}

#endif /* MPIRPC__INTERNAL__CONSTRUCTION_INFO_HPP */
