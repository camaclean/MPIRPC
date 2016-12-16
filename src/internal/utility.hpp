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

#ifndef MPIRPC__INTERNAL__UTILITY_HPP
#define MPIRPC__INTERNAL__UTILITY_HPP

#include "detail/utility.hpp"

#include <utility>

namespace mpirpc
{

namespace internal
{

/**
 * Passer can be used along with uniform initialization to unpack parameter packs
 * and execute the parameters in the order in which they appear. This is necessary
 * for correctness when side effects are important.
 */
struct passer {
    passer(...) {}
};

template <typename F, typename Tuple>
decltype(auto) apply(F&& f, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return detail::apply_impl(std::forward<F>(f), std::forward<Tuple>(t), Indices{});
}

template <typename F, class Class, typename Tuple>
decltype(auto) apply(F&& f, Class *c, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return detail::apply_impl(std::forward<F>(f), c, std::forward<Tuple>(t), Indices{});
}

/**
 * @details
 * Mechanism:
 *
 * Applying a datagram as arguments with any type to a function is a non-trivial task. The naive approach would be to unpack
 * the stream into a tuple, then apply those tuple arguments to a function. However, this approach has some notable limitations
 * and inefficiencies. First of all, it is not possible to construct a std::tuple with a mix of any type. Take, for instance, a
 * std::tuple containing an array and an mpirpc::pointer_wrapper. An array type is not MoveConstructible or CopyConstructible.
 * The array could be initialized and then the values from the stream applied to it, but mpirpc::pointer_wrapper is not
 * DefaultConstructible. An array type also can't be returned by an unmarshalling function and references to an array can't be
 * used, ei,ther, unless it was initialized outside of the unmarshaller. Otherwise, the result would be a dangling reference. An
 * array type could be created from a std::initializer_list instead, but this would not solve the general case of types with
 * deleted default, copy, and move constructors. For these types, direct initialization is needed. Unfortunately, std::tuple
 * lacks direct initialization capabilities.
 *
 * A previous implementation attempt used two tuples, one created with default constructors and modified using the stream after
 * initialization and a second tuple for move-constructible types suitable for non-DefaultConstructible types. However, this
 * involved some complicated template metaprogramming to determine the parameter index, which tuple to use, and the index in the
 * tuple for each parameter index. It also still had the limitation that a type must be either DefaultConstructible or be
 * MoveConstructible. Types requiring direct initialization would still be unsupported.
 *
 * This implementation gets around this problem by creating a tuple of pointers to each parameter type. How this tuple is
 * initialized depends on the type. If is_buildtype\<T> is true for that type, then the std::tuple element will be initialized
 * to a pointer within a stack buffer which is properly aligned for an element of that type. If is_buildtype\<T> is false, then
 * the pointer points to the location of the data in the stream buffer. By default, is_buildtype\<T> is true if T is a scalar
 * type or an array of scalar types. Otherwise, it is false. Users may specialize is_buildtype\<T> to a boolean true value for
 * custom types that can be correctly accessed by a reinterpret_cast\<T*> on the buffer location (objects with standard layout
 * and no pointer/reference type member variables).
 *
 * Next, types for which is_buildtype\<T> is true call direct_initializer\<T>::placementnew_construct(Allocator,T*,Stream).
 * This constructs a T using placement new, which is suitable for constructing types in the stack buffer. Types which have pointers
 * of their own should use direct_initializer\<T>::construct(Allocator,T*,Buffer), which uses the (possibly custom) Allocator for
 * construction. If T is polymorphic, register_polymorphism\<T>() must be called before invocation of apply(). The arguments for
 * construction of T are provided by unmarshaller\<T,Buffer,Alignment>::unmarshal(Allocator,Buffer).
 *
 * Next, the function is run by unpacking each element of the tuple as each parameter. If the function has a non-void return
 * type, it is autowrapped and added to the output parameter buffer using remarshaller\< T, Buffer, Alignment >::marshal(Buffer,T).
 *
 * Then, passback types from the parameter tuple are appended to the parameter buffer using
 * remarshaller\<T,Buffer,Alignment>::marshal(Buffer,T).
 *
 * Finally, for each element of the parameter tuple for which the value of is_buildtype\< T > is true the tuple element is passed
 * to a cleanup function to run the destructor on the constructed types.
 *
 * @code{.unparsed}
 * Call graph:
 *                                                    apply()
 *                                                      |
 *                                                      v
 *                                                 apply_impl()
 *                                                      |
 *                                                      v
 *                                              get_from_buffer<Alignment>()
 *                                                      |
 *                                                      v
 *                                            is_buildtype<T,Buffer>?
 *                                            No /              \ Yes
 *                          get_pointer_from_buffer()          direct_initializer<T>::placementnew_construct()
 *                                              |                |
 *                                              v                v
 *                  Buffer::reinterpret_and_advance<T>()     unmarshaller<T,Buffer,Alignment,typename=void>::unmarshal()
 *                                              |                |
 *                                              |                v
 *                                              |             get<T>(Buffer,Allocator)
 *                                              |                |
 *                                              |                v
 *                                              |        piecewise_allocator_traits<Allocator>::construct()
 *                                               \_____________/
 *                                                      |
 *                                                      v
 *                                                    call f()
 *                                                      |
 *                                                      v
 *                                               (result == void)?
 *                                             No /            \ Yes
 *             remarshaller<T,Buffer,Alignment>::marshal()     |
 *                                                |            |
 *                                                v            |
 *               marshaller<T,Buffer,Alignment>::marshal()     |
 *                                                \____________/
 *                                                      |
 *                                                      v
 *                                                  cleanup()
 * @endcode
 *
 * The implementation of remarshaller\<T,Buffer,Alignment>::marshal() simply calls marshaller\<T,Buffer,Alignment>::marshal().
 * However, this struct can be specialized if different behavior is required when passing back (such as noting and skipping
 * unmodified parameters).
 *
 * is_buildtype\<T,Buffer> is used to determine if the type stored in the Buffer can be read simply by a reinterpret_cast\<T> at
 * the current location of the buffer. Therefore, is_buildtype\<T> should always be true for Buffer implementations that use
 * non-binary data or packed data structures.
 *
 * unmarshaller\<T,Buffer,Alignment> specializations: call parameterbuffer_unmarshaller\<T,Alignment> for non-pointer scalar types
 *                                                   unmarshaller\<pointer_wrapper\<T>,Buffer,Alignment,void>
 *                                                   user-defined unmarshallers
 *
 *
 *        unmarshaller\<pointer_wrapper\<T>,Buffer,Alignment,typename=void>
 *                                      |
 *                                      v
 *                            std::is_polymorphic\<T>?
 *                          No /                   \ Yes
 *   direct_initializer\<T>::construct              polymorphic_factory\<T>->build()
 *                      U = T |                     | U = from id -> polymorphic_factory map
 *                            v                     v
 *                  unmarshaller\<U,Buffer,Alignment>::unmarshal
 *
 * To add support for custom types, unmarshaller\<T,Buffer,Alignment> should be specialized. To avoid template ambiguity, only the first
 * template parameter should be specialized. If the unmarshaller is specific to a Buffer type or alignment, SFINAE should be used for the
 * last template parameter. unmarshaller\<T,Buffer,Alignment> should return either a single type to be passed as a single argument to the
 * constructor or a tuple of types which will be unpacked as constructor arguments.
 *
 * Example:
 *
 * \code{.cpp}
 * class A {
 * public:
 *     A(int v) : a(v) {}
 *     virtual void test() = 0;
 *     virtual ~A() {}
 *     int a;
 * };
 *
 * class B : public A {
 * public:
 *     B(int v, int v2) : A(v), b(v2) {}
 *     virtual void test() {}
 *     virtual ~B() {}
 *     int b;
 * };
 *
 * template<typename Buffer, std::size_t Alignment>
 * struct marshaller<B,Buffer,Alignment> {
 *     static void marshal(Buffer& b, const B& val) { b.put(val.a); b.put(val.b); }
 * };
 *
 * template<typename Buffer, std::size_t Alignment>
 * struct unmarshaller<B,Buffer,Alignment> {
 *     template<typename Allocator>
 *     static decltype(auto) unmarshal(Allocator& alloc, Buffer& buff) {
 *         int a = get<int>(buff,alloc), b = get<int>(buff,alloc);
 *         return std::make_tuple(std::piecewise_construct,std::move(a),std::move(b));
 *     }
 * };
 * \endcode
 *
 * Since B is a polymorphic type, register_polymorphism<B>() should be called before invocation of
 * apply(), ideally at the beginning of the program. A does not need to be registered or have a
 * specialization of marshaller or unmarshaller, as it is not a constructible type.
 *
 */
template<typename F,typename Allocator, typename InBuffer, typename OutBuffer, typename... Alignments,
         std::enable_if_t<std::is_function<std::remove_pointer_t<std::remove_reference_t<F>>>::value && !std::is_member_function_pointer<std::remove_reference_t<F>>::value>* = nullptr>
void apply(F&& f, Allocator&& a, InBuffer&& s, OutBuffer&& os, bool get_return = false, std::tuple<Alignments...> = std::tuple<Alignments...>{})
{
    using Func = std::remove_reference_t<F>;
    using fargs = typename mpirpc::internal::function_parts<Func>::arg_types;
    using ts = typename mpirpc::internal::wrapped_function_parts<Func>::storage_types;
    using pass_backs = typename mpirpc::internal::wrapped_function_parts<Func>::pass_backs;
    using default_alignments = custom_alignments<typename mpirpc::internal::function_parts<Func>::default_alignments,std::tuple<Alignments...>>;
    constexpr std::size_t num_args = mpirpc::internal::function_parts<Func>::num_args;
    detail::apply_impl(std::forward<F>(f),std::forward<Allocator>(a),std::forward<InBuffer>(s), std::forward<OutBuffer>(os), get_return, fargs{}, ts{}, pass_backs{}, std::make_index_sequence<num_args>{}, default_alignments{});
}

template<typename F,typename Allocator, typename InBuffer, typename OutBuffer, typename... Alignments,
         std::enable_if_t<std::is_function<std::remove_pointer_t<std::remove_reference_t<F>>>::value && !std::is_member_function_pointer<std::remove_reference_t<F>>::value>* = nullptr>
void apply(std::function<F>& f, Allocator&& a, InBuffer&& s, OutBuffer&& os, bool get_return = false, std::tuple<Alignments...> = std::tuple<Alignments...>{})
{
    using Func = std::remove_reference_t<F>;
    using fargs = typename mpirpc::internal::function_parts<std::add_pointer_t<Func>>::arg_types;
    using ts = typename mpirpc::internal::wrapped_function_parts<std::add_pointer_t<Func>>::storage_types;
    using pass_backs = typename mpirpc::internal::wrapped_function_parts<std::add_pointer_t<Func>>::pass_backs;
    using default_alignments = custom_alignments<typename mpirpc::internal::function_parts<std::add_pointer_t<Func>>::default_alignments,std::tuple<Alignments...>>;
    constexpr std::size_t num_args = mpirpc::internal::function_parts<std::add_pointer_t<Func>>::num_args;
    detail::apply_impl(f,std::forward<Allocator>(a),std::forward<InBuffer>(s), std::forward<OutBuffer>(os), get_return, fargs{}, ts{}, pass_backs{}, std::make_index_sequence<num_args>{}, default_alignments{});
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer, typename... Alignments,
         std::enable_if_t<std::is_member_function_pointer<std::remove_reference_t<F>>::value>* = nullptr>
void apply(F&& f, Class *c, Allocator&& a, InBuffer&& s, OutBuffer&& os, bool get_return = false, std::tuple<Alignments...> = std::tuple<Alignments...>{})
{
    using Func = std::remove_reference_t<F>;
    using FArgs = typename mpirpc::internal::function_parts<Func>::arg_types;
    using Ts = typename mpirpc::internal::wrapped_function_parts<Func>::storage_types;
    using PassBacks = typename mpirpc::internal::wrapped_function_parts<Func>::pass_backs;
    using default_alignments = custom_alignments<typename mpirpc::internal::function_parts<Func>::default_alignments,std::tuple<Alignments...>>;
    constexpr std::size_t num_args = mpirpc::internal::function_parts<Func>::num_args;
    detail::apply_impl(std::forward<F>(f),c,std::forward<Allocator>(a),std::forward<InBuffer>(s), std::forward<OutBuffer>(os), get_return, FArgs{}, Ts{}, PassBacks{}, std::make_index_sequence<num_args>{}, default_alignments{});
}

template<std::size_t Pos, typename Int, Int Max, Int... Is>
constexpr Int get_clamped(std::integer_sequence<Int,Is...>)
{
    return ::mpirpc::internal::detail::get_integer_sequence_clamped_impl<Pos,Int,Max,Is...>::value;
}

template<std::size_t Pos, std::size_t Max, std::size_t... Is>
constexpr std::size_t get_clamped(std::integer_sequence<std::size_t, Is...>)
{
    return get_clamped<Pos,std::size_t,Max>(std::integer_sequence<std::size_t,Is...>());
}

template<std::size_t Pos, typename Int, Int...Is>
constexpr Int get(std::integer_sequence<Int,Is...>)
{
    return ::mpirpc::internal::detail::get_integer_sequence_impl<Pos,Int,Is...>::value;
}

template<typename Is1, typename Is2>
struct integer_sequence_cat;

template<typename Int, Int... I1s, Int... I2s>
struct integer_sequence_cat<std::integer_sequence<Int, I1s...>, std::integer_sequence<Int, I2s...>>
{
    using type = std::integer_sequence<Int,I1s...,I2s...>;
};

template <typename Is1, typename Is2>
using integer_sequence_cat_type = typename integer_sequence_cat<Is1,Is2>::type;

} //internal

} //mpirpc

#endif /* MPIRPC__INTERNAL__UTILITY_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
