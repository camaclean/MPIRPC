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

#ifndef MPIRPC__INTERNAL__DETAIL__APPLY_BUFFER_HPP
#define MPIRPC__INTERNAL__DETAIL__APPLY_BUFFER_HPP

#include <utility>
#include <tuple>
#include "../alignment.hpp"
#include "../direct_initializer.hpp"
#include "../remarshaller.hpp"
#include "../utility.hpp"
#include "../function_attributes.hpp"

namespace mpirpc
{

namespace internal
{

namespace detail
{

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Pos, typename Types, typename Alignments>
struct alignment_padding_helper;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Pos, typename...Ts, typename... Alignments>
struct alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos,std::tuple<Ts...>,std::tuple<Alignments...>>
{
    using type = std::tuple_element_t<Pos,std::tuple<Ts...>>;
    using type_alignment = std::tuple_element_t<Pos,std::tuple<Alignments...>>;
    static constexpr std::size_t type_size = sizeof(type);
    static constexpr bool predicate = (mpirpc::is_buildtype<type,Buffer>() && !SkipBuildTypes) || (!mpirpc::is_buildtype<type,Buffer>() && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,std::tuple<Ts...>,std::tuple<Alignments...>>::end_address_offset;
    static constexpr std::size_t delta = (predicate) ? mpirpc::internal::calculate_alignment_padding(prev_end_address_offset,type_alignment::value) : 0;
    static constexpr std::size_t start_address_offset = (predicate) ? prev_end_address_offset + delta : prev_end_address_offset;
    static constexpr std::size_t end_address_offset = (predicate) ? start_address_offset + type_size : prev_end_address_offset;
    static constexpr std::size_t total_padding = alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,std::tuple<Ts...>,std::tuple<Alignments...>>::total_padding + delta;
    static constexpr std::size_t total_size = alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,Pos-1,std::tuple<Ts...>,std::tuple<Alignments...>>::total_size + delta + type_size;
};

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename T, typename... Ts, typename Alignment, typename... Alignments>
struct alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,0,std::tuple<T,Ts...>,std::tuple<Alignment,Alignments...>>
{
    using type = T;
    using type_alignment = Alignment;
    static constexpr std::size_t type_size = sizeof(type);
    static constexpr bool predicate = (is_buildtype<type,Buffer>() && !SkipBuildTypes) || (!is_buildtype<type,Buffer>() && !SkipNonBuildTypes);
    static constexpr std::size_t prev_end_address_offset = 0;
    static constexpr std::size_t delta = 0;
    static constexpr std::size_t start_address_offset = 0;
    static constexpr std::size_t end_address_offset = (predicate) ? unpacked_size_v<Buffer,type,type_alignment> : 0;
    static constexpr std::size_t total_padding = 0;
    static constexpr std::size_t total_size = type_size;
};

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes>
struct alignment_padding_helper<Buffer,SkipBuildTypes,SkipNonBuildTypes,0,std::tuple<>,std::tuple<>>
{
    using type = std::nullptr_t;
    static constexpr bool predicate = false;
    static constexpr std::size_t prev_end_address_offset = 0;
    static constexpr std::size_t delta = 0;
    static constexpr std::size_t start_address_offset = 0;
    static constexpr std::size_t end_address_offset = 0;
    static constexpr std::size_t total_padding = 0;
    static constexpr std::size_t total_size = 0;
};

template<typename T1, typename T2,typename = void>
struct choose_custom_alignment;

template<typename...T1s,typename...T2s>
struct choose_custom_alignment<std::tuple<T1s...>,std::tuple<T2s...>, std::enable_if_t<!!sizeof...(T2s)>>
{
    using type = std::tuple<T2s...>;
};

template<typename...T1s, typename... T2s>
struct choose_custom_alignment<std::tuple<T1s...>,std::tuple<T2s...>, std::enable_if_t<!sizeof...(T2s)>>
{
    using type = std::tuple<T1s...>;
};

}

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename Types, typename Alignments>
constexpr std::size_t alignment_padding = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, (std::tuple_size<Types>::value > 0) ? (std::tuple_size<Types>::value-1) : 0, Types, Alignments>::total_padding;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, typename Types, typename Alignments>
constexpr std::size_t aligned_buffer_size = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, (std::tuple_size<Types>::value > 0) ? (std::tuple_size<Types>::value-1) : 0, Types, Alignments>::total_size;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Index, typename Types, typename Alignments>
constexpr std::size_t aligned_buffer_address_offset = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, Index, Types, Alignments>::start_address_offset;

template<typename Buffer, bool SkipBuildTypes, bool SkipNonBuildTypes, std::size_t Index, typename Types, typename Alignments>
constexpr std::size_t aligned_buffer_delta = detail::alignment_padding_helper<Buffer, SkipBuildTypes, SkipNonBuildTypes, Index, Types, Alignments>::delta;

template<typename T1, typename T2>
using custom_alignments = typename detail::choose_custom_alignment<T1,T2>::type;

namespace detail
{

template<typename F, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t... Is, typename... Alignments,
         std::enable_if_t<std::is_same<::mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, ::mpirpc::internal::type_pack<FArgs...>, ::mpirpc::internal::type_pack<Ts...>, ::mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = ::mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype_v<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + ::mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments...>>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(::mpirpc::internal::detail::get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    if (get_return)
        (void)swallow{((PBs) ? (remarshaller<std::remove_reference_t<::mpirpc::internal::autowrapped_type<FArgs>>,OutBuffer,Alignments>::marshal(os, ::mpirpc::internal::autowrap<::mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(::mpirpc::internal::detail::cleanup<InBuffer,Alignments>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<typename F, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, typename... Alignments,
         std::enable_if_t<!std::is_same<::mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, ::mpirpc::internal::type_pack<FArgs...>, ::mpirpc::internal::type_pack<Ts...>, ::mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = ::mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    using PointerTuple = std::tuple<std::add_pointer_t<Ts>...>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    PointerTuple t{((is_buildtype_v<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + ::mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments...>>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(::mpirpc::internal::detail::get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    auto&& ret = std::forward<F>(f)(static_cast<FArgs>(*std::get<Is>(t))...);
    if (get_return)
    {
        using R = ::mpirpc::internal::function_return_type<F>;
        remarshaller<R,OutBuffer,std::integral_constant<std::size_t,alignof(R)>>::marshal(os,::mpirpc::internal::autowrap<mpirpc::internal::function_return_type<F>,decltype(ret)>(std::move(ret)));
        (void)swallow{((PBs) ? (remarshaller<std::remove_reference_t<::mpirpc::internal::autowrapped_type<FArgs>>,OutBuffer,Alignments>::marshal(os, ::mpirpc::internal::autowrap<::mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    }
    (void)swallow{(cleanup<InBuffer,Alignments>(std::forward<Allocator>(a),std::get<Is>(t)), 0)...};
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, typename... Alignments,
         std::enable_if_t<std::is_same<::mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, ::mpirpc::internal::type_pack<FArgs...>, ::mpirpc::internal::type_pack<Ts...>, ::mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype_v<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + ::mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments...>>)) : nullptr)...};
    using swallow = int[];
    (void)swallow{(get_from_buffer<Alignments>(std::forward<Allocator>(a),std::get<Is>(t),s), 0)...};
    ((*c).*(std::forward<F>(f)))(static_cast<FArgs>(*std::get<Is>(t))...);
    if (get_return)
        (void)swallow{((PBs) ? (remarshaller<std::remove_reference_t<mpirpc::internal::autowrapped_type<FArgs>>,OutBuffer,Alignments>::marshal(os, ::mpirpc::internal::autowrap<mpirpc::internal::autowrapped_type<FArgs>,Ts>(*std::get<Is>(t))), 0) : 0)...};
    (void)swallow{(cleanup<InBuffer,Alignments>(a,std::get<Is>(t)), 0)...};
}

template<typename F, class Class, typename Allocator, typename InBuffer, typename OutBuffer, typename...FArgs, typename...Ts, bool... PBs, std::size_t...Is, typename... Alignments,
         std::enable_if_t<!std::is_same<mpirpc::internal::function_return_type<std::remove_reference_t<F>>,void>::value>* = nullptr>
void apply_impl(F&& f, Class *c, Allocator&& a, InBuffer& s, OutBuffer& os, bool get_return, mpirpc::internal::type_pack<FArgs...>, mpirpc::internal::type_pack<Ts...>, mpirpc::internal::bool_template_list<PBs...>, std::index_sequence<Is...>, std::tuple<Alignments...>)
{
    constexpr std::size_t buffer_size = mpirpc::internal::aligned_buffer_size<InBuffer,false,true,std::tuple<Ts...>,std::tuple<Alignments...>>;
    char * buffer = (buffer_size > 0) ? static_cast<char*>(alloca(buffer_size)) : nullptr;
    std::tuple<std::add_pointer_t<Ts>...> t{((is_buildtype_v<Ts,InBuffer>) ? static_cast<std::add_pointer_t<Ts>>(static_cast<void*>(buffer + mpirpc::internal::aligned_buffer_address_offset<InBuffer,false,true,Is,std::tuple<Ts...>,std::tuple<Alignments>>)) : nullptr)...};
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


}

}

}

#endif /* MPIRPC__INTERNAL__DETAIL__APPLY_BUFFER_HPP */
