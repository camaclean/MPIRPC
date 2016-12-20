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

#ifndef MPIRPC__INTERNAL__DETAIL__UTILITY_HPP
#define MPIRPC__INTERNAL__DETAIL__UTILITY_HPP

#include <utility>
#include <tuple>
#include "../alignment.hpp"
#include "../direct_initializer.hpp"
#include "../remarshaller.hpp"

namespace mpirpc
{

namespace internal
{

namespace detail
{

template<typename Alignment, typename Allocator, typename T, typename Buffer, std::enable_if_t<is_buildtype<T,Buffer>>* = nullptr>
void get_from_buffer(Allocator &a, T*& t, Buffer&& s)
{
    direct_initializer<T,Alignment>::placementnew_construct(a,t,s);
}

template<typename Alignment, typename Allocator, typename T, typename Buffer, std::enable_if_t<!is_buildtype<T,Buffer>>* = nullptr>
void get_from_buffer(Allocator &a, T*& t, Buffer&& s)
{
    get_pointer_from_buffer<Alignment::value>(s,t);
}

template<typename Buffer, typename Alignment, typename Allocator, typename T, std::enable_if_t<is_buildtype<T,Buffer>>* = nullptr>
void cleanup(Allocator &a, T* t)
{
    direct_initializer<T,Alignment>::placementnew_destruct(a,t);
}

template<typename Buffer, typename Alignment, typename Allocator, typename T, std::enable_if_t<!is_buildtype<T,Buffer>>* = nullptr>
void cleanup(Allocator &a, T* t)
{}

template <typename F, typename Tuple, std::size_t... I>
decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    return std::forward<F>(f)(static_cast<std::tuple_element_t<I,typename ::mpirpc::internal::function_parts<std::remove_reference_t<F>>::args_tuple_type>>(std::get<I>(std::forward<Tuple>(t)))...);
}

template <typename F, class Class, typename Tuple, size_t... I>
decltype(auto) apply_impl(F&& f, Class *c, Tuple&& t, std::index_sequence<I...>)
{
    return ((*c).*(std::forward<F>(f)))(static_cast<std::tuple_element_t<I,typename ::mpirpc::internal::function_parts<std::remove_reference_t<F>>::args_tuple_type>>(std::get<I>(std::forward<Tuple>(t)))...);
}

template<typename F, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t... Is, typename... Alignments,
         std::enable_if_t<std::is_same<mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments...>>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    if (get_return)
        (void)swallow{((PBs) ? (remarshaller<std::remove_reference_t<mpirpc::internal::autowrapped_type<FArgs>>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(cleanup<InBuffer,Alignments>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<typename F, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, typename... Alignments,
         std::enable_if_t<!std::is_same<mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    using PointerTuple = std::tuple<std::add_pointer_t<Ts>...>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    PointerTuple t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments...>>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    auto&& ret = std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    if (get_return)
    {
        using R = mpirpc::internal::function_return_type<F>;
        remarshaller<R,OutBuffer,std::integral_constant<std::size_t,alignof(R)>>::marshal(os,mpirpc::internal::autowrap<mpirpc::internal::function_return_type<F>,decltype(ret)>(std::move(ret)));
        (void)swallow{((PBs) ? (remarshaller<std::remove_reference_t<mpirpc::internal::autowrapped_type<FArgs>>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    }
    (void)swallow{(cleanup<InBuffer,Alignments>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, typename... Alignments,
         std::enable_if_t<std::is_same<mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments...>>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    ((*c).*(std::forward<F>(f)))(static_cast<FArgs>(*std::get<Is>(t))...);
    if (get_return)
        (void)swallow{((PBs) ? (remarshaller<std::remove_reference_t<mpirpc::internal::autowrapped_type<FArgs>>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(cleanup<InBuffer,Alignments>(a,std::get<Is>(t)), 0)...};
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, typename... Alignments,
         std::enable_if_t<!std::is_same<mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments>>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    auto&& ret = ((*c).*(std::forward<F>(f)))(static_cast<FArgs>(*std::get<Is>(t))...);
    if (get_return)
    {
        using R = mpirpc::internal::function_return_type<F>;
        remarshaller<R,OutBuffer,std::integral_constant<std::size_t,alignof(R)>>::marshal(os,mpirpc::internal::autowrap<mpirpc::internal::function_return_type<F>,decltype(ret)>(std::move(ret)));
        (void)swallow{((PBs) ? (remarshaller<std::remove_reference_t<mpirpc::internal::autowrapped_type<FArgs>>,OutBuffer,Alignments>::marshal(os, mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    }
    (void)swallow{(cleanup<InBuffer,Alignments>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<std::size_t Pos, typename Int, Int Max, Int...Is>
struct get_integer_sequence_clamped_impl;

template<std::size_t Pos, typename Int, Int Max, Int I, Int... Is>
struct get_integer_sequence_clamped_impl<Pos,Int,Max,I,Is...>
{
    constexpr static Int value = get_integer_sequence_clamped_impl<Pos-1,Int,Max,Is...>::value;
};

template<typename Int, Int I, Int Max, Int...Is>
struct get_integer_sequence_clamped_impl<0,Int,Max,I,Is...>
{
    constexpr static Int value = I;
};

template<std::size_t Pos, typename Int, Int Max>
struct get_integer_sequence_clamped_impl<Pos,Int,Max>
{
    constexpr static Int value = Max;
};

template<std::size_t Pos, typename Int, Int... Is>
struct get_integer_sequence_impl;

template<std::size_t Pos, typename Int, Int I, Int... Is>
struct get_integer_sequence_impl<Pos,Int,I,Is...>
{
    constexpr static Int value = get_integer_sequence_impl<Pos-1,Int,Is...>::value;
};

template<typename Int, Int I, Int... Is>
struct get_integer_sequence_impl<0,Int,I,Is...>
{
    constexpr static Int value = I;
};

}

}

}

#endif /* MPIRPC__INTERNAL__DETAIL__UTILITY_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
