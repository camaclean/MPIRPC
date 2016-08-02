/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014  Colin MacLean <s0838159@sms.ed.ac.uk>
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

#ifndef ORDEREDCALL_HPP
#define ORDEREDCALL_HPP

#include "common.hpp"
#include "pointerwrapper.hpp"
#include "forwarders.hpp"

#include <tuple>
#include <iostream>

namespace mpirpc {

template <typename F, typename Tuple, size_t... I>
decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return std::forward<F>(f)(static_cast<std::tuple_element_t<I,typename FunctionParts<std::remove_reference_t<F>>::args_tuple_type>>(std::get<I>(std::forward<Tuple>(t)))...);
}

template <typename F, class Class, typename Tuple, size_t... I>
decltype(auto) apply_impl(F&& f, Class *c, Tuple&& t, std::index_sequence<I...>)
{
    return ((*c).*(std::forward<F>(f)))(static_cast<std::tuple_element_t<I,typename FunctionParts<std::remove_reference_t<F>>::args_tuple_type>>(std::get<I>(std::forward<Tuple>(t)))...);
}

template <typename F, typename Tuple>
decltype(auto) apply(F&& f, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
}

template <typename F, class Class, typename Tuple>
decltype(auto) apply(F&& f, Class *c, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return apply_impl(std::forward<F>(f), c, std::forward<Tuple>(t), Indices{});
}

template<typename T>
struct arg_cleanup
{
    static void apply(typename std::remove_reference<T>::type& t) { std::cout << "blank clean up for " << typeid(t).name() << std::endl; }
    static void apply(typename std::remove_reference<T>::type&& t) { std::cout << "generic rvalue " << typeid(t).name() << std::endl; }
};

/*template<typename T>
struct arg_cleanup<PointerParameter<T>&>
{
    static void apply(PointerParameter<T>& t) { std::cout << "cleaned up pointer" << std::endl; delete t.pointer; }
};*/

/*template<typename T>
struct arg_cleanup<PointerParameter<T>&&>
{

    static void apply(PointerParameter<T>&& t) { std::cout << "not cleaning up pointer rvalue" << std::endl; delete t.pointer; }
};*/

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct arg_cleanup<PointerWrapper<T,N,PassOwnership,PassBack,Allocator>&>
{
    static void apply(PointerWrapper<T,N,PassOwnership,PassBack,Allocator>& t) { std::cout << "cleaned up C Array of " << typeid(T).name() << std::endl; t.free(); }
};

/*template<typename T, std::size_t N>
struct arg_cleanup<CArrayWrapper<T,N>>
{
    static void apply(CArrayWrapper<T,N> t) { std::cout << "cleaning up C Array lvalue " << typeid(t).name() <<  std::endl; t.del(); }
    static void apply(CArrayWrapper<T,N>&& t) { std::cout << "not cleaning up C Array rvalue " << typeid(t).name() <<  std::endl; }
};*/

template<bool C, typename T>
struct reapply_const_helper;

template<typename T>
struct reapply_const_helper<true,T>
{
    using type = const T;
};

template<typename T>
struct reapply_const_helper<false,T>
{
    using type = T;
};

template<typename FT, typename T>
using reapply_const_t = typename reapply_const_helper<std::is_const<FT>::value && !std::is_pointer<FT>::value,T>::type;

template<typename FT, typename T>
auto reapply_const(T&& t)
    -> typename reapply_const_helper<std::is_const<FT>::value && !std::is_pointer<FT>::value,T>::type
{
    return static_cast<typename reapply_const_helper<std::is_const<FT>::value && !std::is_pointer<FT>::value,T>::type>(t);
};

template<>
struct arg_cleanup<char*>
{
    static void apply(char*& s) { delete[] s; /*std::cout << "deleted s" << std::endl;*/ }
};

/*template<bool PassBack>
struct pass_back_helper;

template<>
struct pass_back_helper<true>
{
    template<typename T>
    static void process(T&& t)
    {
        std::cout << "Passing back " << typeid(t).name() << " " << t << std::endl;
    }
};

template<>
struct pass_back_helper<false>
{
    template<typename T>
    static void process(T&& t) {}
};

template<typename...FArgs>
struct pass_back
{
    template<typename... Args>
    static void do_pass_back(Args&&... args)
    {
        [](...){}( (pass_back_helper<is_pass_back<FArgs>::value>::process(std::forward<Args>(args)), 0)...);
    }

    static void pass(FArgs&... args)
    {
        [](...){}( (pass_back_helper<is_pass_back<FArgs>::value>::process(args), 0)...);
    }

};*/

/*template<typename F>
struct post_function_types;

template<typename R, class C, typename... Args>
struct post_function_types<R(C::*)(Args...)>
{
    using pass_back = pass_back<Args...>;
    //using cleanup = do_post_exec<Args...>;
};*/

template<typename... Args>
void do_post_exec(Args&&... args)
{
    [](...){}( (arg_cleanup<Args>::apply(std::forward<Args>(args)), 0)...);
}

/**
 * @brief The OrderedCall<F> class
 *
 * OrderedCall<F> can be used to call functions such that the
 * side effects of the parameters are evaluated in the order
 * in which they appear. This is done using uniform initilization:
 * constructing using {} brackets instead of (). This bound call
 * can then be executed using OrderedCall<F>::operator().
 */
template <typename F>
struct OrderedCall;

/**
 * Specialization of OrderedCall<F> for function pointer calls.
 */
template<typename R, typename... FArgs>
struct OrderedCall<R(*)(FArgs...)>
{
    using FunctionType = R(*)(FArgs...);
    using StorageTuple = typename storage_function_parts<FunctionType>::storage_tuple_type;

    OrderedCall(FunctionType func, std::remove_reference_t<FArgs>&&... args)
        : function(func), args_tuple(std::forward<FArgs>(args)...)
    {}

    template<typename T = R, typename std::enable_if<!std::is_same<T,void>::value,T>::type* = nullptr>
    R operator()()
    {
        T ret = apply(function, args_tuple);
        //apply(&pass_back<FArgs...>::pass, args_tuple);
        return ret;
    }

    template<typename T = R, typename std::enable_if<std::is_same<T,void>::value,void>::type* = nullptr>
    void operator()()
    {
        apply(function, args_tuple);
        //apply(&pass_back<FArgs...>::pass, args_tuple);
    }

    ~OrderedCall()
    {
        apply(&do_post_exec<FArgs...>, args_tuple);
    }

    FunctionType function;
    StorageTuple args_tuple;
};

/**
 * Specialization of OrderedCall<F> for member function pointer calls.
 */
template<typename R, typename Class, typename... FArgs>
struct OrderedCall<R(Class::*)(FArgs...)>
{
    using FunctionType = R(Class::*)(FArgs...);
    using StorageTuple = typename storage_function_parts<FunctionType>::storage_tuple_type;

    OrderedCall(FunctionType func, Class *c, std::remove_reference_t<FArgs>&&... args)
        : function(func), obj(c), args_tuple(std::forward<FArgs>(args)...)
    {}

    template<typename T = R, typename std::enable_if<!std::is_same<T,void>::value,T>::type* = nullptr>
    T operator()()
    {
        T ret = apply(function, obj, args_tuple);
        //apply(&pass_back<FArgs...>::pass, args_tuple);
        return ret;
    }

    template<typename T = R, typename std::enable_if<std::is_same<T,void>::value,void>::type* = nullptr>
    void operator()()
    {
        apply(function, obj, args_tuple);
        //apply(&pass_back<FArgs...>::pass, args_tuple);
    }

    ~OrderedCall()
    {
        apply(&do_post_exec<FArgs...>, args_tuple);
    }

    FunctionType function;
    Class *obj;
    StorageTuple args_tuple;
};

/**
 * Specialization of OrderedCall<F> for std::function calls.
 */
template<typename R, typename... FArgs>
struct OrderedCall<std::function<R(FArgs...)>>
{
    using FunctionType = std::function<R(FArgs...)>;
    using StorageTuple = typename storage_function_parts<FunctionType>::storage_tuple_type;

    OrderedCall(FunctionType& func, std::remove_reference_t<FArgs>&&... args)
        : function(func), args_tuple(std::forward<FArgs>(args)...)
    {}

    template<typename T = R, typename std::enable_if<!std::is_same<T,void>::value,T>::type* = nullptr>
    T operator()()
    {
        T ret = apply(function, args_tuple);
        //apply(&pass_back<FArgs...>::pass, args_tuple);
        return ret;
    }

    template<typename T = R, typename std::enable_if<std::is_same<T,void>::value,void>::type* = nullptr>
    void operator()()
    {
        apply(function, args_tuple);
        //apply(&pass_back<FArgs...>::pass, args_tuple);
    }

    ~OrderedCall()
    {
        apply(&do_post_exec<FArgs...>, args_tuple);
    }

    FunctionType function;
    StorageTuple args_tuple;
};

}

#endif // ORDEREDCALL_HPP
