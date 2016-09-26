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

#ifndef COMMON_HPP
#define COMMON_HPP

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

/**
 * Passer can be used along with uniform initialization to unpack parameter packs
 * and execute the parameters in the order in which they appear. This is necessary
 * for correctness when side effects are important.
 */
struct Passer {
    Passer(...) {}
};

template<typename F>
struct FunctionParts;

template<typename R, class Class, typename... Args>
struct FunctionParts<R(Class::*)(Args...)>
{
    using return_type = R;
    using class_type = Class;
    using args_tuple_type = std::tuple<Args...>;
    using function_type = R(Class::*)(Args...);
};

template<typename R, typename... Args>
struct FunctionParts<R(*)(Args...)>
{
    using return_type = R;
    using args_tuple_type = std::tuple<Args...>;
    using function_type = R(*)(Args...);
};

template<typename R, typename... Args>
struct FunctionParts<std::function<R(Args...)>>
{
    using return_type = R;
    using args_tuple_type = std::tuple<Args...>;
    using function_type = std::function<R(Args...)>;
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
class PointerWrapper;

template<typename T>
struct storage_type
{
    using type = T;
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct storage_type<PointerWrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using type = typename std::conditional<std::is_array<T>::value,T,T*>::type;
};

template<typename F>
struct storage_function_parts;

template<typename R, class Class, typename... Args>
struct storage_function_parts<R(Class::*)(Args...)>
{
    using return_type = R;
    using class_type = Class;
    using wrapped_function_type = R(Class::*)(Args...);
    using storage_tuple_type = std::tuple<typename std::remove_reference<Args>::type...>;
    using function_type = R(Class::*)(typename storage_type<Args>::type...);
};

template<typename R, typename... Args>
struct storage_function_parts<R(*)(Args...)>
{
    using return_type = R;
    using wrapped_function_type = R(*)(Args...);
    using storage_tuple_type = std::tuple<typename std::remove_reference<Args>::type...>;
    using function_type = R(*)(typename storage_type<Args>::type...);
};

template<typename R, typename... Args>
struct storage_function_parts<std::function<R(Args...)>>
{
    using return_type = R;
    using wrapped_function_type = std::function<R(Args...)>;
    using storage_tuple_type = std::tuple<typename std::remove_reference<Args>::type...>;
    using function_type = std::function<R(typename storage_type<Args>::type...)>;
};

template<typename Functor, typename storage_function_parts<Functor>::function_type f>
struct function_identifier {};

using FnHandle = unsigned long long;
using TypeId = unsigned long long;
using ObjectId = unsigned long long;

template<typename T> struct remove_all_const : std::remove_const<T> {};

template<typename T> struct remove_all_const<T*> {
    typedef typename remove_all_const<T>::type *type;
};

template<typename T> struct remove_all_const<T * const> {
    typedef typename remove_all_const<T>::type *type;
};

template<typename T> struct remove_all_const<T&> {
    typedef typename remove_all_const<T>::type &type;
};

template<typename T> struct remove_all_const<const T&> {
    typedef typename remove_all_const<T>::type &type;
};

template<bool... Bs>
struct any_true;

template<bool B1, bool... Bs>
struct any_true<B1, Bs...>
{
    constexpr static bool value = B1 || any_true<Bs...>::value;
};

template<bool B>
struct any_true<B>
{
    constexpr static bool value = B;
};

template<bool...Vs>
struct bool_tuple{};

template<typename T>
struct is_pass_back
{
    constexpr static bool value = std::is_lvalue_reference<T>::value &&
                                  !std::is_const<typename std::remove_reference<T>::type>::value &&
                                  !std::is_pointer<typename std::remove_reference<T>::type>::value &&
                                  !std::is_array<typename std::remove_reference<T>::type>::value;
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct is_pass_back<PointerWrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    constexpr static bool value = PassBack;
};

template<typename T>
struct pass_back_false
{
    constexpr static bool value = false;
};

/*template<typename T>
constexpr std::size_t array_elements(const T&) noexcept
{
    return sizeof(T)/sizeof(typename std::remove_all_extents<T>::type);
}*/

}

#endif // COMMON_HPP

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
