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

#ifndef MPIRPC__TYPE_CONSTRUCTOR_HPP
#define MPIRPC__TYPE_CONSTRUCTOR_HPP

#include "../internal/utility.hpp"

namespace mpirpc
{

template<typename T,typename=void>
struct type_constructor
{
    template<typename... Args>
    static void construct(T *t, Args&&... args)
    {
        new (t) T(std::forward<Args>(args)...);
    }
};

template<typename... Ts>
struct type_constructor<std::tuple<Ts...>,
        std::enable_if_t<
            !std::is_constructible<std::tuple<Ts...>,Ts...>::value &&
            std::is_default_constructible<std::tuple<Ts...>>::value
        >
    >
{
    //TODO: arrays with multiple extents
    template<typename T, typename U>
    constexpr static void do_assign(T& t, U&& u)
    {
        t = u;
    }

    template<typename T, typename U, std::size_t N>
    constexpr static void do_assign(T(&t)[N], U(&u)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
            do_assign(t[i],u[i]);
    }

    template<typename T, typename U, std::size_t N>
    constexpr static void do_assign(T(&t)[N], U(&&u)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
            do_assign(t[i],std::move(u[i]));
    }

    template<std::size_t I, typename T, std::size_t N,
             std::enable_if_t<
                !std::is_assignable<decltype(std::get<I>(std::declval<std::tuple<Ts...>&>())),T(&)[N]>::value &&
                std::is_array<std::remove_reference_t<std::tuple_element_t<I,std::tuple<Ts...>>>>::value
             >* = nullptr>
    static void assign(std::tuple<Ts...>& t, T(&arg)[N])
    {
        auto& tmp = std::get<I>(t);
        for (std::size_t i = 0; i < N; ++i)
        {
            tmp[i] = arg[i];
        }
    }

    template<std::size_t I, typename T, std::size_t N,
             std::enable_if_t<
                !std::is_assignable<decltype(std::get<I>(std::declval<std::tuple<Ts...>&>())),T(&&)[N]>::value &&
                std::is_array<std::remove_reference_t<std::tuple_element_t<I,std::tuple<Ts...>>>>::value
             >* = nullptr>
    static void assign(std::tuple<Ts...>& t, T(&&arg)[N])
    {
        auto& tmp = std::get<I>(t);
        for (std::size_t i = 0; i < N; ++i)
        {
            tmp[i] = std::move(arg[i]);
        }
    }

    template<std::size_t I, typename Arg,
             std::enable_if_t<std::is_assignable<decltype(std::get<I>(std::declval<std::tuple<Ts...>&>())),Arg>::value>* = nullptr>
    static void assign(std::tuple<Ts...>& t, Arg&& arg)
    {
        std::get<I>(t) = arg;
    }

    template<typename... Args, std::size_t... Is>
    static void assign(std::tuple<Ts...>& t, std::index_sequence<Is...>, Args&&... args)
    {
        (void)mpirpc::internal::swallow{(assign<Is>(t,std::forward<Args>(args)), 0)...};
    }

    template<typename... Args>
    static void construct(std::tuple<Ts...> *t, Args&&... args)
    {
        new (t) std::tuple<Ts...>();
        assign(*t, std::index_sequence_for<Args...>(), std::forward<Args>(args)...);
    }
};

} // namespace mpirpc

#endif /* MPIRPC__TYPE_CONSTRUCTOR_HPP */
