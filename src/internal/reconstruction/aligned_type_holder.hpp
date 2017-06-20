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
#include "../../type_constructor.hpp"
#include "detail/stored_arguments_info.hpp"
#include "type_conversion.hpp"
#include "../alignment.hpp"
#include "../utility.hpp"
#include "../../unmarshaller.hpp"
#include <cxxabi.h>
#include <type_traits>
#include <utility>

namespace mpirpc
{

namespace internal
{

namespace reconstruction
{

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

    using type_storage = typename std::aligned_storage<sizeof(std::remove_reference_t<T>),Alignment::value>::type;

    using stored = std::integer_sequence<bool, is_function_storage_duration_v<ArgumentTypes,StoredArguments,Alignments>...>;
    using stored_types = filter_tuple<stored, std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using aligned_storage_tuple_type = filter_tuple<stored, std::tuple<reconstruction_storage_aligned_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using stored_indexes = filtered_sequence_indexes_type<stored>;

    using shared_stored = std::integer_sequence<bool, is_function_group_shared_storage_duration_v<ArgumentTypes, StoredArguments, Alignments>...>;
    using shared_stored_types = filter_tuple<shared_stored, std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using shared_aligned_storage_tuple_type = filter_tuple<shared_stored, std::tuple<reconstruction_storage_aligned_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using shared_stored_indexes = filtered_sequence_indexes_type<shared_stored>;

    using individual_stored = std::integer_sequence<bool, is_function_individual_storage_duration_v<ArgumentTypes, StoredArguments, Alignments>...>;
    using individual_stored_types = filter_tuple<individual_stored, std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using individual_aligned_storage_tuple_type = filter_tuple<individual_stored, std::tuple<reconstruction_storage_aligned_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using individual_stored_indexes = filtered_sequence_indexes_type<individual_stored>;

    using manager_stored = std::integer_sequence<bool, is_manager_storage_duration_v<ArgumentTypes, StoredArguments, Alignments>...>;
    using manager_stored_types = filter_tuple<manager_stored, std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using manager_aligned_storage_tuple_type = filter_tuple<manager_stored, std::tuple<reconstruction_storage_aligned_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using manager_stored_indexes = filtered_sequence_indexes_type<manager_stored>;

    using manager_shared_stored = std::integer_sequence<bool, is_manager_storage_duration_v<ArgumentTypes, StoredArguments, Alignments>...>;
    using manager_shared_stored_types = filter_tuple<manager_shared_stored, std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using manager_shared_aligned_storage_tuple_type = filter_tuple<manager_shared_stored, std::tuple<reconstruction_storage_aligned_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using manager_shared_stored_indexes = filtered_sequence_indexes_type<manager_shared_stored>;

    using manager_individual_stored = std::integer_sequence<bool, is_manager_storage_duration_v<ArgumentTypes, StoredArguments, Alignments>...>;
    using manager_individual_stored_types = filter_tuple<manager_individual_stored, std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using manager_individual_aligned_storage_tuple_type = filter_tuple<manager_individual_stored, std::tuple<reconstruction_storage_aligned_storage_type<ArgumentTypes,StoredArguments,Alignments>...>>;
    using manager_individual_stored_indexes = filtered_sequence_indexes_type<manager_individual_stored>;

    static constexpr std::size_t stored_count = std::tuple_size<stored_types>::value;
    using storage_types = std::tuple<reconstruction_storage_type<ArgumentTypes,StoredArguments,Alignments>...>;
    using storage_construction_types = std::tuple<reconstruction_storage_constructor_type<ArgumentTypes,StoredArguments,Alignments>...>;
    using proxy_types = std::tuple<ConstructorArgumentTypes&&...>;

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
    using index_to_individual_storage_index = std::tuple_element_t<I, individual_stored_indexes>;

    template<std::size_t I>
    static constexpr std::size_t index_to_storage_index_v = index_to_storage_index<I>::value;

    template<std::size_t I>
    static constexpr bool is_stored_v = integer_sequence_element_v<I,stored>;

    template<std::size_t I>
    static constexpr bool index_to_individual_storage_index_v = index_to_individual_storage_index<I>::value;

    template<std::size_t I>
    static constexpr bool is_individual_stored_v = integer_sequence_element_v<I,individual_stored>;

    template<std::size_t I>
    static constexpr bool index_to_shared_storage_index_v = std::tuple_element_t<I, shared_stored_indexes>::value;

    template<std::size_t I>
    static constexpr bool is_shared_stored_v = integer_sequence_element_v<I, shared_stored>;

protected:

    template<std::size_t I,
             std::enable_if_t<
                is_individual_stored_v<I> && is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(individual_aligned_storage_tuple_type& stored_args, shared_aligned_storage_tuple_type&, arguments_tuple_type& args)
    {
        using arg_type = argument_at_index<I>;
        using con_type = storage_constructor_type_at_index<I>;
        constexpr std::size_t SI = index_to_individual_storage_index_v<I>;
        con_type *storage = reinterpret_cast<con_type*>(&std::get<SI>(stored_args));
        new (storage) con_type{std::get<I>(args)};
        return static_cast<proxy_type<I>>(storage->value());
    }

    template<std::size_t I,
             std::enable_if_t<
                is_shared_stored_v<I> && is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(individual_aligned_storage_tuple_type&, shared_aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        using arg_type = argument_at_index<I>;
        using con_type = storage_constructor_type_at_index<I>;
        constexpr std::size_t SI = index_to_shared_storage_index_v<I>;
        con_type *storage = reinterpret_cast<con_type*>(&std::get<SI>(stored_args));
        new (storage) con_type{std::get<I>(args)};
        return static_cast<proxy_type<I>>(storage->value());
    }

    template<std::size_t I,
             std::enable_if_t<
                is_stored_v<I> && is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        using arg_type = argument_at_index<I>;
        using con_type = storage_constructor_type_at_index<I>;
        constexpr std::size_t SI = index_to_storage_index_v<I>;
        con_type *storage = reinterpret_cast<con_type*>(&std::get<SI>(stored_args));
        new (storage) con_type{std::get<I>(args)};
        return static_cast<proxy_type<I>>(storage->value());
    }




    template<std::size_t I, bool Initial,
             std::enable_if_t<
                is_individual_stored_v<I> && !is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(individual_aligned_storage_tuple_type& stored_args, shared_aligned_storage_tuple_type&, arguments_tuple_type& args)
    {
        constexpr std::size_t SI = index_to_individual_storage_index_v<I>;
        using arg_type = argument_at_index<I>;
        auto arg = std::get<I>(args);
        std::cout << "arg: " <<  arg << std::endl;
        std::remove_cv_t<std::remove_reference_t<arg_type>>* tmp = reinterpret_cast<std::remove_cv_t<std::remove_reference_t<arg_type>>*>(&std::get<SI>(stored_args));
        std::cout <<  abi::__cxa_demangle(typeid(std::tuple<decltype(tmp)>).name(),0,0,0) <<  std::endl;
        (void)type_constructor<std::remove_cv_t<std::remove_reference_t<arg_type>>>{reinterpret_cast<std::remove_cv_t<std::remove_reference_t<arg_type>>*>(&std::get<SI>(stored_args)), arg};
        std::cout << I <<  " " << abi::__cxa_demangle(typeid(std::tuple<arg_type>).name(),0,0,0) <<  " " << abi::__cxa_demangle(typeid(std::tuple<proxy_type<I>>).name(),0,0,0) <<  " " << abi::__cxa_demangle(typeid(std::tuple<std::remove_cv_t<std::remove_reference_t<arg_type>>>).name(),0,0,0) << std::endl;

        std::cout << *tmp <<  std::endl;
        return static_cast<proxy_type<I>>(*tmp);
    }

    template<std::size_t I, bool Initial,
             std::enable_if_t<
                is_shared_stored_v<I> && !is_construction_info_v<argument_at_index<I>>
              >* = nullptr,
              std::enable_if_t<Initial>* = nullptr>
    static proxy_type<I> construct(individual_aligned_storage_tuple_type&, shared_aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        constexpr std::size_t SI = index_to_shared_storage_index_v<I>;
        using arg_type = argument_at_index<I>;
        (void)type_constructor<std::remove_cv_t<std::remove_reference_t<arg_type>>>{reinterpret_cast<std::remove_cv_t<std::remove_reference_t<arg_type>>*>(&std::get<SI>(stored_args)), std::get<I>(args)};
        std::cout << "Initial: " << I <<  " " << abi::__cxa_demangle(typeid(std::tuple<arg_type>).name(),0,0,0) <<  " " << abi::__cxa_demangle(typeid(std::tuple<proxy_type<I>>).name(),0,0,0) << std::endl;
        return reinterpret_cast<proxy_type<I>>(std::get<SI>(stored_args));
    }

    template<std::size_t I, bool Initial,
             std::enable_if_t<
                is_shared_stored_v<I> && !is_construction_info_v<argument_at_index<I>>
              >* = nullptr,
              std::enable_if_t<!Initial>* = nullptr>
    static proxy_type<I> construct(individual_aligned_storage_tuple_type&, shared_aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        constexpr std::size_t SI = index_to_shared_storage_index_v<I>;
        using arg_type = argument_at_index<I>;
        std::cout << "Not initial: " << I <<  " " << abi::__cxa_demangle(typeid(std::tuple<arg_type>).name(),0,0,0) <<  " " << abi::__cxa_demangle(typeid(std::tuple<proxy_type<I>>).name(),0,0,0) << std::endl;
        return reinterpret_cast<proxy_type<I>>(std::get<SI>(stored_args));
    }

    template<std::size_t I,
             std::enable_if_t<
                is_stored_v<I> && !is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(aligned_storage_tuple_type& stored_args, arguments_tuple_type& args)
    {
        constexpr std::size_t SI = index_to_storage_index_v<I>;;
        using arg_type = argument_at_index<I>;
        (void)type_constructor<std::remove_cv_t<std::remove_reference_t<arg_type>>>{reinterpret_cast<std::remove_cv_t<std::remove_reference_t<arg_type>>*>(&std::get<SI>(stored_args)), std::get<I>(args)};
        std::cout << I <<  " " << abi::__cxa_demangle(typeid(std::tuple<arg_type>).name(),0,0,0) <<  " " << abi::__cxa_demangle(typeid(std::tuple<proxy_type<I>>).name(),0,0,0) << std::endl;
        return reinterpret_cast<proxy_type<I>>(std::get<SI>(stored_args));
    }




    template<std::size_t I, bool Initial, typename IAST, typename SAST, std::enable_if_t<!is_stored_v<I>>* = nullptr>
    static proxy_type<I> construct(IAST&&, SAST&&, arguments_tuple_type& args)
    {
        construct<I>(nullptr, args);
    }

    //do not store and is_construction_info true
    template<std::size_t I, typename AST,
             std::enable_if_t<
                !is_stored_v<I> && is_construction_info_v<argument_at_index<I>>
               >* = nullptr>
    static decltype(auto) construct(AST&&, arguments_tuple_type& args)
    {
        //std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<I,proxy_tuple_type>>>;
        //using arg_type = stored_type<index_to_storage_index_v<I>>;
        using con_type = storage_constructor_type_at_index<I>;
        std::cout << abi::__cxa_demangle(typeid(con_type).name(),0,0,0) << std::endl;
        return con_type::construct(std::get<I>(args));//tuple_construct<arg_type>(std::get<I>(args).args());
    }

    template<std::size_t I, typename AST,
             std::enable_if_t<
                !is_stored_v<I> && !is_construction_info_v<argument_at_index<I>>
              >* = nullptr>
    static proxy_type<I> construct(AST&&, arguments_tuple_type& args)
    {
        using con_type = storage_constructor_type_at_index<I>;
        std::cout << I <<  " " << abi::__cxa_demangle(typeid(std::tuple<proxy_type<I>>).name(),0,0,0) << std::endl;
        return reinterpret_cast<proxy_type<I>>(std::get<I>(args));
    }

    template<std::size_t... Is>
    static void construct(std::remove_cv_t<std::remove_reference_t<T>> *val, aligned_storage_tuple_type &stored_args, arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        type_constructor<std::remove_cv_t<std::remove_reference_t<T>>>{reinterpret_cast<std::remove_cv_t<std::remove_reference_t<T>>*>(val),aligned_type_holder::construct<Is>(stored_args, t)...};
    }

    template<bool B, std::size_t... Is>
    static void construct(std::remove_cv_t<std::remove_reference_t<T>> *val, individual_aligned_storage_tuple_type& individual_stored_args, shared_aligned_storage_tuple_type &shared_stored_args, arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        type_constructor<std::remove_cv_t<std::remove_reference_t<T>>> a{reinterpret_cast<std::remove_cv_t<std::remove_reference_t<T>>*>(val),aligned_type_holder::construct<Is, B>(individual_stored_args, shared_stored_args, t)...};
        std::cout << (void*) val <<  " " << val->a() << " " << val->b() << std::endl;
    }

    /*template<typename U, typename... Args, std::size_t... Is>
    static U tuple_construct(const std::tuple<Args...>& args, std::index_sequence<Is...>)
    {
        return U(std::get<Is>(args)...);
    }

    template<typename U, typename... Args>
    static U tuple_construct(const std::tuple<Args...>& args)
    {
        return tuple_construct<U>(args,std::make_index_sequence<sizeof...(Args)>{});
    } */

    template<std::size_t I>
    static void destruct_stored_arg(aligned_storage_tuple_type& storage_tuple)
    {
        using storage_type = storage_type_at_storage_index<I>;
        //std::cout << "Reverse indexes: " << abi::__cxa_demangle(typeid(storage_tuple_types).name(),0,0,0) << std::endl;
        //std::cout << abi::__cxa_demangle(typeid(argument<I>(storage_tuple)).name(),0,0,0) << " | " << abi::__cxa_demangle(typeid(storage_type).name(),0,0,0) << std::endl;
        argument<I>(storage_tuple).~storage_type();
    }

    template<typename U>
    static void destruct(U& t)
    {
        t.~U();
    }

    template<std::size_t... Is>
    static void destruct_stored_args(aligned_storage_tuple_type& storage_tuple, std::index_sequence<Is...>)
    {
        (void)swallow{(destruct_stored_arg<Is>(storage_tuple), 0)...};
    }

    template<std::size_t... Is>
    static void destruct_shared_stored_args(shared_aligned_storage_tuple_type& storage_tuple, std::index_sequence<Is...>)
    {
        (void)swallow{(destruct(reinterpret_cast<std::tuple_element_t<Is, shared_stored_types>&>(std::get<Is>(storage_tuple))), 0)...};
    }

    template<std::size_t... Is>
    static void destruct_individual_stored_args(individual_aligned_storage_tuple_type& storage_tuple, std::index_sequence<Is...>)
    {
        (void)swallow{(destruct(reinterpret_cast<std::tuple_element_t<Is, individual_stored_types>&>(std::get<Is>(storage_tuple))), 0)...};
    }

public:

    template<std::size_t I>
    static storage_type_at_storage_index<I>& argument(aligned_storage_tuple_type& storage_tuple)
    {
        using arg_type = storage_type_at_storage_index<I>;
        return reinterpret_cast<arg_type&>(std::get<I>(storage_tuple));
    }

    static void destruct_stored_args(aligned_storage_tuple_type& storage_tuple)
    {
        destruct_stored_args(storage_tuple, std::make_index_sequence<stored_count>{});
    }

    static void destruct_shared_stored_args(shared_aligned_storage_tuple_type& storage_tuple)
    {
        destruct_shared_stored_args(storage_tuple, std::make_index_sequence<std::tuple_size<shared_aligned_storage_tuple_type>::value>{});
    }

    static void destruct_individual_stored_args(individual_aligned_storage_tuple_type& storage_tuple)
    {
        destruct_individual_stored_args(storage_tuple, std::make_index_sequence<std::tuple_size<individual_aligned_storage_tuple_type>::value>{});
    }

    static void construct(std::remove_cv_t<std::remove_reference_t<T>> *val, aligned_storage_tuple_type &stored_args, arguments_tuple_type& t)
    {
        construct(val, stored_args, t, std::make_index_sequence<std::tuple_size<arguments_tuple_type>::value>());
    }

    template<bool B>
    static void construct(std::remove_cv_t<std::remove_reference_t<T>> *val, individual_aligned_storage_tuple_type& individual_stored_args, shared_aligned_storage_tuple_type &shared_stored_args, arguments_tuple_type& t)
    {
        construct<B>(val, individual_stored_args, shared_stored_args, t, std::make_index_sequence<std::tuple_size<arguments_tuple_type>::value>{});
    }

    aligned_type_holder(arguments_tuple_type& t)
        : m_val{}, m_stored_args{}
    {
        construct(reinterpret_cast<std::remove_cv_t<std::remove_reference_t<T>>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }

    aligned_type_holder(construction_info_type& t) : aligned_type_holder(t.args()) {}

    aligned_type_holder()
        : m_val{}, m_stored_args{}
    {}

    void construct(arguments_tuple_type& t)
    {
        construct(reinterpret_cast<std::remove_cv_t<std::remove_reference_t<T>>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }

    T& value() { return reinterpret_cast<T&>(m_val); }
    const T& value() const { return reinterpret_cast<const T&>(m_val); }

    template<std::size_t I>
    storage_type_at_storage_index<I>& argument() { return argument<I>(m_stored_args); }

    template<std::size_t I>
    const storage_type_at_storage_index<I>& argument() const { return argument<I>(m_stored_args); }

    ~aligned_type_holder()
    {
        using nonref_type = std::remove_reference_t<T>;
        reinterpret_cast<nonref_type*>(&m_val)->~nonref_type();
        destruct_stored_args(m_stored_args);
    }

private:
    type_storage m_val;
    aligned_storage_tuple_type m_stored_args;
};

template<typename T, std::size_t N, typename Alignment, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T[N],Alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
public:
    using base_type = T;
    using base_type_holder = aligned_type_holder<T, Alignment, std::tuple<std::add_lvalue_reference_t<std::remove_extent_t<std::remove_reference_t<ConstructorArgumentTypes>>>...>, std::tuple<std::add_lvalue_reference_t<std::remove_extent_t<std::remove_reference_t<ArgumentTypes>>>...>, std::tuple<StoredArguments...>, std::tuple<Alignments...>>;
    using aligned_storage_tuple_type = typename base_type_holder::aligned_storage_tuple_type[N];
    using type_storage = typename base_type_holder::type_storage[N];
};

template<typename T, typename Alignment, typename... ConstructorArgumentTypes, typename... ArgumentTypes, typename... StoredArguments, typename... Alignments>
class aligned_type_holder<T[],Alignment,std::tuple<ConstructorArgumentTypes...>,std::tuple<ArgumentTypes...>,std::tuple<StoredArguments...>,std::tuple<Alignments...>>
{
};

}

}

template<typename T, typename S, std::size_t N>
class unmarshalled_array_holder<T,S,N,true>
{
public:
    unmarshalled_array_holder(T&& ptr, S&& stor)
        : m_ptr{std::move(ptr)}, m_stor_ptr{std::move(stor)} {}

    constexpr std::size_t size() const { return N; }
    T&& pointer() { return std::move(m_ptr); }
    S&& storage() { return std::move(m_stor_ptr); }
    decltype(auto) operator[] (std::size_t index) { return m_ptr[index]; }

private:
    T m_ptr;
    S m_stor_ptr;
};

template<typename T, typename S>
class unmarshalled_array_holder<T,S,0,true>
{
public:
    unmarshalled_array_holder(std::size_t size, T&& ptr, S&& stor)
        : m_size{size}, m_ptr{std::move(ptr)}, m_stor_ptr{std::move(stor)} {}

    const std::size_t size() const { return m_size; }
    T&& pointer() { return std::move(m_ptr); }
    S&& storage() { return std::move(m_stor_ptr); }
    decltype(auto) operator[] (std::size_t index) { return m_ptr[index]; }

private:
    const std::size_t m_size;
    T m_ptr;
    S m_stor_ptr;
};

template<typename T, typename S>
auto make_unmarshalled_array_holder(std::size_t size, T&& t, S&& s)
    -> unmarshalled_array_holder<T,S,0,true>
{
    std::cout << "making array holder with storage" << std::endl;
    return {size, std::move(t), std::move(s)};
}

template<std::size_t N, typename T, typename S>
auto make_unmarshalled_array_holder(T&& t, S&& s)
    -> unmarshalled_array_holder<T,S,N,true>
{
    std::cout << "making array holder with storage" << std::endl;
    return {std::move(t), std::move(s)};
}

template<typename Buffer, typename Alignment, typename Options, typename T, std::size_t N>
struct unmarshaller<T[N],Buffer,Alignment,Options,
    std::enable_if_t<
        is_buildtype_v<std::remove_all_extents_t<T>,Buffer> &&
        is_construction_info_v<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>>
      >
  >
{
    // unmarshaller type
    using UT = unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>;

    // aligned type holder
    using ATH = typename UT::template aligned_type_holder<Alignment>;

    // real type
    using RT = typename ATH::type;

    // aligned storage tuple type
    using IASTT = typename ATH::individual_aligned_storage_tuple_type;
    using SASTT = typename ATH::shared_aligned_storage_tuple_type;

    // unique_ptr for the actual type data
    using unique_ptr_type = std::unique_ptr<internal::retype_array_type<T, RT>[], std::function<void(internal::retype_array_type<T, RT>*)>>;

    // unique_ptr for the stored arguments data
    //using individual_stored_data_unique_ptr_type = std::unique_ptr<internal::retype_array_type<T, IASTT>, std::function<void(internal::retype_array_type<T, IASTT>*)>>;

    template<typename Allocator, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        internal::retype_array_type<T, RT>* ad_ptr;
        internal::retype_array_type<T, IASTT>* id_ptr;
        SASTT* sd_ptr;
        constexpr std::size_t a_alignment = std::max(internal::alignment_reader<Alignment>::value,alignof(void*));
        constexpr std::size_t i_alignment = alignof(IASTT);
        constexpr std::size_t s_alignment = alignof(SASTT);
        constexpr std::size_t buffer_alignment = std::max(a_alignment, std::max(s_alignment, i_alignment));
        constexpr std::size_t a_size = N*sizeof(internal::retype_array_type<T, RT>);
        constexpr std::size_t i_size = N*sizeof(internal::retype_array_type<T, IASTT>);
        constexpr std::size_t s_size = sizeof(SASTT);
        constexpr std::size_t buffer_size = a_size + i_size + s_size + sizeof(internal::retype_array_type<T, IASTT>) + sizeof(SASTT);
        void *buffer;
        size_t sz = buffer_size;

        posix_memalign((void**) &buffer , buffer_alignment, buffer_size);
        ad_ptr = reinterpret_cast<internal::retype_array_type<T, RT>*>(buffer);
        sz -= a_size;
        id_ptr = reinterpret_cast<internal::retype_array_type<T, IASTT>*>(ad_ptr+N);
        std::align(i_alignment, i_size, id_ptr, sz);
        sz -= i_size;
        sd_ptr = static_cast<SASTT*>(id_ptr+N);
        std::align(s_alignment, s_size, sd_ptr, sz);


        auto type_array = std::unique_ptr<internal::retype_array_type<T,RT>[],std::function<void(internal::retype_array_type<T,RT>*)>>{ad_ptr, [=](internal::retype_array_type<T,RT>* p) {
            for (std::size_t i = 0; i < N; ++i)
                p[i].~RT();
            ATH::destruct_shared_stored_args(*sd_ptr);
            for (std::size_t i = 0; i < N; ++i)
                ATH::destruct_individual_stored_args(id_ptr[N]);

            free(p);
            std::cout << "freed p" << std::endl;
        }};

        ATH::template construct<true>(&type_array[0], id_ptr[0], *sd_ptr, unmarshaller<T,Buffer,Alignment,Options>::unmarshall(a,b).args());
        for (std::size_t i = 1; i < N; ++i)
            ATH::template construct<false>(&type_array[i], id_ptr[i], *sd_ptr, unmarshaller<T,Buffer,Alignment,Options>::unmarshall(a,b).args());
        return make_unmarshalled_array_holder<N>(std::move(type_array));
    }

    template<bool First, typename Allocator, typename U = T,
             std::enable_if_t<std::rank<U>::value == 0>* = nullptr,
             std::enable_if_t<First>* = nullptr>
    static void unmarshal_into(Allocator&& a, Buffer& b, RT(&v)[N], IASTT(&i)[N],  SASTT& s)
    {
        ATH::template construct<true>(&v[0], i[0], s, mpirpc::get<T>(b,a).args());
        for (std::size_t j = 1; j < N; ++j)
            ATH::template construct<false>(&v[j], i[j], s, mpirpc::get<T>(b,a).args());
    }

    template<bool First, typename Allocator, typename U = T,
             std::enable_if_t<std::rank<U>::value == 0>* = nullptr,
             std::enable_if_t<!First>* = nullptr>
    static void unmarshal_into(Allocator&& a, Buffer& b, RT(&v)[N], IASTT(&i)[N],  SASTT& s)
    {
        for (std::size_t j = 0; j < N; ++j) {
            std::cout <<  (void*) &i[j] << " " << (void*) &s << std::endl;
            ATH::template construct<false>(&v[j], i[j], s, mpirpc::get<T>(b,a).args());
        }
    }

    template<bool First, typename Allocator, typename U = T, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static void unmarshal_into(Allocator&& a, Buffer& b, RT(&v)[N], IASTT(&i)[N], SASTT& s)
    {
        for (std::size_t j = 0; j < N; ++j)
            unmarshaller<T,Buffer,Alignment,Options>::template unmarshal_into<First>(a,b,v,i[j],s);
    }

    template<typename U, std::size_t M, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static void destruct(U(&v)[M])
    {
        for (std::size_t i = 0; i < M; ++i)
            v[i].~U();
    }

    template<typename U, std::size_t M, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static void destruct(U(&v)[M])
    {
        for (std::size_t i = 0; i < M; ++i)
                unmarshaller<T,Buffer,Alignment,Options>::destruct(v[i]);
    }

    template<typename U, std::size_t M, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static void destruct_stored_args(U(&v)[M])
    {
        for (std::size_t i = 0; i < M; ++i)
            ATH::destruct_individual_stored_args(v[i]);
    }

    template<typename U, std::size_t M, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static void destruct_stored_args(U(&v)[M])
    {
        for (std::size_t i = 0; i < M; ++i)
            unmarshaller<T,Buffer,Alignment,Options>::destruct_stored_args(v[i]);
    }

    template<typename S, typename U = T, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static void destruct_shared_stored_args(S&& s)
    {
        unmarshaller<U,Buffer,Alignment,Options>::destruct_shared_stored_args(s);
    }

    template<typename S, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static void destruct_shared_stored_args(S&& s)
    {
        ATH::destruct_shared_stored_args(s);
    }

    template<typename Allocator, typename U = T, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        internal::retype_array_type<T, RT>* ad_ptr;
        internal::retype_array_type<T, IASTT>* id_ptr;
        SASTT* sd_ptr;
        constexpr std::size_t a_alignment = std::max(internal::alignment_reader<Alignment>::value,alignof(void*));
        constexpr std::size_t i_alignment = alignof(IASTT);
        constexpr std::size_t s_alignment = alignof(SASTT);
        constexpr std::size_t buffer_alignment = std::max(a_alignment, std::max(s_alignment, i_alignment));
        constexpr std::size_t a_size = N*sizeof(internal::retype_array_type<T, RT>);
        constexpr std::size_t i_size = N*sizeof(internal::retype_array_type<T, IASTT>);
        constexpr std::size_t s_size = sizeof(SASTT);
        constexpr std::size_t buffer_size = a_size + i_size + s_size + sizeof(internal::retype_array_type<T, IASTT>) + sizeof(SASTT);
        void *buffer;
        size_t sz = buffer_size;

        posix_memalign((void**) &buffer , buffer_alignment, buffer_size);
        ad_ptr = static_cast<internal::retype_array_type<T, RT>*>(buffer);
        sz -= a_size;
        void* id_ptr_voidp = reinterpret_cast<void*>(&ad_ptr[N]);
        if (!std::align(i_alignment, i_size, id_ptr_voidp, sz))
            std::cout << "error 1" << std::endl;
        id_ptr = reinterpret_cast<internal::retype_array_type<T, IASTT>*>(id_ptr_voidp);
        sz -= i_size;
        void* sd_ptr_voidp = reinterpret_cast<void*>(&id_ptr[N]);
        if (!std::align(s_alignment, s_size, sd_ptr_voidp, sz))
            std::cout << "error 2" << std::endl;
        sd_ptr = reinterpret_cast<SASTT*>(sd_ptr_voidp);
        //id_ptr = new internal::retype_array_type<T, IASTT>[N];
        //sd_ptr = new SASTT();
        std::cout <<  buffer <<  " " << ad_ptr <<  " " << id_ptr << " " << sd_ptr <<  std::endl;

        auto type_array = std::unique_ptr<internal::retype_array_type<T,RT>[],std::function<void(internal::retype_array_type<T,RT>*)>>{ad_ptr, [=](internal::retype_array_type<T,RT>* p) {

            //for (std::size_t i = 0; i < N; ++i)
            //    unmarshaller<T,Buffer,Alignment,Options>::destruct(p[i]);
            //unmarshaller<T, Buffer, Alignment,Options>::destruct_shared_stored_args(*sd_ptr);
            //for (std::size_t i = 0; i < N; ++i)
            //    unmarshaller<T, Buffer, Alignment,Options>::destruct_stored_args(id_ptr[i]);
            std::cout <<  buffer <<  " " << ad_ptr <<  " " << id_ptr << " " << sd_ptr <<  std::endl;
            free(p);
            std::cout << "freed p" << std::endl;
        }};

        unmarshaller<T,Buffer,Alignment,Options>::template unmarshal_into<true>(a,b,ad_ptr[0],id_ptr[0], *sd_ptr);
        //unmarshaller<T,Buffer,Alignment,Options>::template unmarshal_into<false>(a,b,ad_ptr[0],id_ptr[0], *sd_ptr);
        for (std::size_t i = 1; i < N; ++i)
            unmarshaller<T,Buffer,Alignment,Options>::template unmarshal_into<false>(a,b,ad_ptr[i],id_ptr[i], *sd_ptr);
        return make_unmarshalled_array_holder<N>(std::move(type_array));
    }
};

template<typename Buffer, typename Alignment, typename Options, typename T>
struct unmarshaller<T[],Buffer,Alignment,Options,
    std::enable_if_t<
        is_buildtype_v<std::remove_all_extents_t<T>,Buffer> &&
        is_construction_info_v<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>>
      >
  >
{
    // unmarshaller type
    using UT = unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>;

    // aligned type holder
    using ATH = typename UT::template aligned_type_holder<Alignment>;

    // real type
    using RT = typename ATH::type;

    // aligned storage tuple type
    using IASTT = typename ATH::individual_aligned_storage_tuple_type;
    using SASTT = typename ATH::shared_aligned_storage_tuple_type;

    template<typename Allocator, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        internal::retype_array_type<T, RT>* ad_ptr;
        internal::retype_array_type<T, IASTT>* id_ptr;
        SASTT* sd_ptr;
        constexpr std::size_t a_alignment = std::max(internal::alignment_reader<Alignment>::value,alignof(void*));
        constexpr std::size_t i_alignment = alignof(IASTT);
        constexpr std::size_t s_alignment = alignof(SASTT);
        constexpr std::size_t buffer_alignment = std::max(a_alignment, std::max(s_alignment, i_alignment));
        std::size_t a_size = size*sizeof(internal::retype_array_type<T, RT>);
        std::size_t i_size = size*sizeof(internal::retype_array_type<T, IASTT>);
        std::size_t s_size = sizeof(SASTT);
        std::size_t buffer_size = a_size + i_size + s_size + sizeof(internal::retype_array_type<T, IASTT>) + sizeof(SASTT);
        void *buffer;
        size_t sz = buffer_size;

        posix_memalign((void**) &buffer , buffer_alignment, buffer_size);
        ad_ptr = static_cast<internal::retype_array_type<T, RT>*>(buffer);
        sz -= a_size;
        void* id_ptr_voidp = reinterpret_cast<void*>(ad_ptr+size);
        std::align(i_alignment, i_size, id_ptr_voidp, sz);
        id_ptr = reinterpret_cast<internal::retype_array_type<T, IASTT>*>(id_ptr_voidp);
        sz -= i_size;
        void* sd_ptr_voidp = reinterpret_cast<void*>(id_ptr+size);
        std::align(s_alignment, s_size, sd_ptr_voidp, sz);
        sd_ptr = reinterpret_cast<SASTT*>(sd_ptr_voidp);
        std::cout <<  "here1" <<  std::endl;

        auto type_array = std::unique_ptr<internal::retype_array_type<T,RT>[],std::function<void(internal::retype_array_type<T,RT>*)>>{ad_ptr, [=](internal::retype_array_type<T,RT>* p) {
            unmarshaller<T, Buffer, Alignment,Options>::destruct_shared_stored_args(*sd_ptr);
            for (std::size_t i = 0; i < size; ++i)
                unmarshaller<T,Buffer,Alignment,Options>::destruct(p[i]);
            for (std::size_t i = 0; i < size; ++i)
                unmarshaller<T, Buffer, Alignment,Options>::destruct_stored_args(id_ptr[i]);

            free(p);
            std::cout << "freed p" << std::endl;
        }};

        ATH::template construct<true>(&type_array[0], id_ptr[0], *sd_ptr, unmarshaller<T,Buffer,Alignment,Options>::unmarshall(a,b).args());
        for (std::size_t i = 1; i < size; ++i)
            ATH::template construct<false>(&type_array[i], id_ptr[i], *sd_ptr, unmarshaller<T,Buffer,Alignment,Options>::unmarshall(a,b).args());
        return make_unmarshalled_array_holder(size, std::move(type_array));
    }

    template<typename Allocator, typename U = T, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        internal::retype_array_type<T, RT>* ad_ptr;
        internal::retype_array_type<T, IASTT>* id_ptr;
        SASTT* sd_ptr;
        constexpr std::size_t a_alignment = std::max(internal::alignment_reader<Alignment>::value,alignof(void*));
        constexpr std::size_t i_alignment = alignof(IASTT);
        constexpr std::size_t s_alignment = alignof(SASTT);
        constexpr std::size_t buffer_alignment = std::max(a_alignment, std::max(s_alignment, i_alignment));
        std::size_t a_size = size*sizeof(internal::retype_array_type<T, RT>);
        std::size_t i_size = size*sizeof(internal::retype_array_type<T, IASTT>);
        std::size_t s_size = sizeof(SASTT);
        std::size_t buffer_size = a_size + i_size + s_size + sizeof(internal::retype_array_type<T, IASTT>) + sizeof(SASTT);
        void *buffer;
        size_t sz = buffer_size;

        posix_memalign((void**) &buffer , buffer_alignment, buffer_size);
        ad_ptr = static_cast<internal::retype_array_type<T, RT>*>(buffer);
        sz -= a_size;
        void* id_ptr_voidp = reinterpret_cast<void*>(ad_ptr+size);
        std::align(i_alignment, i_size, id_ptr_voidp, sz);
        id_ptr = reinterpret_cast<internal::retype_array_type<T, IASTT>*>(id_ptr_voidp);
        sz -= i_size;
        void* sd_ptr_voidp = reinterpret_cast<void*>(id_ptr+size);
        std::align(s_alignment, s_size, sd_ptr_voidp, sz);
        sd_ptr = reinterpret_cast<SASTT*>(sd_ptr_voidp);
        std::cout <<  "here2" <<  std::endl;

        auto type_array = std::unique_ptr<internal::retype_array_type<T,RT>[],std::function<void(internal::retype_array_type<T,RT>*)>>{ad_ptr, [=](internal::retype_array_type<T,RT>* p) {
            unmarshaller<T, Buffer, Alignment,Options>::destruct_shared_stored_args(*sd_ptr);
            for (std::size_t i = 0; i < size; ++i)
                unmarshaller<T,Buffer,Alignment,Options>::destruct(p[i]);
            for (std::size_t i = 0; i < size; ++i)
                unmarshaller<T, Buffer, Alignment,Options>::destruct_stored_args(id_ptr[i]);

            free(p);
            std::cout << "freed p" << std::endl;
        }};

        unmarshaller<T,Buffer,Alignment,Options>::template unmarshal_into<true>(a,b,type_array[0],id_ptr[0], *sd_ptr);
        for (std::size_t i = 1; i < size; ++i)
            unmarshaller<T,Buffer,Alignment,Options>::template unmarshal_into<false>(a,b,type_array[i],id_ptr[i], *sd_ptr);
        return make_unmarshalled_array_holder(size, std::move(type_array));
    }
};

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNED_TYPE_HOLDER_HPP */
