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
    static void construct(std::remove_cv_t<std::remove_reference_t<T>> *val, aligned_storage_tuple_type &stored_args, arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        type_constructor<std::remove_cv_t<std::remove_reference_t<T>>>::construct(reinterpret_cast<std::remove_cv_t<std::remove_reference_t<T>>*>(val),aligned_type_holder::construct<Is>(stored_args, t)...);
    }

    template<std::size_t... Is>
    static std::remove_reference_t<T> construct(arguments_tuple_type& t, std::index_sequence<Is...>)
    {
        aligned_storage_tuple_type stored_args;
        std::remove_reference_t<T> ret{aligned_type_holder::construct<Is>(stored_args, t)...};
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

    static std::remove_reference_t<T> construct(const construction_info_type& t)
    {
        return construct(t.args(),std::make_index_sequence<construction_info_type::num_args()>{});
    }

    static void destruct_stored_args(aligned_storage_tuple_type& storage_tuple)
    {
        destruct_stored_args(storage_tuple, std::make_index_sequence<stored_count>{});
    }

    aligned_type_holder(arguments_tuple_type& t)
        : m_val{}, m_stored_args{}
    {
        construct(reinterpret_cast<std::remove_cv_t<std::remove_reference_t<T>>*>(&m_val), m_stored_args, t, std::make_index_sequence<sizeof...(ArgumentTypes)>{});
    }

    aligned_type_holder(construction_info_type& t) : aligned_type_holder(t.args())
    {}

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

template<typename Buffer, typename Alignment, typename T, std::size_t N>
struct unmarshaller<T[N],Buffer,Alignment,
    std::enable_if_t<
        is_buildtype_v<std::remove_all_extents_t<T>,Buffer> &&
        is_construction_info_v<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>>
      >
  >
{
    using UT = unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>;
    using unique_ptr_type = std::unique_ptr<internal::retype_array_type<T,UT>[],std::function<void(internal::retype_array_type<T,UT>*)>>;

    template<typename Allocator, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        internal::retype_array_type<T,UT>* ptr;
        posix_memalign((void**) &ptr, internal::alignment_reader<Alignment>::value, N*sizeof(internal::retype_array_type<T,UT>));

        auto ret = std::unique_ptr<internal::retype_array_type<T,UT>[],std::function<void(internal::retype_array_type<T,UT>*)>>{ptr, [](internal::retype_array_type<T,UT>* p) {
            for (std::size_t i = 0; i < N; ++i)
                p[i].~UT();
            free(p);
            std::cout << "freed p" << std::endl;
        }};
        for (std::size_t i = 0; i < N; ++i)
            new (&ret[i]) UT(unmarshaller<T,Buffer,Alignment>::unmarshall(a,b));
        return make_unmarshalled_array_holder<N>(std::move(ret));
    }

    template<typename Allocator, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static void unmarshal_into(Allocator&& a, Buffer& b, UT(&v)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
            new (&v[i]) UT(mpirpc::get<T>(b,a));
    }

    template<typename Allocator, typename U = T, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static void unmarshal_into(Allocator&& a, Buffer& b, UT(&v)[N])
    {
        for (std::size_t i = 0; i < N; ++i)
            unmarshaller<T,Buffer,Alignment>::unmarshal_into(a,b,v);
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
                unmarshaller<T,Buffer,Alignment>::destruct(v[i]);
    }

    template<typename Allocator, typename U = T, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        //std::cout << abi::__cxa_demangle(typeid(ret2).name(),0,0,0) << std::endl;
        internal::retype_array_type<T,UT>* ptr;
        constexpr std::size_t alignment = std::max(internal::alignment_reader<Alignment>::value,alignof(void*));
        posix_memalign((void**) &ptr, alignment, N*sizeof(internal::retype_array_type<T,UT>));

        auto ret = std::unique_ptr<internal::retype_array_type<T,UT>[],std::function<void(internal::retype_array_type<T,UT>*)>>{ptr, [](internal::retype_array_type<T,UT>* p) {
            for (std::size_t i = 0; i < N; ++i)
                unmarshaller<T,Buffer,Alignment>::destruct(p[i]);
            free(p);
            std::cout << "freed p" << std::endl;
        }};
        //std::vector<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>> ret;
        for (std::size_t i = 0; i < N; ++i)
            unmarshaller<T,Buffer,Alignment>::unmarshal_into(a,b,ret[i]);
        return make_unmarshalled_array_holder<N>(std::move(ret));
    }
};

template<typename Buffer, typename Alignment, typename T>
struct unmarshaller<T[],Buffer,Alignment,
    std::enable_if_t<
        is_buildtype_v<std::remove_all_extents_t<T>,Buffer> &&
        is_construction_info_v<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>>
      >
  >
{
    using UT = unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>;
    using unique_ptr_type = std::unique_ptr<internal::retype_array_type<T,UT>[],std::function<void(internal::retype_array_type<T,UT>*)>>;

    template<typename Allocator, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        internal::retype_array_type<T,UT>* ptr;
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        posix_memalign((void**) &ptr, internal::alignment_reader<Alignment>::value, size*sizeof(internal::retype_array_type<T,UT>));

        unique_ptr_type ret{ptr, [=](internal::retype_array_type<T,UT>* p) {
            for (std::size_t i = 0; i < size; ++i)
                p[i].~UT();
            free(p);
            std::cout << "freed p" << std::endl;
        }};

        for (std::size_t i = 0; i < size; ++i)
            new (&ret[i]) UT(mpirpc::get<T>(b,a));
        return make_unmarshalled_array_holder(size,std::move(ret));
    }

    template<typename Allocator, typename U = T, std::enable_if_t<(std::rank<U>::value > 0)>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        //std::cout << abi::__cxa_demangle(typeid(ret2).name(),0,0,0) << std::endl;
        internal::retype_array_type<T,UT>* ptr;
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        constexpr std::size_t alignment = std::max(internal::alignment_reader<Alignment>::value,alignof(void*));
        posix_memalign((void**) &ptr, alignment, size*sizeof(internal::retype_array_type<T,UT>));
        unique_ptr_type ret{ptr, [=](internal::retype_array_type<T,UT>* p) {
            for (std::size_t i = 0; i < size; ++i)
                unmarshaller<T,Buffer,Alignment>::destruct(p[i]);
            free(p);
            std::cout << "freed p" << std::endl;
        }};
        for (std::size_t i = 0; i < size; ++i)
            unmarshaller<T,Buffer,Alignment>::unmarshal_into(a,b,ret[i]);
        return make_unmarshalled_array_holder(size,std::move(ret));
    }
};

}

#endif /* MPIRPC__INTERNAL__RECONSTRUCTION__ALIGNED_TYPE_HOLDER_HPP */
