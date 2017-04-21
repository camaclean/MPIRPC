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
//#include "internal/reconstruction/aligned_type_holder.hpp"
#include "construction_info.hpp"
#include "types.hpp"
#include "alignment.hpp"
#include <cxxabi.h>
#include <vector>
#include <memory>

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
 * Remote unmarshalling, on the other hand, can require allocating additional memory to
 * hold reference types. There may also be limitations on how types can be constructed or a
 * lack of assignment operators.
 *
 * Take, for example, std::tuple<A&>. This requires constructing a std::tuple<A&> and an
 * object A, requiring at least sizeof(std::tuple<A&>)+sizeof(A) storage (more depending on
 * the alignment requirements).
 *
 * Additionally, there are types without copy or move assignment operators/constructors to
 * consider. Take, for example, std::tuple<int[5]>. This type needs to be default constructed,
 * then modified. It can't be coppied. std::tuple<A&>, on the other hand, can't be default
 * constructed.
 */


template<typename T, typename Buffer, typename Allocator>
using has_unmarshaller = typename detail::has_unmarshaller_helper<T,Buffer,Allocator>::type;

template<typename T, typename Buffer, typename Allocator>
constexpr bool has_unmarshaller_v = detail::has_unmarshaller_helper<T,Buffer,Allocator>::value;

template<typename T, typename Buffer, typename Allocator>
using unmarshaller_type = typename detail::unmarshaller_type_helper<T,Buffer,Allocator>::type;


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

template<typename T, typename U>
struct retype_array
{
    constexpr static std::size_t extent = 0;
    constexpr static std::size_t elements = 1;
    using type = U;
};

template<typename T, std::size_t N, typename U>
struct retype_array<T[N],U>
{
    constexpr static std::size_t extent = N;
    constexpr static std::size_t elements = N*retype_array<T,U>::elements;
    using type = typename retype_array<T,U>::type[N];
};

template<typename T, typename U>
using retype_array_type = typename retype_array<T,U>::type;

template<typename T>
constexpr std::size_t array_total_elements_v = retype_array<T,T>::elements;

template<typename T>
class unmarshalled_array_holder
{
public:

    unmarshalled_array_holder(std::size_t size, T&& ptr)
        : m_size(size), m_ptr(std::move(ptr)) {
            std::cout << "made unmarshalled_array_holder" << std::endl;
        }

    const std::size_t size() const { return m_size; }
    T&& pointer() { std::cout << "getting pointer" << std::endl; return std::move(m_ptr); }
    decltype(auto) operator[] (std::size_t index) { return m_ptr[index]; }

private:
    const std::size_t m_size;
    T m_ptr;
};

template<typename T>
auto make_unmarshalled_array_holder(std::size_t size, T&& t)
    -> unmarshalled_array_holder<T>
{
    std::cout << "making array holder" << std::endl;
    return {size, std::move(t)};
}

template<typename Buffer, typename Alignment, typename T, std::size_t N>
struct unmarshaller<T[N],Buffer,Alignment,
    std::enable_if_t<
        is_buildtype_v<std::remove_all_extents_t<T>,Buffer> //&&
        //is_construction_info_v<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>>
      >
  >
{
    using UT = unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>;
    using unique_ptr_type = std::unique_ptr<retype_array_type<T,UT>[],std::function<void(retype_array_type<T,UT>*)>>;

    template<typename Allocator, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        retype_array_type<T,UT>* ptr;
        posix_memalign((void**) &ptr, internal::alignment_reader<Alignment>::value, N*sizeof(retype_array_type<T,UT>));

        auto ret = std::unique_ptr<retype_array_type<T,UT>[],std::function<void(retype_array_type<T,UT>*)>>{ptr, [](retype_array_type<T,UT>* p) {
            for (std::size_t i = 0; i < N; ++i)
                p[i].~UT();
            free(p);
            std::cout << "freed p" << std::endl;
        }};
        for (std::size_t i = 0; i < N; ++i)
            new (&ret[i]) UT(unmarshaller<T,Buffer,Alignment>::unmarshall(a,b));
        return make_unmarshalled_array_holder(N,std::move(ret));
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
        retype_array_type<T,UT>* ptr;
        posix_memalign((void**) &ptr, internal::alignment_reader<Alignment>::value, N*sizeof(retype_array_type<T,UT>));

        auto ret = std::unique_ptr<retype_array_type<T,UT>[],std::function<void(retype_array_type<T,UT>*)>>{ptr, [](retype_array_type<T,UT>* p) {
            for (std::size_t i = 0; i < N; ++i)
                unmarshaller<T,Buffer,Alignment>::destruct(p[i]);
            free(p);
            std::cout << "freed p" << std::endl;
        }};
        //std::vector<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>> ret;
        for (std::size_t i = 0; i < N; ++i)
            unmarshaller<T,Buffer,Alignment>::unmarshal_into(a,b,ret[i]);
        return make_unmarshalled_array_holder(N,std::move(ret));
    }
};

template<typename Buffer, typename Alignment, typename T>
struct unmarshaller<T[],Buffer,Alignment,
    std::enable_if_t<
        is_buildtype_v<std::remove_all_extents_t<T>,Buffer> //&&
        //is_construction_info_v<unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>>
      >
  >
{
    using UT = unmarshaller_type<std::remove_all_extents_t<T>,Buffer,Alignment>;
    using unique_ptr_type = std::unique_ptr<retype_array_type<T,UT>[],std::function<void(retype_array_type<T,UT>*)>>;

    template<typename Allocator, typename U = T, std::enable_if_t<std::rank<U>::value == 0>* = nullptr>
    static decltype(auto) unmarshal(Allocator&& a, Buffer& b)
    {
        retype_array_type<T,UT>* ptr;
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        posix_memalign((void**) &ptr, internal::alignment_reader<Alignment>::value, size*sizeof(retype_array_type<T,UT>));

        unique_ptr_type ret{ptr, [=](retype_array_type<T,UT>* p) {
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
        retype_array_type<T,UT>* ptr;
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        posix_memalign((void**) &ptr, internal::alignment_reader<Alignment>::value, size*sizeof(retype_array_type<T,UT>));
        unique_ptr_type ret{ptr, [=](retype_array_type<T,UT>* p) {
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

#endif /* MPIRPC__UNMARSHALLER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
