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

#ifndef MPIRPC__INTERNAL__TYPE_MASSAGING_HPP
#define MPIRPC__INTERNAL__TYPE_MASSAGING_HPP

#include <type_traits>
#include <tuple>
#include <functional>

namespace mpirpc
{

namespace internal
{

/**
 * \internal
 * Used to find the type used by the real function signature without the wrappers
 */
template<typename T>
struct unwrapped;

/**
 * \internal
 * A helper type to query the unwrapped type
 */
template<typename T>
using unwrapped_type = typename unwrapped<T>::type;

/**
 * \internal
 * Used to find the wrapped types for autowrapped function arguments, such as pointers and arrays
 */
template<typename T>
struct autowrapped;

/**
 * \internal
 * A helper type to query the wrapped type
 */
template<typename T>
using autowrapped_type = typename autowrapped<T>::type;

/**
 * \internal
 * Automatically construct wrappers around types which need them, otherwise behave like std::forward
 */
template<typename T>
auto autowrap(T&& t);

/**
 * \intneral
 * Given the choice between the function argument FArg and the potentially wrapped argument Arg, choose the wrapped type
 * while still converting other types of Arg to FArg if necessary
 */
template<typename FArg, typename Arg>
struct choose_wrapped;

/**
 * \internal
 * A helper type for choosing the wrapped type
 * \see choose_wrapped
 */
template<typename FArg, typename Arg>
using choose_wrapped_type = typename choose_wrapped<FArg,Arg>::type;

/**
 * \internal
 * Apply Arg reference to FArg type. Also chooses the wrapped type.
 *
 *      FArg            Arg         Result
 *      double&         float       double&
 *      double          float&&     double
 */
template<typename FArg, typename Arg>
struct choose_reference;

/**
 * \internal
 * A helper type for giving FArg the reference type of Arg. Also chooses the wrapped type.
 */
template<typename FArg, typename Arg>
using choose_reference_type = typename choose_reference<FArg,Arg>::type;

/**
 * Aggressively remove const from a type
 */
template<typename T>
struct remove_all_const;

/**
 * Aggressively remove const from a type
 */
template<typename T>
using remove_all_const_type = typename remove_all_const<T>::type;

/**
 * @brief Choose the correct argument type to pass to the type marshalling phase.
 *
 * The function arguments stored in the unmarshaller in Function<> are not necessarily the same type as
 * passed to the invoke function. Therefore, we need to cast invocation arguments to the types expected
 * by the unmarshaller so that the marshaller builds the correct data packet. However, we can't just
 * std::forward<FArgs>(args)... because we need to make an exception for special wrapper types.
 */

template<typename FT, typename T>
inline constexpr auto forward_parameter(T&& t) noexcept
    -> choose_reference_type<FT, typename std::remove_cv<T>::type>
{
    return static_cast<choose_reference_type<FT, std::remove_cv_t<T>>>(t);
}

/*************************************************************************************/
/*************************************************************************************/
/*                                  Implementation                                   */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*                             mpirpc::internal::unwrapped                           */
/*************************************************************************************/

template<typename T>
struct unwrapped
{
    using type = T;
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct unwrapped<::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using type = T*;
};

template<typename T, std::size_t N>
struct unwrapped<T[N]>
{
    using type = T*;
};

/*************************************************************************************/
/*                            mpirpc::internal::autowrapped                          */
/*************************************************************************************/

template<typename T>
struct autowrapped
{
    using type = T;
};

template<typename T, std::size_t N>
struct autowrapped<T[N]>
{
    using type = ::mpirpc::pointer_wrapper<T,N,false,std::is_const<T>::value,std::allocator<T>>;
};

template<typename T>
struct autowrapped<T*>
{
    using type = std::conditional_t<
                    std::is_same<std::remove_cv_t<T>,char>::value,
                    T*,
                    ::mpirpc::pointer_wrapper<T,0,false,!std::is_const<T>::value,std::allocator<T>>
                 >;
};

template<typename T>
using autowrapped_type = typename autowrapped<T>::type;

/*************************************************************************************/
/*                            mpirpc::internal::autowrap                             */
/*************************************************************************************/

template<typename T,
         std::enable_if_t<std::is_array<std::remove_reference_t<T>>::value>* = nullptr>
auto autowrap(T& t) -> decltype(auto)
{
    using U = std::remove_extent_t<T>;
    constexpr std::size_t extent = std::extent<T>::value;
    return ::mpirpc::pointer_wrapper<U,extent,false,!std::is_const<T>::value,std::allocator<U>>(static_cast<std::decay_t<T>>(t));
}

template<typename T,
         std::enable_if_t<!std::is_array<T>::value>* = nullptr>
auto autowrap(std::remove_reference_t<T>& t) -> decltype(auto)
{
    return static_cast<T&&>(t);
}

template<typename T,
         std::enable_if_t<!std::is_array<T>::value>* = nullptr>
auto autowrap(std::remove_reference<T>&& t) -> decltype(auto)
{
    return static_cast<T&&>(t);
}


/*************************************************************************************/
/*                          mpirpc::internal::choose_wrapped                         */
/*************************************************************************************/

template<typename FArg, typename Arg>
struct choose_wrapped
{
    using type = FArg;
};

template<typename FArg, typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct choose_wrapped<FArg, ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using type = ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>;
};

/*************************************************************************************/
/*                         mpirpc::internal::choose_reference                        */
/*************************************************************************************/

template<typename FArg, typename Arg>
struct choose_reference
{
    using base_type = std::remove_reference_t<choose_wrapped_type<FArg,Arg>>;
    using type = typename std::conditional<std::is_lvalue_reference<Arg>::value,
                        base_type&,
                        typename std::conditional<std::is_rvalue_reference<Arg>::value,
                            base_type&&,
                            typename std::conditional<std::is_rvalue_reference<Arg>::value,
                                base_type&&,
                                base_type
                            >::type
                        >::type
                 >::type;
};

template<typename FArg, typename T, std::size_t N>
struct choose_reference<FArg, T(&)[N]>
{
    using type = ::mpirpc::pointer_wrapper<T,N,false,!std::is_const<T>::value,std::allocator<T>>;
};

template<typename FArg, typename T, std::size_t N>
struct choose_reference<FArg, T(*)[N]>
{
    using type = ::mpirpc::pointer_wrapper<T,N,false,!std::is_const<T>::value,std::allocator<T>>;
};

/*************************************************************************************/
/*                         mpirpc::internal::remove_all_const                        */
/*************************************************************************************/

template<typename T>
struct remove_all_const : std::remove_const<T> {};

template<typename T>
struct remove_all_const<T*>
{
    using type = typename remove_all_const<T>::type*;
};

template<typename T>
struct remove_all_const<T * const>
{
    using type =  typename remove_all_const<T>::type*;
};

template<typename T>
struct remove_all_const<T&>
{
    using type = typename remove_all_const<T>::type&;
};

template<typename T>
struct remove_all_const<const T&>
{
    using type =  typename remove_all_const<T>::type&;
};

}

}

#endif /* MPIRPC__INTERNAL__TYPE_MASSAGING_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
