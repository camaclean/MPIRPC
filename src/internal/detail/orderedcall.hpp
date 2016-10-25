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

#ifndef MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP
#define MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP

#include <tuple>
#include <utility>
#include "../pointerwrapper.hpp"
#include "../unmarshalling.hpp"
#include <memory>

namespace mpirpc
{

namespace internal
{

namespace detail
{

template<typename T>
struct arg_cleanup
{
    template<typename Allocator>
    static void apply(Allocator&& a, typename std::remove_reference<T>::type&)
    {
        //std::cout << "blank clean up for " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
    }

    template<typename Allocator>
    static void apply(Allocator&& a, typename std::remove_reference<T>::type&&)
    {
        //std::cout << "generic rvalue " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
    }
};

template<typename T>
struct arg_cleanup<pointer_wrapper<T>>
{
    template<typename Allocator>
    static void apply(Allocator&& a, pointer_wrapper<T>&& t)
    {
        //std::cout << "cleaned up C Array of " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
        t.free(a);
    }
};

template<typename T, std::size_t N>
struct arg_cleanup<T(&)[N]>
{
    template<typename Allocator>
    static void apply(Allocator&& a, T(&v)[N])
    {
        //std::cout << "cleaning up reference to array" << std::endl;
        using NA = typename std::allocator_traits<std::remove_reference_t<Allocator>>::template rebind_alloc<std::remove_const_t<T>>;
        NA na(a);
        for (std::size_t i = 0; i < N; ++i)
            mpirpc::array_destroy_helper<T>::destroy(a,v[i]);
        std::allocator_traits<NA>::deallocate(na,(std::remove_const_t<T>*) &v,N);
    }
};

template<typename T, std::size_t N>
struct arg_cleanup<T(&&)[N]>
{
    template<typename Allocator>
    static void apply(Allocator&& a, T(&&v)[N])
    {
        std::cout << "cleaning up rvalue reference to array" << std::endl;
        using NA = typename std::allocator_traits<std::remove_reference_t<Allocator>>::template rebind_alloc<std::remove_const_t<T>>;
        NA na(a);
        for (std::size_t i = 0; i < N; ++i)
            mpirpc::array_destroy_helper<T>::destroy(a,v[i]);
        std::allocator_traits<NA>::deallocate(na,(std::remove_const_t<T>*) &v,N);
    }
};

template<>
struct arg_cleanup<char*>
{
    template<typename Allocator>
    static void apply(Allocator&& a, char*&& s) { delete[] s; std::cout << "deleted char*" << std::endl; }
};

template<>
struct arg_cleanup<const char*>
{
    template<typename Allocator>
    static void apply(Allocator&& a, const char*&& s)
    {
        delete[] s;
        std::cout << "deleted const char*" << std::endl;
    }
};

template<typename Allocator, typename... Ts, std::size_t... Is>
void clean_up_args_tuple_impl(Allocator&& a, std::tuple<Ts...>& t, std::index_sequence<Is...>)
{
    (void)(int[]){ (std::cout << abi::__cxa_demangle(typeid(void(*)(Ts)).name(),0,0,0) << std::endl, arg_cleanup<Ts>::apply(a, std::forward<Ts>(std::get<Is>(t))),0)... };
}

template<typename Allocator, typename... Args>
void clean_up_args_tuple(Allocator&& a, std::tuple<Args...> &t)
{
    clean_up_args_tuple_impl(a,t,std::make_index_sequence<sizeof...(Args)>{});
}

template<typename T, std::size_t I>
struct argument_info
{
    using type = T;
    static constexpr bool mct = std::is_move_constructible<T>::value;
    static constexpr bool passback = ::mpirpc::internal::is_pass_back<T>::value;
    static constexpr std::size_t index = I;
};

template<typename...>
struct argument_storage_tuples;

template<typename T, typename...Rest>
struct argument_storage_tuples<T,Rest...>
{
    static constexpr bool condition = std::is_move_constructible<T>::value;
    using mct_tuple = ::mpirpc::internal::tuple_cat_type<std::conditional_t<condition,std::tuple<T>,std::tuple<>>,typename argument_storage_tuples<Rest...>::mct_tuple>;
    using nmct_tuple = ::mpirpc::internal::tuple_cat_type<std::conditional_t<!condition,std::tuple<T>,std::tuple<>>,typename argument_storage_tuples<Rest...>::nmct_tuple>;
};

template<>
struct argument_storage_tuples<>
{
    using mct_tuple = std::tuple<>;
    using nmct_tuple = std::tuple<>;
};

template<std::size_t MCTSize, std::size_t NMCTSize, typename...>
struct argument_storage_info_impl;

template<std::size_t MCTSize, std::size_t NMCTSize, typename T, typename... Rest>
struct argument_storage_info_impl<MCTSize, NMCTSize, T, Rest...>
{
    static constexpr bool condition = std::is_move_constructible<T>::value;
    static constexpr std::size_t param_index = MCTSize + NMCTSize - sizeof...(Rest) - 1;
    static constexpr std::size_t index = std::conditional_t<condition,
                                                           std::integral_constant<std::size_t,MCTSize>,
                                                           std::integral_constant<std::size_t,NMCTSize>
                                                          >::value -
                                         std::tuple_size<std::conditional_t<condition,
                                                                            typename argument_storage_tuples<T,Rest...>::mct_tuple,
                                                                            typename argument_storage_tuples<T,Rest...>::nmct_tuple
                                                                           >>::value;
    using current_info =  argument_info<T,index>;
    using mct_indexes = std::conditional_t<condition,
                                           ::mpirpc::internal::integer_sequence_cat_type<std::index_sequence<param_index>,typename argument_storage_info_impl<MCTSize,NMCTSize,Rest...>::mct_indexes>,
                                           typename argument_storage_info_impl<MCTSize,NMCTSize,Rest...>::mct_indexes
                                          >;
    using nmct_indexes = std::conditional_t<!condition,
                                            ::mpirpc::internal::integer_sequence_cat_type<std::index_sequence<param_index>,typename argument_storage_info_impl<MCTSize,NMCTSize,Rest...>::nmct_indexes>,
                                            typename argument_storage_info_impl<MCTSize,NMCTSize,Rest...>::nmct_indexes
                                           >;
    using split_indexes = ::mpirpc::internal::integer_sequence_cat_type<std::index_sequence<index>,typename argument_storage_info_impl<MCTSize,NMCTSize,Rest...>::split_indexes>;
    using info = ::mpirpc::internal::tuple_cat_type<std::tuple<current_info>,typename argument_storage_info_impl<MCTSize,NMCTSize,Rest...>::info>;
};

template<std::size_t MCTSize, std::size_t NMCTSize>
struct argument_storage_info_impl<MCTSize, NMCTSize>
{
    using info = std::tuple<>;
    using mct_indexes = std::index_sequence<>;
    using nmct_indexes = std::index_sequence<>;
    using split_indexes = std::index_sequence<>;
};

template<typename>
struct argument_storage_info;

template<typename... Ts>
struct argument_storage_info<std::tuple<Ts...>>
{
    using mct_tuple = typename argument_storage_tuples<Ts...>::mct_tuple;
    using nmct_tuple = typename argument_storage_tuples<Ts...>::nmct_tuple;
    
    using info_type =  argument_storage_info_impl<std::tuple_size<mct_tuple>::value,std::tuple_size<nmct_tuple>::value,Ts...>;
    using info = typename info_type::info;
    using mct_indexes = typename info_type::mct_indexes;
    using nmct_indexes = typename info_type::nmct_indexes;
    using split_indexes = typename info_type::split_indexes;
    constexpr static std::size_t size = sizeof...(Ts);
};

template<std::size_t TupleIndex, typename Allocator, typename Stream, typename... Ts>
void unmarshal_nmct_impl2(Allocator &a, Stream &s, std::tuple<Ts...>& t)
{
    //std::cout << "unmarshalling index " << ArgIndex << " with tuple index " << TupleIndex << " and type " << abi::__cxa_demangle(typeid(std::get<TupleIndex>(t)).name(),0,0,0) << std::endl;
    s >> std::get<TupleIndex>(t);
}

template<std::size_t NMCT_Begin, typename Allocator, typename Stream, typename... NMCTs, std::size_t... Is, std::size_t... SplitTupleIs>
void unmarshal_nmct_impl(Allocator &a, Stream &s, std::tuple<NMCTs...> &t, std::index_sequence<Is...>, std::index_sequence<SplitTupleIs...>)
{
    using swallow = int[];
    (void)swallow{(unmarshal_nmct_impl2<::mpirpc::internal::get<NMCT_Begin+Is>(std::index_sequence<SplitTupleIs...>())>(a,s,t), 0)...};
}

template<std::size_t NMCT_Begin, std::size_t NMCT_End, typename Allocator, typename Stream, typename... NMCTs, std::size_t... SplitTupleIs, std::enable_if_t<!(NMCT_Begin >= NMCT_End)>* = nullptr>
void unmarshal_nmct(Allocator &a, Stream &s, std::tuple<NMCTs...> &t, std::index_sequence<SplitTupleIs...>)
{
    unmarshal_nmct_impl<NMCT_Begin>(a,s,t,std::make_index_sequence<NMCT_End-NMCT_Begin>(),std::index_sequence<SplitTupleIs...>());
}

template<std::size_t NMCT_Begin, std::size_t NMCT_End, typename Allocator, typename Stream, typename... NMCTs, std::size_t... SplitTupleIs, std::enable_if_t<(NMCT_Begin >= NMCT_End)>* = nullptr>
void unmarshal_nmct(Allocator &, Stream &, std::tuple<NMCTs...> &, std::index_sequence<SplitTupleIs...>)
{}

template<std::size_t NMCT_Begin, std::size_t NMCT_End, typename T, typename Allocator, typename Stream, typename... NMCTs, std::size_t... SplitTupleIs>
T unmarshal_tuples_impl2(Allocator &a, Stream&s, std::tuple<NMCTs...> &t,std::index_sequence<SplitTupleIs...>)
{
    //std::cout << "unmarshalling index " << ArgIndex << " of type " << abi::__cxa_demangle(typeid(T).name(),0,0,0) << std::endl;
    //std::cout << "unmarshalling nmct range: " << NMCT_Begin << " " << NMCT_End << std::endl;
    T ret = mpirpc::unmarshaller_remote<T>::unmarshal(a,s);
    unmarshal_nmct<NMCT_Begin,NMCT_End>(a,s,t,std::index_sequence<SplitTupleIs...>());
    return ret;
}

template<std::size_t ArgLen, typename Allocator, typename Stream, typename...NMCTs, typename...MCTs, std::size_t... ArgIs, std::size_t... MCT_Is, std::size_t...SplitTupleIs>
std::tuple<MCTs...> unmarshal_tuples_impl(
         Allocator &a,
         Stream &s,
         std::tuple<NMCTs...> &t,
         std::index_sequence<ArgIs...>,
         std::index_sequence<MCT_Is...>,
         std::index_sequence<SplitTupleIs...>,
         argument_types<MCTs...>
     )
{
    return std::tuple<MCTs...>{std::allocator_arg_t{}, a, unmarshal_tuples_impl2<ArgIs+1,::mpirpc::internal::get_clamped<MCT_Is+1,std::size_t,ArgLen>(std::index_sequence<ArgIs...>()),MCTs>(a,s,t,std::index_sequence<SplitTupleIs...>())...};
}



template<typename T>
struct unmarshal_tuples
{
    using mct_tuple_type = typename argument_storage_info<T>::mct_tuple;
    using nmct_tuple_type = typename argument_storage_info<T>::nmct_tuple;
    using mct_indexes = typename argument_storage_info<T>::mct_indexes;
    using split_indexes = typename argument_storage_info<T>::split_indexes;

    template<typename Allocator, typename Stream>
    static mct_tuple_type unmarshal(Allocator &a, Stream &s, nmct_tuple_type& nmct_tuple)
    {

         constexpr std::size_t mct_index_size = mct_indexes::size();
         return unmarshal_tuples_impl<std::tuple_size<T>::value>(a,s,nmct_tuple,mct_indexes(),std::make_index_sequence<mct_index_size>(),split_indexes(),tuple_to_argument_types_type<mct_tuple_type>());
    }
};

template<std::size_t I, bool MCT, typename MTuple, typename NMTuple, std::enable_if_t<MCT>* = nullptr>
decltype(auto) get_tuples(MTuple&& m, NMTuple&&)
{
    return std::get<I>(std::forward<MTuple>(m));
}

template<std::size_t I, bool MCT, typename MTuple, typename NMTuple, std::enable_if_t<!MCT>* = nullptr>
decltype(auto) get_tuples(MTuple&&, NMTuple&& nm)
{
    return std::get<I>(std::forward<NMTuple>(nm));
}

template<typename F, typename MTuple, typename NMTuple, typename... Ts, std::size_t... TIs, std::size_t... Is>
auto apply_impl(F&& f, MTuple&& m, NMTuple&& nm, std::tuple<argument_info<Ts,TIs>...>, std::index_sequence<Is...>)
    -> ::mpirpc::internal::function_return_type<F>
{
    return std::forward<F>(f)(static_cast<std::tuple_element_t<Is,typename ::mpirpc::internal::function_parts<std::remove_reference_t<F>>::args_tuple_type>>(get_tuples<TIs,std::is_move_constructible<Ts>::value>(std::forward<MTuple>(m),std::forward<NMTuple>(nm)))...);
}

template<typename F, typename MTuple, typename NMTuple>
auto apply(F&& f, MTuple&& mct, NMTuple&& nmct)
    -> ::mpirpc::internal::function_return_type<F>
{
    using args_tuple_type = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    using info_type = argument_storage_info<args_tuple_type>;
    //static_assert(std::is_same<std::remove_reference_t<MTuple>, typename info_type::mct_tuple>::value, "Wrong types for MTuple");
    //static_assert(std::is_same<std::remove_reference_t<NMTuple>, typename info_type::nmct_tuple>::value, "Wrong types for NMTuple");
    return apply_impl(std::forward<F>(f), mct, nmct, typename info_type::info{}, std::make_index_sequence<info_type::size>{});
}

template <typename F, class Class, typename MTuple, typename NMTuple, typename... Ts, std::size_t... TIs, std::size_t... Is>
decltype(auto) apply_impl(F&& f, Class *c, MTuple&& m, NMTuple&& nm, std::tuple<argument_info<Ts,TIs>...>, std::index_sequence<Is...>)
{
    return ((*c).*(std::forward<F>(f)))(static_cast<std::tuple_element_t<Is,typename ::mpirpc::internal::function_parts<std::remove_reference_t<F>>::args_tuple_type>>(get_tuples<TIs,std::is_move_constructible<Ts>::value>(std::forward<MTuple>(m),std::forward<NMTuple>(nm)))...);
}

template<typename F, class Class, typename MTuple, typename NMTuple>
auto apply(F&& f, Class *c, MTuple&& mct, NMTuple&& nmct)
    -> ::mpirpc::internal::function_return_type<F>
{
    using args_tuple_type = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type;
    using info_type = argument_storage_info<args_tuple_type>;
    //static_assert(std::is_same<std::remove_reference_t<MTuple>, typename info_type::mct_tuple>::value, "Wrong types for MTuple");
    //static_assert(std::is_same<std::remove_reference_t<NMTuple>, typename info_type::nmct_tuple>::value, "Wrong types for NMTuple");
    return apply_impl(std::forward<F>(f), c, mct, nmct, typename info_type::info{}, std::make_index_sequence<info_type::size>{});
}

template<typename MTuple, typename NMTuple, typename Stream, bool... PBs, typename... Ts, std::size_t... TIs>
void marshal_pass_back_impl(Stream&& out, MTuple&& mct, NMTuple&& nmct, ::mpirpc::internal::bool_template_list<PBs...>, std::tuple<argument_info<Ts,TIs>...>)
{
    using swallow = int[];
    (void)swallow{((PBs) ? out << get_tuples<TIs,std::is_move_constructible<Ts>::value>(mct,nmct), 1 : 0)...};
}

template<typename F, typename Stream, typename StorageTupleType = typename ::mpirpc::internal::wrapped_function_parts<F>::storage_tuple_type>
void marshal_pass_back(Stream&& out, typename argument_storage_info<StorageTupleType>::mct_tuple& mct, typename argument_storage_info<StorageTupleType>::nmct_tuple& nmct)
{
    marshal_pass_back_impl(out, mct, nmct, typename ::mpirpc::internal::wrapped_function_parts<F>::pass_backs{}, typename argument_storage_info<StorageTupleType>::info{});
}

} //detail

} //internal

} //mpirpc

#endif /* MPIRPC__INTERNAL__DETAIL__ORDEREDCALL_HPP */
