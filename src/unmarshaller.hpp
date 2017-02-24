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
#include "internal/type_properties.hpp"
#include "internal/alignment.hpp"
#include "internal/utility.hpp"
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



template<typename T>
struct is_construction_info : std::false_type {};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple>
struct is_construction_info<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>> : std::true_type {};

template<typename T, typename Alignment, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
class aligned_type_holder;

/*template<typename... Ts, std::size_t Alignment>
struct type_default_alignment_helper<std::tuple<Ts...>,Alignment>
{
};*/

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple, typename = void>
class construction_info_to_aligned_type_holder;

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class construction_info_to_aligned_type_holder<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
    using type_alignment = std::conditional_t<(sizeof...(Alignments) > 0),
                                              internal::alignment_reader_type<std::tuple<Alignments...>>,
                                              std::integral_constant<std::size_t,alignof(T)>
                                             >;
    using internal_alignments = std::conditional_t<(sizeof...(Alignments) == sizeof...(ArgumentTypes) +1),
                                                   internal::internal_alignments_tuple_type<std::tuple<Alignments...>>,
                                                   std::tuple<type_default_alignment<ArgumentTypes,alignof(ArgumentTypes)>...>
                                                  >;
    using type = aligned_type_holder<T,type_alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,internal_alignments>;
};

template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments>
class construction_info<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>>
{
public:
    using ConstructorArgumentTypesTuple = std::tuple<ConstructorArgumentTypes...>;
    using ArgumentsTuple = std::tuple<ArgumentTypes...>;
    using StoredArgumentsTuple = std::tuple<StoredArguments...>;
    
    template<typename AlignmentsTuple>
    using make_aligned_type_holder = typename construction_info_to_aligned_type_holder<T,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,AlignmentsTuple>::type;
    
    /*template<typename AlignmentsTuple>
    using make_aligned_type_holder = std::enable_if_t<std::tuple_size<AlignmentsTuple>::value == sizeof...(ArgumentTypes) + 1, aligned_type_holder<
              T
            , mpirpc::internal::alignment_reader_type<AlignmentsTuple>
            , std::tuple<ConstructorArgumentTypes...>
            , std::tuple<ArgumentTypes...>
            , std::tuple<StoredArguments...>
            , mpirpc::internal::internal_alignments_tuple_type<AlignmentsTuple>
        >>;
        
    template<typename AlignmentsTuple>
    using make_aligned_type_holder = std::enable_if_t<std::tuple_size<AlignmentsTuple>::value == 1, aligned_type_holder<
              T
            , mpirpc::internal::alignment_reader_type<AlignmentsTuple>
            , std::tuple<ConstructorArgumentTypes...>
            , std::tuple<ArgumentTypes...>
            , std::tuple<StoredArguments...>
            , std::tuple<type_default_alignment<ArgumentTypes,alignof(ArgumentTypes)>...>
        >>;*/
        
    template<typename... Args, std::enable_if_t<std::is_constructible<ArgumentsTuple,Args...>::value>* = nullptr>
    construction_info(Args&&... args)
        : arguments{std::forward<Args>(args)...}
    {
        using swallow = int[];
        (void)swallow{(std::cout << abi::__cxa_demangle(typeid(args).name(),0,0,0) << std::endl, 0)...};
        //static_assert(sizeof...(Args) == std::tuple_size<ArgumentsTuple>::value, "Must have the same number of arguments");
        std::cout << sizeof...(Args) << std::endl;
        std::cout << std::tuple_size<ArgumentsTuple>::value << std::endl;
        std::cout << abi::__cxa_demangle(typeid(ArgumentsTuple).name(),0,0,0) << std::endl;
    }
    
    ArgumentsTuple& args() { return arguments; }
    
    static constexpr std::size_t num_args() { return sizeof...(ArgumentTypes); }
private:
    ArgumentsTuple arguments;
};

template<typename T, typename Store, typename Alignment>
struct storage_type_helper
{
    using type = T;
    using constructor_type = T;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Alignment, typename... Alignments>
struct storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::true_type,std::tuple<Alignment,Alignments...>>
{
    using type = aligned_type_holder<T,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using constructor_type = aligned_type_holder<T,Alignment,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>::type;
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename Alignment, typename... Alignments>
struct storage_type_helper<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>,std::false_type,std::tuple<Alignment,Alignments...>>
{
    using type = T;
    using constructor_type = aligned_type_holder<void,std::integral_constant<std::size_t,0ULL>,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple,std::tuple<Alignments...>>;
    using aligned_storage_type = typename std::aligned_storage<sizeof(type),Alignment::value>::type;
};

template<typename T, typename Alignment, typename = void>
struct is_overaligned_type : std::false_type {};

template<typename T, typename Alignment>
struct is_overaligned_type<T,Alignment,std::enable_if_t<(mpirpc::internal::alignment_reader<Alignment>::value > alignof(T))>> : std::true_type {};

template<typename T, typename Alignment>
constexpr bool is_overaligned_type_v = is_overaligned_type<T,Alignment>::value;

template<typename T, typename Store, typename Alignment, typename = void>
struct is_stored_type : std::false_type {};

template<typename T, typename Store, typename Alignment>
struct is_stored_type<T,Store,Alignment,
                      std::enable_if_t<
                          Store::value ||
                          is_construction_info<T>::value ||
                          (
                              std::is_reference<T>::value &&
                              is_overaligned_type_v<T,Alignment>
                          )
                      >
                     >
        : std::true_type {};

template<typename T, typename Store, typename Alignment, typename = void>
struct is_static_construct_temporary_type : std::false_type {};

template<typename T, typename Store, typename Alignment>
struct is_static_construct_temporary_type<T,Store,Alignment,
                      std::enable_if_t<
                          is_construction_info<T>::value ||
                          (
                              std::is_reference<T>::value &&
                              is_overaligned_type<T,Alignment>::value
                          )
                      >
                     >
        : std::true_type {};

template<std::size_t Size, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
struct stored_arguments_impl;

template<std::size_t Size, typename Argument, typename StoredArgument, typename Alignment>
struct stored_arguments_impl<Size,std::tuple<Argument>,std::tuple<StoredArgument>,std::tuple<Alignment>>
{
    using type = typename storage_type_helper<Argument,StoredArgument,Alignment>::type;
    using types = std::conditional_t<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , std::tuple<std::remove_reference_t<type>>
                      , std::tuple<>
                  >;
    using tuple = std::conditional_t<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , std::tuple<typename std::aligned_storage<sizeof(std::remove_reference_t<type>), mpirpc::internal::alignment_reader<Alignment>::value>::type>
                      , std::tuple<>
                  >;
    static constexpr std::size_t index = Size-1;
    using indexes = std::conditional_t<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , std::index_sequence<index>
                      , std::index_sequence<>
                  >;
    using static_construct_types = std::conditional_t<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , std::tuple<std::remove_reference_t<type>>
                      , std::tuple<>
                  >;
    using static_construct_tuple = std::conditional_t<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , std::tuple<typename std::aligned_storage<sizeof(std::remove_reference_t<type>), mpirpc::internal::alignment_reader<Alignment>::value>::type>
                      , std::tuple<>
                  >;
    using static_construct_indexes = std::conditional_t<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , std::index_sequence<index>
                      , std::index_sequence<>
                  >;
};

template<std::size_t Size, typename Argument, typename... Arguments, typename StoredArgument, typename... StoredArguments, typename Alignment, typename... Alignments>
struct stored_arguments_impl<Size,std::tuple<Argument,Arguments...>,std::tuple<StoredArgument,StoredArguments...>,std::tuple<Alignment,Alignments...>>
{
    using type = typename storage_type_helper<Argument,StoredArgument,Alignment>::type;
    using prev = stored_arguments_impl<Size,std::tuple<Arguments...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using prev_types = typename prev::types;
    using prev_tuple = typename prev::tuple;
    using prev_indexes = typename prev::indexes;
    static constexpr std::size_t index = Size-sizeof...(Arguments)-1;
    using types = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , std::remove_reference_t<type>
                      , prev_types
                  >;
    using tuple = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , typename std::aligned_storage<sizeof(std::remove_reference_t<type>),mpirpc::internal::alignment_reader<Alignment>::value>::type
                      , prev_tuple
                  >;
    
    using indexes = mpirpc::internal::conditional_index_sequence_prepend_type<
                        is_stored_type<Argument,StoredArgument,Alignment>::value
                      , index
                      , prev_indexes
                  >;
    using static_construct_types = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , std::remove_reference_t<type>
                      , typename prev::static_construct_types
                  >;
    using static_construct_tuple = mpirpc::internal::conditional_tuple_type_prepend_type<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , typename std::aligned_storage<sizeof(std::remove_reference_t<type>),mpirpc::internal::alignment_reader<Alignment>::value>::type
                      , typename prev::static_construct_tuple
                  >;
    using static_construct_indexes = mpirpc::internal::conditional_index_sequence_prepend_type<
                        is_static_construct_temporary_type<Argument,StoredArgument,Alignment>::value
                      , index
                      , typename prev::static_construct_indexes
                  >;
};

template<std::size_t Size, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
struct stored_arguments_impl2;

template<std::size_t Size, typename Argument, typename StoredArgument, typename Alignment>
struct stored_arguments_impl2<Size,std::tuple<Argument>,std::tuple<StoredArgument>,std::tuple<Alignment>>
{
    using type = typename storage_type_helper<Argument,StoredArgument,Alignment>::type;
    static constexpr std::size_t index = (StoredArgument::value || is_construction_info<type>::value) ? Size-1 : Size;
    using tuple_indexes = std::conditional_t<
                              is_stored_type<Argument,StoredArgument,Alignment>::value
                            , std::tuple<std::integral_constant<std::size_t,Size-1>>
                            , std::tuple<decltype(std::ignore)>
                          >;
};

template<std::size_t Size, typename Argument, typename... Arguments, typename StoredArgument, typename... StoredArguments, typename Alignment, typename... Alignments>
struct stored_arguments_impl2<Size,std::tuple<Argument,Arguments...>,std::tuple<StoredArgument,StoredArguments...>,std::tuple<Alignment,Alignments...>>
{
    using type = typename storage_type_helper<Argument,StoredArgument,Alignment>::type;
    using prev_stored_arguments = stored_arguments_impl2<Size,std::tuple<Arguments...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using prev_tuple_indexes = typename prev_stored_arguments::tuple_indexes;
    
    constexpr static std::size_t index = (StoredArgument::value) ? prev_stored_arguments::index - 1 : prev_stored_arguments::index;
    
    using tuple_indexes = std::conditional_t<
                              is_stored_type<Argument,StoredArgument,Alignment>::value
                            , mpirpc::internal::tuple_type_prepend_type<
                                std::integral_constant<std::size_t,index>,
                                prev_tuple_indexes
                              >
                            , mpirpc::internal::tuple_type_prepend_type<
                                decltype(std::ignore),
                                prev_tuple_indexes
                              >
                          >;
};

template<typename T>
struct constructor_argument_info
{
    using type = T;
    
    template<typename U>
    static T get(U&& u)
    {
        return u;
    }
};

template<typename T, typename ConstructorArgumentTypesTuple, typename ArgumentsTuple, typename StoredArgumentsTuple>
struct constructor_argument_info<construction_info<T,ConstructorArgumentTypesTuple,ArgumentsTuple,StoredArgumentsTuple>>
{
    using type = T;
};

template<typename ConstructorArgumentsTuple, typename ArgumentsTuple, typename StoredArgumentsTuple, typename AlignmentsTuple>
struct stored_arguments;

template<typename...ConstructorArguments, typename... Arguments, typename... StoredArguments, typename... Alignments>
struct stored_arguments<std::tuple<ConstructorArguments...>,std::tuple<Arguments...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
    using constructor_arguments_tuple_type = std::tuple<ConstructorArguments...>;
    using arguments_tuple_type = std::tuple<Arguments...>;
    using stored_arguments_tuple_type = std::tuple<StoredArguments...>;
    using alignments_tuple_type = std::tuple<Alignments...>;
    using storage_tuple_type = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::tuple;
    using storage_types = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::types;
    using storage_tuple_indexes_type = typename stored_arguments_impl2<std::tuple_size<storage_tuple_type>::value,arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::tuple_indexes;
    using storage_tuple_reverse_indexes_type = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::indexes;
    using proxy_tuple_type = std::tuple<ConstructorArguments&&...>;
    using static_construct_types = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::static_construct_types;
    using static_construct_tuple = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::static_construct_tuple;
    using static_construct_indexes = typename stored_arguments_impl<sizeof...(Arguments),arguments_tuple_type,stored_arguments_tuple_type,alignments_tuple_type>::static_construct_indexes;
};

template<typename T>
struct type_constructor
{
    template<typename... Args>
    static void construct(T *t, Args&&... args)
    {
        new (t) T(std::forward<Args>(args)...);
    }
};



template<typename T, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T,std::integral_constant<std::size_t,0ULL>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
public:
    using type = T;
    //using type_storage = typename std::aligned_storage<sizeof(T),Alignment::value>::type;
    using constructor_argument_types_tuple = std::tuple<ConstructorArgumentTypes...>;
    //static constexpr std::size_t alignment = mpirpc::internal::alignment_reader<Alignment>::value;
    using stored_arguments_info = stored_arguments<std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using stored_arguments_tuple_type = typename stored_arguments_info::stored_arguments_tuple_type;
    using arguments_tuple_type = typename stored_arguments_info::arguments_tuple_type;
    using proxy_tuple_type = typename stored_arguments_info::proxy_tuple_type;
    using storage_tuple_type = typename stored_arguments_info::storage_tuple_type;
    using storage_tuple_types = typename stored_arguments_info::storage_types;
    using storage_tuple_indexes_type = typename stored_arguments_info::storage_tuple_indexes_type;
    using storage_tuple_reverse_indexes_type = typename stored_arguments_info::storage_tuple_reverse_indexes_type;
    using construction_info_type = construction_info<T,constructor_argument_types_tuple,arguments_tuple_type,stored_arguments_tuple_type>;
    using storage_types = std::tuple<typename storage_type_helper<ArgumentTypes,StoredArguments,Alignments>::type...>;
    using storage_construction_types = std::tuple<typename storage_type_helper<ArgumentTypes,StoredArguments,Alignments>::constructor_type...>;
    
    template<std::size_t I>
    using argument_at_index = std::tuple_element_t<I,arguments_tuple_type>;
    
    template<std::size_t I>
    using storage_constructor_type_at_index = std::tuple_element_t<I,storage_construction_types>;
    
    template<std::size_t I>
    using storage_type_at_index = std::tuple_element_t<I,storage_types>;
    
    template<std::size_t I>
    using storage_type_at_storage_index = std::tuple_element_t<I, storage_tuple_types>;

protected:
    template<std::size_t I, 
             std::enable_if_t<std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<is_construction_info<argument_at_index<I>>::value>* = nullptr>
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
             std::enable_if_t<!is_construction_info<argument_at_index<I>>::value>* = nullptr>
    static std::tuple_element_t<I,proxy_tuple_type> construct(storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        constexpr std::size_t SI = std::tuple_element_t<I,storage_tuple_indexes_type>::value;
        using arg_type = argument_at_index<I>;
        type_constructor<std::remove_cv_t<arg_type>>::construct(reinterpret_cast<std::remove_cv_t<arg_type>*>(&std::get<SI>(stored_args)), std::get<I>(args));
        std::cout << "Copy constructing: " << abi::__cxa_demangle(typeid(arg_type).name(),0,0,0) << std::endl;
        return reinterpret_cast<std::tuple_element_t<I,proxy_tuple_type>>(std::get<SI>(stored_args));
    }
    
    //do not stored and is_construction_info true
    template<std::size_t I,
             std::enable_if_t<!std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<is_construction_info<argument_at_index<I>>::value>* = nullptr>
    static std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<I,proxy_tuple_type>>> construct(storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        using arg_type = std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<I,proxy_tuple_type>>>;
        using con_type = storage_constructor_type_at_index<I>;
        std::cout << "Constructing using construction_info (non-saved): " << abi::__cxa_demangle(typeid(std::tuple_element_t<I,proxy_tuple_type>).name(),0,0,0) << " | " << abi::__cxa_demangle(typeid(con_type).name(),0,0,0) << std::endl;
        return con_type::construct(std::get<I>(args));//tuple_construct<arg_type>(std::get<I>(args).args());
    }
    
    template<std::size_t I,
             std::enable_if_t<!std::tuple_element_t<I,std::tuple<StoredArguments...>>::value>* = nullptr,
             std::enable_if_t<!is_construction_info<argument_at_index<I>>::value>* = nullptr>
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
        std::cout << "Reverse indexes: " << abi::__cxa_demangle(typeid(storage_tuple_types).name(),0,0,0) << std::endl;
        std::cout << abi::__cxa_demangle(typeid(argument<I>(storage_tuple)).name(),0,0,0) << " | " << abi::__cxa_demangle(typeid(storage_type).name(),0,0,0) << std::endl;
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
    using stored_arguments_info = stored_arguments<std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using stored_arguments_tuple_type = typename stored_arguments_info::stored_arguments_tuple_type;
    using arguments_tuple_type = typename stored_arguments_info::arguments_tuple_type;
    using proxy_tuple_type = typename stored_arguments_info::proxy_tuple_type;
    using storage_tuple_type = typename stored_arguments_info::storage_tuple_type;
    using storage_tuple_types = typename stored_arguments_info::storage_types;
    using storage_tuple_indexes_type = typename stored_arguments_info::storage_tuple_indexes_type;
    using storage_tuple_reverse_indexes_type = typename stored_arguments_info::storage_tuple_reverse_indexes_type;
    using construction_info_type = construction_info<T,constructor_argument_types_tuple,arguments_tuple_type,stored_arguments_tuple_type>;
    using storage_types = std::tuple<typename storage_type_helper<ArgumentTypes,StoredArguments,Alignments>::type...>;
    using storage_construction_types = std::tuple<typename storage_type_helper<ArgumentTypes,StoredArguments,Alignments>::constructor_type...>;
    using super = aligned_type_holder<T,std::integral_constant<std::size_t,0ULL>,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    
    template<std::size_t I>
    using argument_at_index = std::tuple_element_t<I,arguments_tuple_type>;
    
    template<std::size_t I>
    using storage_constructor_type_at_index = std::tuple_element_t<I,storage_construction_types>;
    
    template<std::size_t I>
    using storage_type_at_index = std::tuple_element_t<I,storage_types>;
    
    template<std::size_t I>
    using storage_type_at_storage_index = std::tuple_element_t<I, storage_tuple_types>;
    
public:
    aligned_type_holder(arguments_tuple_type& t)
    {
        super::construct(reinterpret_cast<std::remove_cv_t<T>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }
    
    aligned_type_holder(construction_info_type& t)
        : aligned_type_holder(t.args())
    {}
    
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

template<typename ParameterTypesTuple, typename TypesTuple, typename StoreArgumentsTuple, typename AlignmentsTuple>
class aligned_parameter_holder;

template<typename... ParameterTypes, typename... Types, typename... StoredArguments, typename... Alignments>
class aligned_parameter_holder<std::tuple<ParameterTypes...>, std::tuple<Types...>, std::tuple<StoredArguments...>, std::tuple<Alignments...>>
    : public aligned_type_holder<void,std::integral_constant<std::size_t,0ULL>,std::tuple<ParameterTypes...>,std::tuple<Types...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
public:
    using parameter_types_tuple = std::tuple<ParameterTypes...>;
    using stored_arguments_info = stored_arguments<std::tuple<ParameterTypes...>,std::tuple<Types...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;
    using stored_arguments_tuple_type = typename stored_arguments_info::stored_arguments_tuple_type;
    using parameter_tuple_type = typename stored_arguments_info::arguments_tuple_type;
    using proxy_tuple_type = typename stored_arguments_info::proxy_tuple_type;
    using storage_tuple_type = typename stored_arguments_info::storage_tuple_type;
    using storage_tuple_types = typename stored_arguments_info::storage_types;
    using storage_tuple_indexes_type = typename stored_arguments_info::storage_tuple_indexes_type;
    using storage_tuple_reverse_indexes_type = typename stored_arguments_info::storage_tuple_reverse_indexes_type;
    //using construction_info_type = construction_info<T,constructor_argument_types_tuple,arguments_tuple_type,stored_arguments_tuple_type>;
    using storage_types = std::tuple<typename storage_type_helper<Types,StoredArguments,Alignments>::type...>;
    using storage_construction_types = std::tuple<typename storage_type_helper<Types,StoredArguments,Alignments>::constructor_type...>;
    using super = aligned_type_holder<void,std::integral_constant<std::size_t,0ULL>,std::tuple<ParameterTypes...>,std::tuple<Types...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>;

    template<std::size_t I>
    using storage_type_at_storage_index = std::tuple_element_t<I, storage_tuple_types>;
    
protected:
    template<std::size_t... Is>
    void construct(parameter_tuple_type& t, std::index_sequence<Is...>)
    {
        using swallow = int[];
        (void)swallow{(super::construct<Is>(m_stored_args, t), 0)...};
    }
public:
    aligned_parameter_holder(parameter_tuple_type& t)
    {
        construct(t,std::make_index_sequence<sizeof...(Types)>{});
    }
    
    template<typename... Ts,
             std::enable_if_t<(std::is_constructible<Types,Ts>::value && ...)>* = nullptr>
    aligned_parameter_holder(Ts&&... args)
        : aligned_parameter_holder(parameter_tuple_type{std::forward<Ts>(args)...})
    {}
    
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
    
    ~aligned_parameter_holder()
    {
        super::destruct_stored_args(m_stored_args);
    }
private:
    storage_tuple_type m_stored_args;
};

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


/*template<typename R, typename...Ts, std::size_t...Is>
R construct_impl(const std::tuple<std::piecewise_construct_t,Ts...>& t, std::index_sequence<Is...>)
{
    return R(std::get<Is+1>(t)...);
}

template<typename R, typename...Ts>
R construct(const std::tuple<std::piecewise_construct_t,Ts...>& t)
{
    return construct_impl<R>(t,std::index_sequence_for<Ts...>{});
}

template<typename R, typename T, std::enable_if_t<!internal::is_piecewise_construct_tuple<std::remove_reference_t<T>>::value>* = nullptr>
R construct(T&& t)
{
    return R(std::forward<T>(t));
}*/

template<typename Buffer, typename Alignment, typename... Ts>
struct unmarshaller<std::tuple<Ts...>,Buffer,Alignment>
{
    template<typename Allocator>
    static std::tuple<Ts...> unmarshal(Allocator&& a, Buffer& b)
    {
        //return std::tuple<std::remove_reference_t<Ts>...>{ construct<std::remove_reference_t<Ts>>(unmarshaller<Ts,Buffer,alignof(Ts)>::unmarshal(std::forward<Allocator>(a),b))... };
    }
};

}

#endif /* MPIRPC__UNMARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
