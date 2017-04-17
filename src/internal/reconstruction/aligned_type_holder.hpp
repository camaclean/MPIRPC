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

#ifndef MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNED_TYPE_HOLDER_HPP
#define MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNED_TYPE_HOLDER_HPP

#include "../../construction_info.hpp"
#include "detail/stored_arguments_info.hpp"
#include "type_conversion.hpp"
#include "../alignment.hpp"
#include "../utility.hpp"
#include <cxxabi.h>
#include <type_traits>
#include <utility>

namespace mpirpc
{

namespace internal
{

namespace reconstruction
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

// template<typename T, typename U>
// struct deep_assigner
// {
//     constexpr void assign(T& t, U&& u)
//     {
//         t = u;
//     }
// }
//
// template<typename T, std::size_t N, typename U>
// struct deep_assigner<T[N],U>
// {
//     constexpr void assign(T& t, U&& u)
//     {
//     }
// }
//
// template<typename T, typename U>
// void deep_assign(T& t, U&& u)
// {
//     deep_assigner<T,U&&>::assign(t,u);
// }

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







template<typename T, typename Alignment, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
class aligned_type_holder;

template<typename T, typename Alignment, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T,Alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
public:
    using type = T;
    using constructor_argument_types_tuple_type = std::tuple<ConstructorArgumentTypes...>;
    using arguments_tuple_type = std::tuple<ArgumentTypes...>;
    using stored_arguments_tuple_type = std::tuple<StoredArguments...>;
    using alignments_tuple_type = std::tuple<Alignments...>;
    using construction_info_type = construction_info<T,constructor_argument_types_tuple_type,arguments_tuple_type,stored_arguments_tuple_type>;

    using type_storage = typename std::aligned_storage<sizeof(T),Alignment::value>::type;
    using stored = std::integer_sequence<bool, is_stored_v<ArgumentTypes,StoredArguments,Alignments>...>;
    using stored_types = filter_tuple<stored, std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    static constexpr std::size_t stored_count = std::tuple_size<stored_types>::value;
    using aligned_storage_tuple_type = filter_tuple<stored, std::tuple<reconstruction_storage_aligned_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using storage_types = std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>;
    using storage_construction_types = std::tuple<reconstruction_storage_constructor_type<ArgumentTypes,StoredArguments,Alignments>...>;
    using proxy_types = std::tuple<ConstructorArgumentTypes&&...>;
    using stored_indexes = filtered_sequence_indexes_type<stored>;

    template<std::size_t I>
    using argument_at_index = std::tuple_element_t<I,arguments_tuple_type>;

    template<std::size_t I>
    using storage_constructor_type_at_index = std::tuple_element_t<I,storage_construction_types>;

    template<std::size_t I>
    using stored_type = std::tuple_element_t<I,stored_types>;
    
    template<std::size_t I>
    using proxy_type = std::tuple_element_t<I,proxy_types>;

    template<std::size_t I>
    using storage_type_at_storage_index = std::tuple_element_t<I, stored_types>;

    template<std::size_t I>
    using index_to_storage_index = std::tuple_element_t<I,stored_indexes>;
    
    template<std::size_t I>
    static constexpr std::size_t index_to_storage_index_v = index_to_storage_index<I>::value;
    
    template<std::size_t I>
    static constexpr bool is_stored_v = integer_sequence_element_v<I,stored>;

protected:
    template<std::size_t I,
             std::enable_if_t<
                is_stored_v<I> && is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        using arg_type = argument_at_index<I>;
        using con_type = storage_constructor_type_at_index<I>;
        constexpr std::size_t SI = index_to_storage_index_v<I>;
        std::cout << "Constructing using construction_info (saved)" << abi::__cxa_demangle(typeid(con_type).name(),0,0,0) << " " << abi::__cxa_demangle(typeid(std::tuple<proxy_type<I>>).name(),0,0,0) << std::endl;
        
        con_type *storage = reinterpret_cast<con_type*>(&std::get<SI>(stored_args));
        new (storage) con_type{std::get<I>(args)};
        //type_constructor<con_type>::construct(storage, std::get<I>(args));
        return static_cast<proxy_type<I>>(storage->value());
    }

    template<std::size_t I,
             std::enable_if_t<
                is_stored_v<I> && !is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        constexpr std::size_t SI = index_to_storage_index_v<I>;;
        using arg_type = argument_at_index<I>;
        type_constructor<std::remove_cv_t<std::remove_reference_t<arg_type>>>::construct(reinterpret_cast<std::remove_cv_t<std::remove_reference_t<arg_type>>*>(&std::get<SI>(stored_args)), std::get<I>(args));
        std::cout << "Copy constructing: " << abi::__cxa_demangle(typeid(arg_type).name(),0,0,0) << std::endl;
        return reinterpret_cast<proxy_type<I>>(std::get<SI>(stored_args));
    }

    //do not store and is_construction_info true
    template<std::size_t I,
             std::enable_if_t<
                !is_stored_v<I> && is_construction_info_v<argument_at_index<I>>
               >* = nullptr>
    static stored_type<index_to_storage_index<I>::value> construct(aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        using arg_type = stored_type<index_to_storage_index_v<I>>;//std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<I,proxy_tuple_type>>>;
        using con_type = storage_constructor_type_at_index<I>;
        std::cout << "Constructing using construction_info (non-saved): " << abi::__cxa_demangle(typeid(std::tuple<proxy_type<I>>).name(),0,0,0) << " | " << abi::__cxa_demangle(typeid(con_type).name(),0,0,0) << std::endl;
        return con_type::construct(std::get<I>(args));//tuple_construct<arg_type>(std::get<I>(args).args());
    }

    template<std::size_t I,
             std::enable_if_t<
                !is_stored_v<I> && !is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(aligned_storage_tuple_type&, arguments_tuple_type& args)
    {
        return reinterpret_cast<proxy_type<I>>(std::get<I>(args));
    }

    template<std::size_t... Is>
    static void construct(std::remove_cv_t<T> *val, aligned_storage_tuple_type &stored_args, arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        type_constructor<std::remove_cv_t<T>>::construct(reinterpret_cast<std::remove_cv_t<T>*>(val),aligned_type_holder::construct<Is>(stored_args, t)...);
    }

    template<std::size_t... Is>
    static T construct(arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        aligned_storage_tuple_type stored_args;
        T ret{aligned_type_holder::construct<Is>(stored_args, t)...};
        destruct_stored_args(stored_args);
        return ret;
    }

    template<typename U, typename... Args, std::size_t... Is>
    static U tuple_construct(const std::tuple<Args...>& args, std::index_sequence<Is...>)
    {
        return U(std::get<Is>(args)...);
    }

    template<typename U, typename... Args>
    static U tuple_construct(const std::tuple<Args...>& args)
    {
        return tuple_construct<U>(args,std::make_index_sequence<sizeof...(Args)>{});
    }

    template<std::size_t I>
    static void destruct_stored_arg(aligned_storage_tuple_type& storage_tuple)
    {
        using storage_type = storage_type_at_storage_index<I>;
        //std::cout << "Reverse indexes: " << abi::__cxa_demangle(typeid(storage_tuple_types).name(),0,0,0) << std::endl;
        //std::cout << abi::__cxa_demangle(typeid(argument<I>(storage_tuple)).name(),0,0,0) << " | " << abi::__cxa_demangle(typeid(storage_type).name(),0,0,0) << std::endl;
        argument<I>(storage_tuple).~storage_type();
    }

    template<std::size_t... Is>
    static void destruct_stored_args(aligned_storage_tuple_type& storage_tuple, std::index_sequence<Is...>)
    {
        (void)swallow{(destruct_stored_arg<Is>(storage_tuple), 0)...};
    }

public:

    template<std::size_t I>
    static storage_type_at_storage_index<I>& argument(aligned_storage_tuple_type& storage_tuple)
    {
        using arg_type = storage_type_at_storage_index<I>;
        return reinterpret_cast<arg_type&>(std::get<I>(storage_tuple));
    }

    static T construct(construction_info_type& t)
    {
        return construct(t.args(),std::make_index_sequence<construction_info_type::num_args()>{});
    }

    static void destruct_stored_args(aligned_storage_tuple_type& storage_tuple)
    {
        destruct_stored_args(storage_tuple, std::make_index_sequence<stored_count>{});
    }
    
    aligned_type_holder(typename aligned_type_holder::arguments_tuple_type& t)
        : m_val{}
        , m_stored_args{}
    {
        construct(reinterpret_cast<std::remove_cv_t<T>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }

    aligned_type_holder(typename aligned_type_holder::construction_info_type& t) : aligned_type_holder(t.args())
    {}

    aligned_type_holder()
        : m_val{}, m_stored_args{}
    {}

    void construct(typename aligned_type_holder::arguments_tuple_type& t)
    {
        construct(reinterpret_cast<std::remove_cv_t<T>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }

    T& value() { return reinterpret_cast<T&>(m_val); }
    const T& value() const { return reinterpret_cast<const T&>(m_val); }

    template<std::size_t I>
    storage_type_at_storage_index<I>& argument() { return argument<I>(m_stored_args); }

    template<std::size_t I>
    const storage_type_at_storage_index<I>& argument() const { return argument<I>(m_stored_args); }

    ~aligned_type_holder()
    {
        reinterpret_cast<T*>(&m_val)->~T();
        destruct_stored_args(m_stored_args);
    }
    
private:
    type_storage m_val;
    aligned_storage_tuple_type m_stored_args;
};

template<typename T, std::size_t N, typename Alignment, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T[N],Alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
};

template<typename T, std::size_t N, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T[N],std::integral_constant<std::size_t,0>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
};

template<typename T, typename Alignment, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T[],Alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
};

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T[],std::integral_constant<std::size_t,0>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
};

}

}

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNED_TYPE_HOLDER_HPP */
