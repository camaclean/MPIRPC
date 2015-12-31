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

#include <tuple>
#include <iostream>

namespace mpirpc {

template <typename F, typename Tuple, size_t... I>
decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return std::forward<F>(f)(std::get<I>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
decltype(auto) apply(F&& f, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
}

template<typename T>
struct arg_cleanup
{
    static void apply(typename std::decay<T>::type& t) { std::cout << "blank clean up for " << typeid(t).name() << std::endl; }
    static void apply(typename std::decay<T>::type&& t) { std::cout << "generic rvalue " << typeid(t).name() << std::endl; }
};

/*template<typename T>
struct arg_cleanup<PointerParameter<T>&>
{
    static void apply(PointerParameter<T>& t) { std::cout << "cleaned up pointer" << std::endl; delete t.pointer; }
};*/

template<typename T>
struct arg_cleanup<PointerParameter<T>&&>
{

    static void apply(PointerParameter<T>&& t) { std::cout << "not cleaning up pointer rvalue" << std::endl; delete t.pointer; }
};

template<typename T, std::size_t N>
struct arg_cleanup<PointerWrapper<T,N>&>
{
    static void apply(PointerWrapper<T,N>& t) { std::cout << "cleaned up C Array" << std::endl; t.del(); }
};

/*template<typename T, std::size_t N>
struct arg_cleanup<CArrayWrapper<T,N>>
{
    static void apply(CArrayWrapper<T,N> t) { std::cout << "cleaning up C Array lvalue " << typeid(t).name() <<  std::endl; t.del(); }
    static void apply(CArrayWrapper<T,N>&& t) { std::cout << "not cleaning up C Array rvalue " << typeid(t).name() <<  std::endl; }
};*/

template<>
struct arg_cleanup<char*>
{
    static void apply(char*& s) { delete[] s; /*std::cout << "deleted s" << std::endl;*/ }
};

template<typename... Args>
void do_post_exec(Args&&... args)
{
    Passer p{ (arg_cleanup<Args>::apply(std::forward<Args>(args)), 0)... };
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
    using FuncType = R(*)(FArgs...);

    template<typename... Args>
    OrderedCall(R(*function)(FArgs...), Args&&... args)
    {
        func = function;
        bound = std::bind([&](){ return function(forward_parameter_type_local<typename std::remove_cv<FArgs>::type,Args>(args)...);});
        post = std::bind([&](){ do_post_exec(forward_parameter_cleanup<typename std::remove_cv<FArgs>::type,Args>(args)...);}); // do_post_exec(forward_parameter_type<typename std::::remove_cv<FArgs>::type,Args>(args)...);}); //do_post_exec(static_cast<FArgs>(args)...);}); //forward_parameter_type_local<typename std::remove_cv<FArgs>::type,Args>(args)...);});
    }

    template<typename T = R, typename std::enable_if<!std::is_same<T,void>::value,T>::type* = nullptr>
    R operator()()
    {
        T ret = bound();
        post();
        return ret;
    }

    template<typename T = R, typename std::enable_if<std::is_same<T,void>::value,void>::type* = nullptr>
    void operator()()
    {
        bound();
        post();
    }

    FuncType func;
    std::function<R()> bound;
    std::function<void()> post;
};

/**
 * Specialization of OrderedCall<F> for member function pointer calls.
 */
template<typename R, typename Class, typename... FArgs>
struct OrderedCall<R(Class::*)(FArgs...)>
{
    template<typename... Args>
    OrderedCall(R(Class::*function)(FArgs...), Class *c, Args&&... args)
    {
        bound = std::bind(function, c, forward_parameter_type_local<typename std::remove_cv<FArgs>::type,Args>(args)...);
        post = std::bind(do_post_exec<std::decay_t<Args>&...>, args...);
    }

    template<typename T = R, typename std::enable_if<!std::is_same<T,void>::value,T>::type* = nullptr>
    T operator()()
    {
        T ret = bound();
        post();
        return ret;
    }

    template<typename T = R, typename std::enable_if<std::is_same<T,void>::value,void>::type* = nullptr>
    void operator()()
    {
        bound();
        post();
    }

    std::function<R()> bound;
    std::function<void()> post;
};

/**
 * Specialization of OrderedCall<F> for std::function calls.
 */
template<typename R, typename... FArgs>
struct OrderedCall<std::function<R(FArgs...)>>
{
    template<typename... Args>
    OrderedCall(std::function<R(FArgs...)> &function, Args&&... args)
    {
        bound = std::bind(function, forward_parameter_type_local<typename std::remove_cv<FArgs>::type,Args>(args)...);
        post = std::bind(do_post_exec<std::decay_t<Args>&...>, args...);
    }

    template<typename T = R, typename std::enable_if<!std::is_same<T,void>::value,T>::type* = nullptr>
    T operator()()
    {
        T ret = bound();
        post();
        return ret;
    }

    template<typename T = R, typename std::enable_if<std::is_same<T,void>::value,void>::type* = nullptr>
    void operator()()
    {
        bound();
        post();
    }

    std::function<R()> bound;
    std::function<void()> post;
};

}

#endif // ORDEREDCALL_HPP
