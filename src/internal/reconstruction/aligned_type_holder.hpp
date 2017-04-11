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

#ifndef MPIRPC__INTERNAL__ALIGNED_TYPE_HOLDER_HPP
#define MPIRPC__INTERNAL__ALIGNED_TYPE_HOLDER_HPP

#include "construction_info.hpp"
#include "detail/stored_arguments_info.hpp"
#include "type_conversion.hpp"
#include "../alignment.hpp"
#include <cxxabi.h>

namespace mpirpc
{
    
namespace internal
{
    
namespace reconstruction
{
    
template<typename T>
struct type_constructor
{
    template<typename... Args>
    static void construct(T *t, Args&&... args)
    {
        new (t) T(std::forward<Args>(args)...);
    }
};







template<typename T, typename Alignment, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
class aligned_type_holder;

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T,std::integral_constant<std::size_t,0ULL>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
public:
    using type = T;
    //using type_storage = typename std::aligned_storage<sizeof(T),Alignment::value>::type;
    using constructor_argument_types_tuple = std::tuple<ConstructorArgumentTypes...>;
    //static constexpr std::size_t alignment = mpirpc::internal::alignment_reader<Alignment>::value;
    using sai = ath_detail::stored_arguments_info<std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using stored_arguments_tuple_type = typename sai::stored_arguments_tuple_type;
    using arguments_tuple_type = typename sai::arguments_tuple_type;
    using proxy_tuple_type = typename sai::proxy_tuple_type;
    using storage_tuple_type = typename sai::storage_tuple_type;
    using storage_tuple_types = typename sai::storage_types;
    using storage_tuple_indexes_type = typename sai::storage_tuple_indexes_type;
    using storage_tuple_reverse_indexes_type = typename sai::storage_tuple_reverse_indexes_type;
    using construction_info_type = construction_info<T,constructor_argument_types_tuple,arguments_tuple_type,stored_arguments_tuple_type>;
    using storage_types = std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>;
    using storage_construction_types = std::tuple<reconstruction_storage_constructor_type<ArgumentTypes,StoredArguments,Alignments>...>;
    
    template<std::size_t I>
    using argument_at_index = std::tuple_element_t<I,arguments_tuple_type>;
    
    template<std::size_t I>
    using storage_constructor_type_at_index = std::tuple_element_t<I,storage_construction_types>;
    
    template<std::size_t I>
    using storage_type_at_index = std::tuple_element_t<I,storage_types>;
    
    template<std::size_t I>
    using storage_type_at_storage_index = std::tuple_element_t<I, storage_tuple_types>;
    
    template<std::size_t I>
    using index_to_storage_index = std::tuple_element_t<I,storage_tuple_indexes_type>;

protected:
    template<std::size_t I, 
             std::enable_if_t<std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<is_construction_info_v<argument_at_index<I>>>* = nullptr>
    static std::tuple_element_t<I,proxy_tuple_type> construct(storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        using arg_type = argument_at_index<I>;
        using con_type = storage_constructor_type_at_index<I>;
        constexpr std::size_t SI = std::tuple_element_t<I,storage_tuple_indexes_type>::value;
        type_constructor<con_type>::construct(reinterpret_cast<con_type*>(&std::get<SI>(stored_args)), std::get<I>(args));
        std::cout << "Constructing using construction_info (saved)" << abi::__cxa_demangle(typeid(con_type).name(),0,0,0) << std::endl;
        return reinterpret_cast<std::tuple_element_t<I,proxy_tuple_type>>(std::get<SI>(stored_args));
    }
    
    template<std::size_t I, 
             std::enable_if_t<std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<!is_construction_info_v<argument_at_index<I>>>* = nullptr>
    static std::tuple_element_t<I,proxy_tuple_type> construct(storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        constexpr std::size_t SI = std::tuple_element_t<I,storage_tuple_indexes_type>::value;
        using arg_type = argument_at_index<I>;
        type_constructor<std::remove_cv_t<std::remove_reference_t<arg_type>>>::construct(reinterpret_cast<std::remove_cv_t<std::remove_reference_t<arg_type>>*>(&std::get<SI>(stored_args)), std::get<I>(args));
        std::cout << "Copy constructing: " << abi::__cxa_demangle(typeid(arg_type).name(),0,0,0) << std::endl;
        return reinterpret_cast<std::tuple_element_t<I,proxy_tuple_type>>(std::get<SI>(stored_args));
    }
    
    //do not store and is_construction_info true
    template<std::size_t I,
             std::enable_if_t<!std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<is_construction_info_v<argument_at_index<I>>>* = nullptr>
    static std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<I,proxy_tuple_type>>> construct(storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        using arg_type = std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<I,proxy_tuple_type>>>;
        using con_type = storage_constructor_type_at_index<I>;
        std::cout << "Constructing using construction_info (non-saved): " << abi::__cxa_demangle(typeid(std::tuple_element_t<I,proxy_tuple_type>).name(),0,0,0) << " | " << abi::__cxa_demangle(typeid(con_type).name(),0,0,0) << std::endl;
        return con_type::construct(std::get<I>(args));//tuple_construct<arg_type>(std::get<I>(args).args());
    }
    
    template<std::size_t I,
             std::enable_if_t<!std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<!is_construction_info_v<argument_at_index<I>>>* = nullptr>
    static std::tuple_element_t<I,proxy_tuple_type> construct(storage_tuple_type&, arguments_tuple_type& args)
    {
        return reinterpret_cast<std::tuple_element_t<I,proxy_tuple_type>>(std::get<I>(args));
    }
    
    template<std::size_t... Is>
    static void construct(std::remove_cv_t<T> *val, storage_tuple_type &stored_args, arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        type_constructor<std::remove_cv_t<T>>::construct(reinterpret_cast<std::remove_cv_t<T>*>(val),aligned_type_holder::construct<Is>(stored_args, t)...);
    }
    
    template<std::size_t... Is>
    static T construct(arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        storage_tuple_type stored_args;
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
    static void destruct_stored_arg(storage_tuple_type& storage_tuple)
    {
        using storage_type = storage_type_at_storage_index<I>;
        //std::cout << "Reverse indexes: " << abi::__cxa_demangle(typeid(storage_tuple_types).name(),0,0,0) << std::endl;
        //std::cout << abi::__cxa_demangle(typeid(argument<I>(storage_tuple)).name(),0,0,0) << " | " << abi::__cxa_demangle(typeid(storage_type).name(),0,0,0) << std::endl;
        argument<I>(storage_tuple).~storage_type();
    }
    
    template<std::size_t... Is>
    static void destruct_stored_args(storage_tuple_type& storage_tuple, std::index_sequence<Is...>)
    {
        using swallow = int[];
        (void)swallow{(destruct_stored_arg<Is>(storage_tuple), 0)...};
    }
    
public:
    
    template<std::size_t I>
    static storage_type_at_storage_index<I>& argument(storage_tuple_type& storage_tuple)
    {
        using arg_type = storage_type_at_storage_index<I>;
        return reinterpret_cast<arg_type&>(std::get<I>(storage_tuple));
    }
    
    /*template<std::size_t I>
    static const storage_type_at_storage_index<I>& argument(storage_tuple_type& storage_tuple)
    {
        using arg_type = storage_type_at_storage_index<I>;
        return reinterpret_cast<const arg_type&>(std::get<I>(storage_tuple));
    }*/
    
    static T construct(construction_info_type& t)
    {
        return construct(t.args(),std::make_index_sequence<construction_info_type::num_args()>{});
    }
    
    static void destruct_stored_args(storage_tuple_type& storage_tuple)
    {
        destruct_stored_args(storage_tuple, std::make_index_sequence<std::tuple_size<storage_tuple_type>::value>{});
    }
    
    constexpr static std::size_t stored_argument_count() { return std::tuple_size<storage_tuple_type>::value; }
};

template<typename T, typename Alignment, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T,Alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
    : public aligned_type_holder<T,std::integral_constant<std::size_t,0ULL>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
public:
    using type = T;
    using type_storage = typename std::aligned_storage<sizeof(T),Alignment::value>::type;
    using constructor_argument_types_tuple = std::tuple<ConstructorArgumentTypes...>;
    static constexpr std::size_t alignment = mpirpc::internal::alignment_reader<Alignment>::value;
    using sai = ath_detail::stored_arguments_info<std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using stored_arguments_tuple_type = typename sai::stored_arguments_tuple_type;
    using arguments_tuple_type = typename sai::arguments_tuple_type;
    using proxy_tuple_type = typename sai::proxy_tuple_type;
    using storage_tuple_type = typename sai::storage_tuple_type;
    using storage_tuple_types = typename sai::storage_types;
    using storage_tuple_indexes_type = typename sai::storage_tuple_indexes_type;
    using storage_tuple_reverse_indexes_type = typename sai::storage_tuple_reverse_indexes_type;
    using construction_info_type = construction_info<T,constructor_argument_types_tuple,arguments_tuple_type,stored_arguments_tuple_type>;
    using storage_types = std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>;
    using storage_construction_types = std::tuple<reconstruction_storage_constructor_type<ArgumentTypes,StoredArguments,Alignments>...>;
    using super = aligned_type_holder<T,std::integral_constant<std::size_t,0ULL>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    
    template<std::size_t I>
    using argument_at_index = std::tuple_element_t<I,arguments_tuple_type>;
    
    template<std::size_t I>
    using storage_constructor_type_at_index = std::tuple_element_t<I,storage_construction_types>;
    
    template<std::size_t I>
    using storage_type_at_index = std::tuple_element_t<I,storage_types>;
    
    template<std::size_t I>
    using storage_type_at_storage_index = std::tuple_element_t<I, storage_tuple_types>;
    
    template<std::size_t I>
    using index_to_storage_index = std::tuple_element_t<I,storage_tuple_indexes_type>;
    
public:
    aligned_type_holder(arguments_tuple_type& t)
        : m_val{}
        , m_stored_args{}
    {
        super::construct(reinterpret_cast<std::remove_cv_t<T>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }
    
    aligned_type_holder(construction_info_type& t)
        : aligned_type_holder(t.args())
    {}
    
    aligned_type_holder()
        : m_val{}
        , m_stored_args{}
    {}
    
    void construct(arguments_tuple_type& t)
    {
        super::construct(reinterpret_cast<std::remove_cv_t<T>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }
    
    T& value() { return reinterpret_cast<T&>(m_val); }
    const T& value() const { return reinterpret_cast<const T&>(m_val); }
    
    template<std::size_t I>
    storage_type_at_storage_index<I>& argument()
    {
        return super::argument<I>(m_stored_args);
    }
    
    template<std::size_t I>
    const storage_type_at_storage_index<I>& argument() const
    {
        return super::argument<I>(m_stored_args);
    }
    
    ~aligned_type_holder()
    {
        reinterpret_cast<T*>(&m_val)->~T();
        super::destruct_stored_args(m_stored_args);
    }
private:
    type_storage m_val;
    storage_tuple_type m_stored_args;
};

}

}

}

#endif /* MPIRPC__INTERNAL__ALIGNED_TYPE_HOLDER_HPP */
