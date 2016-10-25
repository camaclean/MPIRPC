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

#ifndef MPIRPC__INTERNAL__ORDEREDCALL_HPP
#define MPIRPC__INTERNAL__ORDEREDCALL_HPP

#include "../common.hpp"
#include "../pointerwrapper.hpp"
#include "detail/orderedcall.hpp"
#include "function_attributes.hpp"
#include "utility.hpp"

#include <tuple>
#include <iostream>
#include <type_traits>

namespace mpirpc
{

namespace internal
{

/**
 * @brief The mpirpc::ordered_call<F> class
 *
 * mpirpc::ordered_call<F> can be used to call functions such that the
 * side effects of the parameters are evaluated in the order
 * in which they appear. This is done using uniform initilization:
 * constructing using {} brackets instead of (). This bound call
 * can then be executed using mpirpc::ordered_call<F>::operator().
 */
template <typename F, typename Allocator>
struct ordered_call;

/**
 * Specialization of mpirpc::ordered_call<F> for function pointer calls.
 */
template<typename R, typename... FArgs, typename Allocator>
struct ordered_call<R(*)(FArgs...), Allocator>
{
    using function_type = R(*)(FArgs...);
    using storage_tuple = typename internal::wrapped_function_parts<function_type>::wrapped_args_tuple_type;

    template<typename... Args>
    ordered_call(internal::unwrapped_function_type<function_type> func, Allocator &a, Args&&... args)
        : function(func), args_tuple(std::forward<Args>(args)...), alloc(a)
    {
        //std::cout << abi::__cxa_demangle(typeid(argument_types<Args&&...>).name(),0,0,0) << std::endl;
    }

    R operator()()
    {
        return internal::apply(function, args_tuple);
    }

    ~ordered_call()
    {
        internal::detail::clean_up_args_tuple(alloc, args_tuple);
    }

    internal::unwrapped_function_type<function_type> function;
    storage_tuple args_tuple;
    Allocator &alloc;
};

/**
 * Specialization of OrderedCall<F> for member function pointer calls.
 */
template<typename R, typename Class, typename... FArgs, typename Allocator>
struct ordered_call<R(Class::*)(FArgs...), Allocator>
{
    using function_type = R(Class::*)(FArgs...);
    using storage_tuple = typename internal::wrapped_function_parts<function_type>::wrapped_args_tuple_type;

    template<typename... Args>
    ordered_call(internal::unwrapped_function_type<function_type> func, Class *c, Allocator &a, Args&&... args)
        : function(func), obj(c), args_tuple(std::forward<Args>(args)...), alloc(a)
    {}

    R operator()()
    {
        return internal::apply(function, obj, args_tuple);;
    }

    ~ordered_call()
    {
        internal::detail::clean_up_args_tuple(alloc, args_tuple);
    }

    internal::unwrapped_function_type<function_type> function;
    Class *obj;
    storage_tuple args_tuple;
    Allocator &alloc;
};

/**
 * Specialization of OrderedCall<F> for std::function calls.
 */
template<typename R, typename... FArgs, typename Allocator>
struct ordered_call<std::function<R(FArgs...)>, Allocator>
{
    using function_type = std::function<R(FArgs...)>;
    using storage_tuple = typename internal::wrapped_function_parts<function_type>::wrapped_args_tuple_type;

    template<typename... Args>
    ordered_call(internal::unwrapped_function_type<function_type>& func, Allocator &a, Args&&... args)
        : function(func), args_tuple(std::forward<Args>(args)...), alloc(a)
    {}

    R operator()()
    {
        return internal::apply(function, args_tuple);
    }

    ~ordered_call()
    {
        internal::detail::clean_up_args_tuple(alloc, args_tuple);
    }

    internal::unwrapped_function_type<function_type> function;
    storage_tuple args_tuple;
    Allocator &alloc;
};

template<typename F, typename Allocator, typename Stream, std::enable_if_t<!std::is_same<function_return_type<F>,void>::value>* = nullptr>
void apply_stream(F&& f, Allocator &a, Stream&& in, Stream&& out)
{
    typename detail::argument_storage_info<typename wrapped_function_parts<F>::storage_tuple_type>::nmct_tuple nmct(std::allocator_arg_t{}, a);
    auto mct = detail::unmarshal_tuples<typename wrapped_function_parts<F>::storage_tuple_type>::unmarshal(a,in,nmct);
    out << detail::apply(std::forward<F>(f),mct,nmct);
    detail::marshal_pass_back<F>(out,mct,nmct);
    detail::clean_up_args_tuple(a,std::move(mct));
    detail::clean_up_args_tuple(a,std::move(nmct));
}

template<typename F, typename Allocator, typename Stream, std::enable_if_t<std::is_same<function_return_type<F>,void>::value>* = nullptr>
void apply_stream(F&& f, Allocator &a, Stream&& in, Stream&& out)
{
    typename detail::argument_storage_info<typename wrapped_function_parts<F>::storage_tuple_type>::nmct_tuple nmct(std::allocator_arg_t{}, a);
    auto mct = detail::unmarshal_tuples<typename wrapped_function_parts<F>::storage_tuple_type>::unmarshal(a,in,nmct);
    detail::apply(std::forward<F>(f),mct,nmct);
    detail::marshal_pass_back<F>(out,mct,nmct);
    detail::clean_up_args_tuple(a,mct);
    detail::clean_up_args_tuple(a,nmct);
}

}

}

#endif // MPIRPC__INTERNAL__ORDEREDCALL_HPP

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
