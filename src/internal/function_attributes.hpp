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

#ifndef MPIRPC__INTERNAL__FUNCTION_ATTRIBUTES_HPP
#define MPIRPC__INTERNAL__FUNCTION_ATTRIBUTES_HPP

#include <tuple>
#include <functional>
#include "type_massaging.hpp"
#include "pass_back.hpp"
#include "../pointerwrapper.hpp"


namespace mpirpc
{

namespace internal
{

/**
 * \internal
 * mpirpc::internal::function_parts is used to get basic information about functions
 *
 * function_parts<F>::args_tuple_type
 *      A tuple of the arguments, std::tuple<Args...>
 *
 * function_parts<F>::class_type
 *      Class (optional)
 *
 * function_parts<F>::function_type
 *      The function type:
 *          * R(*)(Args...)
 *          * R(Class::*)(Args...)
 *          * std::function<R(Args...)>
 *
 * function_parts<F>::return_type
 *          The function return type, R
 */
template<typename F>
struct function_parts;

/**
 * \internal
 * A helper type to query the return type of a function
 */
template<typename F>
using function_return_type = typename function_parts<F>::return_type;

/**
 * \internal
 * mpirpc::internal::storage_function_parts is used to get information about wrapped function types
 * such as their unwrapped equivalents.
 *
 * Args...  is a parameter pack of wrapped function parameter types
 * FArgs... is a parameter pack of unwrapped function parameter types
 *
 * function_parts<F>::class_type
 *      Class (optional)
 *
 * function_parts<F>::wrapped_args_tuple_type
 *      std::tuple<std::remove_reference_t<Args>...>
 *
 * function_parts<F>::wrapped_function_type
 *      The wrapped function type:
 *          * R(*)(Args...)
 *          * R(Class::*)(Args...)
 *          * std::function<R(Args...)>
 *
 * function_parts<F>::function_type
 *      The unwrapped function type:
 *          * R(*)(FArgs...)
 *          * R(Class::*)(FArgs...)
 *          * std::function<R(FArgs...)>
 *
 * function_parts<F>::return_type
 *      The function return type, R
 */
template<typename F>
struct wrapped_function_parts;

/**
 * \internal
 * A helper type to query the wrapped function type
 */
template<typename F>
using wrapped_function_type = typename wrapped_function_parts<F>::wrapped_function_type;

/**
 * \internal
 * A helper type to query the unwrapped function type
 */
template<typename F>
using unwrapped_function_type = typename wrapped_function_parts<F>::unwrapped_function_type;

/**
 * \internal
 * mpirpc::internal::marshaller_function_signature is used to convert types passed to the function invocation methods to the
 * types expected by the function and the unmarshalling mechanisms.
 *
 * Args...  is a parameter pack of parameter types passed to the marshaller
 * FArgs... is a parameter pack of the function parameter types
 * R        is the deduced function return type
 *
 * marshaller_function_signature<F,Args...>::return_type
 *      The return type of the function, R
 *
 * marshaller_function_signature<F,Args...>::void_return_type
 *      Defined as void when #return_type is void. Useful for SFINAE.
 *
 * marshaller_function_signature<F,Args...>::non_void_return_type
 *      Defined as R when #return_type is non-void. Useful for SFINAE.
 *
 * marshaller_function_signature<F,Args...>::parameter_types
 *      A std::tuple with the base types from the function parameters but using the reference types of the values passed to the marshaller.
 *
 *      For instance, if F={void(*)(double)} and Args={float&}, parameter_types should be std::tuple<double&>
 *
 * marshaller_function_signature<F,Args...>::storage_tuple_type
 *      A std::tuple with the wrapped types of the arguments
 *
 * marshaller_function_signature<F,Args...>::applier
 *      The std::function type used to marshal the parameters. Useful for declaring a generic lambda.
 *
 * marshaller_function_signature<F,Args...>::pass_backs
 *      A bool_tuple indicating which arguments should be passed back to the sending PE
 */
template<typename F, typename... Args>
struct marshaller_function_signature;

/**
 * \internal
 * Used by mpirpc::manager to provide an object type unique to a function instance.
 * This is used to give each function instance a unique std::type_index
 */
template<typename Functor, internal::unwrapped_function_type<Functor> f>
struct function_identifier {};

/**
 * \internal
 * Used to get the return and parameter types of lambdas
 */
template <typename F>
struct lambda_traits;

/**
 * \internal
 * A helper type to query a lambda's std::function conversion type
 */
template<typename F>
using lambda_stdfunction_type = typename lambda_traits<F>::lambda_stdfunction;

/**
 * \internal
 * A helper type to query a lambda's function pointer conversion type
 */
template<typename F>
using lambda_fn_ptr_type = typename lambda_traits<F>::lambda_fn_ptr;

/**
 * \internal
 * A helper type to query a labda's return type
 */
template<typename F>
using lambda_return_type = typename lambda_traits<F>::return_type;

/*************************************************************************************/
/*************************************************************************************/
/*                                  Implementation                                   */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*                          mpirpc::internal::function_parts                         */
/*************************************************************************************/

template<typename R, class Class, typename... Args>
struct function_parts<R(Class::*)(Args...)>
{
    using return_type = R;
    using class_type = Class;
    using args_tuple_type = std::tuple<Args...>;
    using function_type = R(Class::*)(Args...);
};

template<typename R, typename... Args>
struct function_parts<R(*)(Args...)>
{
    using return_type = R;
    using args_tuple_type = std::tuple<Args...>;
    using function_type = R(*)(Args...);
};

template<typename R, typename... Args>
struct function_parts<std::function<R(Args...)>>
{
    using return_type = R;
    using args_tuple_type = std::tuple<Args...>;
    using function_type = std::function<R(Args...)>;
};

template<typename Arg>
struct storage_type_helper
{
    using type = std::conditional_t<std::is_array<std::remove_reference_t<Arg>>::value,std::remove_reference_t<Arg>&,std::decay_t<std::remove_reference_t<Arg>>>;
};

template<typename Arg>
using storage_type = typename storage_type_helper<Arg>::type;

/*************************************************************************************/
/*                      mpirpc::internal::storage_function_parts                     */
/*************************************************************************************/

template<typename R, class Class, typename... Args>
struct wrapped_function_parts<R(Class::*)(Args...)>
{
    using return_type = R;
    using class_type = Class;
    using wrapped_function_type = R(Class::*)(autowrapped_type<Args>...);
    using wrapped_args_tuple_type = std::tuple<autowrapped_type<std::conditional_t<std::is_array<std::remove_reference_t<Args>>::value,std::remove_reference_t<Args>&,std::remove_reference_t<Args>>>...>;
    using storage_tuple_type = std::tuple<storage_type<Args>...>;
    using unwrapped_function_type = R(Class::*)(unwrapped_type<Args>...);
};

template<typename R, typename... Args>
struct wrapped_function_parts<R(*)(Args...)>
{
    using return_type = R;
    using wrapped_function_type = R(*)(autowrapped_type<Args>...);
    using wrapped_args_tuple_type = std::tuple<autowrapped_type<std::conditional_t<std::is_array<std::remove_reference_t<Args>>::value,std::remove_reference_t<Args>&,std::remove_reference_t<Args>>>...>;
    using storage_tuple_type = std::tuple<storage_type<Args>...>;
    using unwrapped_function_type = R(*)(unwrapped_type<Args>...);
};

template<typename R, typename... Args>
struct wrapped_function_parts<std::function<R(Args...)>>
{
    using return_type = R;
    using wrapped_function_type = std::function<R(autowrapped_type<Args>...)>;
    using wrapped_args_tuple_type = std::tuple<autowrapped_type<std::conditional_t<std::is_array<std::remove_reference_t<Args>>::value,std::remove_reference_t<Args>&,std::remove_reference_t<Args>>>...>;
    using storage_tuple_type = std::tuple<storage_type<Args>...>;
    using unwrapped_function_type = std::function<R(unwrapped_type<Args>...)>;
};

/*************************************************************************************/
/*                    mpirpc::internal::marshaller_function_parts                    */
/*************************************************************************************/

template<typename...Args>
struct argument_types {};

template<typename R, typename... FArgs, typename... Args>
struct marshaller_function_signature<R(*)(FArgs...), Args...>
{
    using return_type = R;
    using non_void_return_type = std::enable_if_t<!std::is_same<R,void>::value,R>;
    using void_return_type = std::enable_if_t<std::is_same<R,void>::value,R>;
    using parameter_types = argument_types<autowrapped_type<FArgs>...>;
    using applier = std::function<void(choose_reference_type<FArgs,Args>...)>;
    using pass_backs = bool_template_list<is_pass_back<FArgs>::value...>;
};

/*template<typename... FArgs, typename... Args>
struct marshaller_function_signature<void(*)(FArgs...), Args...>
{
    using return_type = void;
    using void_return_type = void;
    using parameter_types = argument_types<autowrapped_type<FArgs,Allocator>...>;
    using applier = std::function<void(choose_reference_type<FArgs,Args>...)>;
    using pass_backs = bool_template_list<is_pass_back<FArgs>::value...>;
};*/

template<typename R, class C, typename... FArgs, typename... Args>
struct marshaller_function_signature<R(C::*)(FArgs...), Args...>
{
    using return_type = R;
    using non_void_return_type = std::enable_if_t<!std::is_same<R,void>::value,R>;
    using void_return_type = std::enable_if_t<std::is_same<R,void>::value,R>;
    using parameter_types = argument_types<autowrapped_type<FArgs>...>;
    using applier = std::function<void(choose_reference_type<FArgs,Args>...)>;
    using pass_backs = bool_template_list<is_pass_back<FArgs>::value...>;
};

/*template<class C, typename... FArgs, typename... Args>
struct marshaller_function_signature<void(C::*)(FArgs...), Args...>
{
    using return_type = void;
    using void_return_type = void;
    using parameter_types = argument_types<autowrapped_type<FArgs>...>;
    using applier = std::function<void(choose_reference_type<FArgs,Args>...)>;
    using pass_backs = bool_template_list<is_pass_back<FArgs>::value...>;
};*/

template<typename R, typename... FArgs, typename... Args>
struct marshaller_function_signature<std::function<R(FArgs...)>, Args...>
{
    using return_type = R;
    using non_void_return_type = std::enable_if_t<!std::is_same<R,void>::value,R>;
    using void_return_type = std::enable_if_t<std::is_same<R,void>::value,R>;
    using parameter_types = argument_types<autowrapped_type<FArgs>...>;
    using applier = std::function<void(typename choose_reference_type<FArgs,Args>::type...)>;
    using pass_backs = bool_template_list<is_pass_back<FArgs>::value...>;
};

/*template<typename... FArgs, typename... Args>
struct marshaller_function_signature<std::function<void(FArgs...)>, Args...>
{
    using return_type = void;
    using void_return_type = void;
    using parameter_types = argument_types<autowrapped_type<FArgs>...>;
    using applier = std::function<void(typename choose_reference_type<FArgs,Args>::type...)>;
    using pass_backs = bool_template_list<is_pass_back<FArgs>::value...>;
};*/

/*************************************************************************************/
/*                          mpirpc::internal::lambda_traits                          */
/*************************************************************************************/

template <typename F>
struct lambda_traits : public lambda_traits<decltype(&std::remove_reference_t<F>::operator())>
{};

template <typename C, typename R, typename... Args>
struct lambda_traits<R(C::*)(Args...) const>
{
    using lambda_fn_ptr       = R(*)(Args...);
    using lambda_stdfunction = std::function<R(Args...)>;
    using return_type = R;
    using pass_backs = bool_template_list<is_pass_back<Args>::value...>;
    using parameter_types = argument_types<autowrapped_type<Args>...>;
};

}

}

#endif /* MPIRPC__INTERNAL__FUNCTION_ATTRIBUTES_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
