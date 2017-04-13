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

#ifndef MPIRPC__UNMARSHALLER_HPP
#define MPIRPC__UNMARSHALLER_HPP

#include <utility>
#include "common.hpp"
#include "detail/unmarshaller.hpp"
#include "internal/type_properties.hpp"
#include "internal/alignment.hpp"
#include "internal/utility.hpp"
#include "internal/reconstruction/aligned_type_holder.hpp"
#include "types.hpp"
#include <cxxabi.h>

namespace mpirpc
{

/*
 * Unmarshalling is a significantly greater challenge than marshalling. When marshalling,
 * the input is already in a valid constructed state.
 *
 * However, when unmarshalling there may be one or more different limitations of the types
 * which need to be handled. First of all, there is the issue of allocating memory. This
 * will often be different for local and remote unmarshalling, since local unmarshalling
 * will already have references or pointers to already-existing memory pass in as parameters.
 * Remote unmarshalling, on the other hand, often requires allocating memory.
 */

/*template<typename... Ts, std::size_t Alignment>
struct type_default_alignment_helper<std::tuple<Ts...>,Alignment>
{
};*/





template<typename T>
struct constructor_argument_info
{
    using type = T;
    
    template<typename U>
    static U&& get(U&& u)
    {
        return u;
    }
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple>
struct constructor_argument_info<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>>
{
    using type = T;
};



template<typename T, typename Buffer, typename Allocator>
using has_unmarshaller = typename detail::has_unmarshaller_helper<T,Buffer,Allocator>::type;

template<typename T, typename Buffer, typename Allocator>
inline constexpr bool has_unmarshaller_v = detail::has_unmarshaller_helper<T,Buffer,Allocator>::value;

template<typename T, typename Buffer, typename Allocator>
using unmarshaller_type = typename detail::unmarshaller_type_helper<T,Buffer,Allocator>::type;

template<typename T, typename Buffer, typename Alignment, typename = void>
struct storage_type_conversion
{
    using type = std::aligned_storage_t<sizeof(T),mpirpc::internal::alignment_reader<Alignment>::value>;
};

template<typename T, typename Buffer, typename Alignments>
struct storage_type_conversion<T,Buffer,Alignments,std::enable_if_t<internal::reconstruction::is_construction_info_v<unmarshaller_type<T,Buffer,std::allocator<char>>>>>
{
    using type = internal::reconstruction::construction_info_to_aligned_type_holder_type<unmarshaller_type<T,Buffer,std::allocator<char>>,Alignments>;
};

template<typename T, typename Buffer, typename Alignment>
using storage_type_conversion_type = typename storage_type_conversion<T,Buffer,Alignment>::type;

template<typename Tuple, typename Buffer, typename AlignmentsTuple>
struct storage_tuple_from_types;

template<typename Types, typename Buffer, typename Alignments>
struct storage_construction_types;

template<typename T, typename... Ts, typename Buffer, typename Alignment, typename... Alignments>
struct storage_construction_types<std::tuple<T,Ts...>, Buffer,std::tuple<Alignment,Alignments...>>
    : internal::tuple_type_prepend<storage_type_conversion_type<std::remove_reference_t<std::remove_cv_t<T>>,Buffer,Alignment>,
        typename storage_construction_types<std::tuple<Ts...>,Buffer,std::tuple<Alignments...>>::type>
{};

template<typename T, typename Buffer, typename Alignment>
struct storage_construction_types<std::tuple<T>,Buffer,std::tuple<Alignment>>
//    : internal::tuple_type_prepend<storage_type_conversion_type<T,Buffer,Alignment>,std::tuple<>>
{
    using stype = storage_type_conversion_type<T,Buffer,Alignment>;
    using type = internal::tuple_type_prepend_type<storage_type_conversion_type<T,Buffer,Alignment>,std::tuple<>>;
};

template<typename Types, typename Buffer, typename Alignments>
using storage_construction_types_type = typename storage_construction_types<Types,Buffer,Alignments>::type;

template<bool... Bs>
struct count_trues;

template<bool B,bool... Bs>
struct count_trues<B,Bs...>
    : std::integral_constant<std::size_t,std::size_t(B)+count_trues<Bs...>::value>
{};

template<bool B>
struct count_trues<B>
    : std::integral_constant<std::size_t,std::size_t(B)>
{};

template<typename... Bs>
struct count_integral_constant_bools;

template<bool... Bs>
struct count_integral_constant_bools<std::tuple<std::integral_constant<bool,Bs>...>> : count_trues<Bs...> {};

struct invalid_index_type {};

constexpr invalid_index_type invalid_index;

template<std::size_t Size, bool... Included>
struct filtered_indexes_helper;

template<std::size_t Size, bool... Included>
struct filtered_indexes_helper<Size,true,Included...>
{
    constexpr static std::size_t index = filtered_indexes_helper<Size,Included...>::next_index;
    constexpr static std::size_t next_index = index-1;
    constexpr static std::size_t size = Size;
    using type = internal::tuple_type_prepend_type<std::integral_constant<std::size_t,index>, typename filtered_indexes_helper<Size,Included...>::type>;
};

template<std::size_t Size, bool... Included>
struct filtered_indexes_helper<Size,false,Included...>
{
    constexpr static std::size_t index = filtered_indexes_helper<Size,Included...>::next_index;
    constexpr static std::size_t next_index = index;
    constexpr static std::size_t size = Size;
    using type = internal::tuple_type_prepend_type<invalid_index_type, typename filtered_indexes_helper<Size,Included...>::type>;
};

template<std::size_t Size>
struct filtered_indexes_helper<Size,true>
{
    static_assert(Size != 0);
    constexpr static std::size_t index = Size-1;
    constexpr static std::size_t next_index = index-1;
    constexpr static std::size_t size = Size;
    using type = std::tuple<std::integral_constant<std::size_t,index>>;
};

template<std::size_t Size>
struct filtered_indexes_helper<Size,false>
{
    static_assert(Size != 0);
    constexpr static std::size_t index = Size-1;
    constexpr static std::size_t next_index = index;
    constexpr static std::size_t size = Size;
    using type = std::tuple<invalid_index_type>;
};

template<bool... Included>
struct filtered_indexes : filtered_indexes_helper<count_trues<Included...>::value,Included...>
{
    constexpr static std::size_t size = count_trues<Included...>::value;
    using input_tuple = std::tuple<std::integral_constant<bool,Included>...>;
};

template<bool... Included>
using filtered_indexes_type = typename filtered_indexes<Included...>::type;

template<typename... Included>
struct filtered_tuple_indexes;

template<bool... Included>
struct filtered_tuple_indexes<std::tuple<std::integral_constant<bool,Included>...>> : filtered_indexes<Included...> {};

template<typename... Included>
using filtered_tuple_indexes_type = typename filtered_tuple_indexes<Included...>::type;



/*template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T,std::integral_constant<std::size_t,0LL>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
public:
    using type = T;
    using constructor_argument_types_tuple = std::tuple<ConstructorArgumentTypes...>;
    using stored_arguments_info = stored_arguments<std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using stored_arguments_tuple_type = typename stored_arguments_info::stored_arguments_tuple_type;
    using arguments_tuple_type = typename stored_arguments_info::arguments_tuple_type;
    using proxy_tuple_type = typename stored_arguments_info::proxy_tuple_type;
    using storage_tuple_type = typename stored_arguments_info::storage_tuple_type;
    using storage_tuple_indexes_type = typename stored_arguments_info::storage_tuple_indexes_type;
    using storage_tuple_reverse_indexes_type = typename stored_arguments_info::storage_tuple_reverse_indexes_type;
    using construction_info_type = construction_info<T,constructor_argument_types_tuple,arguments_tuple_type,stored_arguments_tuple_type>;
    using storage_types = std::tuple<typename storage_type_helper<ArgumentTypes,StoredArguments,Alignments>::type...>;
    using storage_construction_types = std::tuple<typename storage_type_helper<ArgumentTypes,StoredArguments,Alignments>::constructor_type...>;
    
    template<std::size_t I>
    using argument_at_index = std::tuple_element_t<I,arguments_tuple_type>;
    
    template<std::size_t I>
    using storage_constructor_type_at_index = std::tuple_element_t<I,storage_construction_types>;

private:
    template<std::size_t I, 
             std::enable_if_t<std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<is_construction_info<argument_at_index<I>>::value>* = nullptr>
    std::tuple_element_t<I,proxy_tuple_type> construct(arguments_tuple_type& args)
    {
        using arg_type = argument_at_index<I>;
        using con_type = storage_constructor_type_at_index<I>;
        constexpr std::size_t SI = std::tuple_element_t<I,storage_tuple_indexes_type>::value;
        con_type ct(&std::get<SI>(m_stored_args), std::get<I>(args));
        std::cout << "Constructing using construction_info" << std::endl;
        return reinterpret_cast<std::tuple_element_t<I,proxy_tuple_type>>(std::get<SI>(m_stored_args));
    }
    
    template<std::size_t I, 
             std::enable_if_t<std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<!is_construction_info<argument_at_index<I>>::value>* = nullptr>
    std::tuple_element_t<I,proxy_tuple_type> construct(arguments_tuple_type& args)
    {
        constexpr std::size_t SI = std::tuple_element_t<I,storage_tuple_indexes_type>::value;
        using arg_type = argument_at_index<I>;
        type_constructor<std::remove_cv_t<arg_type>>::construct(reinterpret_cast<std::remove_cv_t<arg_type>*>(&std::get<SI>(m_stored_args)), std::get<I>(args));
        std::cout << "Copy constructing: " << abi::__cxa_demangle(typeid(arg_type).name(),0,0,0) << std::endl;
        return reinterpret_cast<std::tuple_element_t<I,proxy_tuple_type>>(std::get<SI>(m_stored_args));
    }
    
    //do not stored and is_construction_info true
    
    template<std::size_t I, std::enable_if_t<!std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr>
    std::tuple_element_t<I,proxy_tuple_type> construct(arguments_tuple_type& args)
    {
        return reinterpret_cast<std::tuple_element_t<I,proxy_tuple_type>>(std::get<I>(args));
    }
    
    template<std::size_t... Is>
    void construct(T *val, arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        type_constructor<std::remove_cv_t<T>>::construct(reinterpret_cast<std::remove_cv_t<T>*>(val),aligned_type_holder::construct<Is>(t)...);
    }
    
public:
    aligned_type_holder(arguments_tuple_type& t)
    {
        construct(std::make_index_sequence<std::tuple_size<arguments_tuple_type>::value>{});
    }
    
    aligned_type_holder()
        : aligned_type_holder(t.args())
    {}
    
    T construct(construction_info_type& t)
    {
    }
    
    storage_tuple_type& stored_args() { return m_stored_args; }
    
    ~aligned_type_holder()
    {
        destruct_stored_args(std::make_index_sequence<std::tuple_size<storage_tuple_type>::value>{});
    }
    
private:
    storage_tuple_type m_stored_args;
};*/


template<typename Buffer, typename Alignment, typename... Ts>
struct unmarshaller<std::tuple<Ts...>,Buffer,Alignment>
{
    template<typename Allocator>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        return construction_info<
            std::tuple<Ts...>, //type
            std::tuple<Ts...>, //constructor arguments
            std::tuple<unmarshaller_type<Ts,Buffer,Allocator>...>, //unmarshalled arguments
            std::tuple<typename std::is_reference<Ts>::type...> //store?
        >{mpirpc::get<std::remove_reference_t<Ts>>(b,a)...};
        //return std::tuple<std::remove_reference_t<Ts>...>{ construct<std::remove_reference_t<Ts>>(unmarshaller<Ts,Buffer,alignof(Ts)>::unmarshal(std::forward<Allocator>(a),b))... };
    }
    
    //template<typename Allocator>
    //static decltype(auto)
};

}

#endif /* MPIRPC__UNMARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
