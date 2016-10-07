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

#ifndef MPIRPC__UNMARSHALLING_HPP
#define MPIRPC__UNMARSHALLING_HPP

#include "parameterstream.hpp"
#include "internal/parameterstream.hpp"


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


template<typename T, typename = void>
struct unmarshaller_remote;

struct unmarshal_array_helper_remote
{
    template<typename T, typename Allocator,
             std::enable_if_t<!std::is_array<std::remove_reference_t<T>>::value>* = nullptr>
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, T& v)
    {
        std::cout << "unmarshal_array_helper generic is unmarshalling " << typeid(T).name() << std::endl;

        std::allocator_traits<Allocator>::construct(a,&v,::mpirpc::unmarshaller_remote<T>::unmarshal(a,s));
    }

    template<typename T, std::size_t N, typename Allocator>
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, T(&arr)[N])
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            std::cout << "unmarshal_array_helper array: " << N << " " << i << " " << arr << " " << typeid(T).name() << " " << typeid(decltype(arr[i])).name() << std::endl;
            unmarshal_array_helper_remote::unmarshal(a, s, arr[i]);
        }
    }
};

template<typename T, typename>
struct unmarshaller_remote
{
    template<typename R = T, typename Stream, typename Allocator, std::enable_if_t<std::is_default_constructible<T>::value>* = nullptr>
    static R unmarshal(Allocator &a, Stream &s)
    {
        R ret;
        s >> ret;
        return ret;
    }
};

/*template<typename T>
struct unmarshaller_remote<T, std::enable_if_t<std::is_pointer<std::remove_reference_t<T>>::value>>
{
    template<typename Stream, typename Allocator>
    static T unmarshal(Allocator &a, Stream &s)
    {
        std::cout << "unmarshalling pointer" << std::endl;
        std::size_t size;
        s >> size;
        using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<std::remove_pointer_t<std::remove_reference_t<T>>>;
        AllocatorType al(a);
        std::remove_reference_t<T> data = al.allocate(size);
        using NewAllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        NewAllocatorType na(a);
        for (std::size_t i = 0; i < size; ++i)
        {
            unmarshal_array_helper_remote::unmarshal(na, s, data[i]);
        }
        return data;
    }
};*/


/*template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct unmarshaller_remote<::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using R = ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>;
    template<typename Stream, typename ManagerAllocator>
    static R unmarshal(ManagerAllocator &a, Stream &s)
    {
        std::cout << "unmarshalling pointer wrapper" << std::endl;
        std::size_t size = internal::pointer_wrapper_stream_size<N>::get(s);
        Allocator al(a);
        T* data = al.allocate(size);
        using NewAllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        NewAllocatorType na(a);
        for (std::size_t i = 0; i < size; ++i)
        {
            unmarshal_array_helper_remote::unmarshal(na, s, data[i]);
        }
        return internal::pointer_wrapper_factory<T,N,PassOwnership,PassBack,Allocator>::create(data,size);
    }
};*/

/*template<typename T, std::size_t N>
struct unmarshaller_remote<T(&)[N]>
{
    using R = T(&)[N];
    template<typename Stream, typename Allocator>
    static R unmarshal(Allocator &a, Stream &s)
    {
        std::cout << "unmarshalling reference to array" << std::endl;
        using NewAllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
        NewAllocatorType na(a);
        T(&data)[N] = (T(&)[N]) *na.allocate(N);
        for (std::size_t i = 0; i < N; ++i)
        {
            unmarshal_array_helper_remote::unmarshal(na, s, data[i]);
        }
        return data;
    }
};*/


template<typename Key, typename T>
struct unmarshaller_remote<std::pair<Key,T>>
{
    using R = std::pair<Key,T>;
    template<typename Allocator, typename Stream>
    static inline R unmarshal(Allocator &a, Stream &s)
    {
        return R(unmarshaller_remote<Key>::unmarshal(a,s),unmarshaller_remote<T>::unmarshal(a,s));
    }
};

template<typename T, typename Stream>
struct unmarshaller_local
{
    void unmarshal(Stream &s, T*& v)
    {

    }
};

template<typename Stream, typename Allocator, typename...Args>
auto unmarshal_to_tuple(Allocator &a, Stream &s)
    -> std::tuple<Args...>
{

}

#if 1
/**
 * Unmarshal types and specialize via specialization
 */
template<typename T>
struct unmarshaller;

template<typename Allocator, typename T>
struct unmarshal_array_helper;

/**
 * Unmarshal non-pointer, non-reference-to-array, and (const) char* types
 */
template<typename T,
         typename ManagerAllocator,
         typename R = std::remove_reference_t<T>,
         typename B = std::remove_pointer_t<R>,
         std::enable_if_t<(!std::is_pointer<R>::value) || std::is_same<R,char*>::value>* = nullptr,
         std::enable_if_t<std::is_default_constructible<typename std::remove_reference<T>::type>::value>* = nullptr>
R unmarshal(parameter_stream& s)
{
    return unmarshaller<R>::unmarshal(s);
}

template<typename Allocator, typename T>
struct unmarshal_array_helper
{
    template<typename U = T,
             std::enable_if_t<!std::is_array<U>::value>* = nullptr>
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, U* v)
    {
        std::cout << "unmarshal_array_helper generic is unmarshalling " << typeid(U).name() << std::endl;
        using B = std::remove_extent_t<std::remove_reference_t<U>>;
        using NewAllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<B>;
        NewAllocatorType na(a);
        std::allocator_traits<decltype(na)>::construct(na,v,::mpirpc::unmarshal<B,NewAllocatorType>(s));
    }
};

template<typename Allocator,
         typename T,
         std::size_t N>
struct unmarshal_array_helper<Allocator, T[N]>
{
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, T(*arr)[N])
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            std::cout << "unmarshal_array_helper array: " << N << " " << i << " " << arr << " " << typeid(T).name() << " " << typeid(decltype(&arr[i])).name() << std::endl;
            unmarshal_array_helper<Allocator,T>::unmarshal(a, s, &arr[0][i]);
        }
    }
};

template<class PW, typename ManagerAllocator>
auto unmarshal(mpirpc::parameter_stream& s)
     -> pointer_wrapper<typename PW::type,
                        PW::count,
                        PW::pass_ownership,
                        PW::pass_back,
                        typename PW::allocator_type
                       >
{
    using T = typename PW::type;
    constexpr auto N = PW::count;
    constexpr auto PassOwnership = PW::pass_ownership;
    constexpr auto PassBack = PW::pass_back;
    using Allocator = typename PW::allocator_type;
    Allocator a;
    std::size_t size = internal::pointer_wrapper_stream_size<N>::get(s);
    std::size_t s2;
    std::cout << "creating array for object. " << size << " objects. " << typeid(T).name() << " " << N << std::endl;
    T* data = a.allocate(size);
    std::cout << "pointer: " << data << " " << size << " " << typeid(typename Allocator::value_type).name() << " " << typeid(Allocator).name() << " " << typeid(T).name() << std::endl;
    for (std::size_t i = 0; i < size; ++i)
    {
        unmarshal_array_helper<Allocator,T>::unmarshal(a, s, (T*) &data[i]);
    }
    return internal::pointer_wrapper_factory<T,N,PassOwnership,PassBack,Allocator>::create(data,size);
};

template<typename T,
         typename Allocator,
         typename B = std::remove_extent_t<std::remove_reference_t<T>>,
         std::enable_if_t<std::is_reference<T>::value>* = nullptr,
         std::enable_if_t<std::is_array<std::remove_reference_t<T>>::value>* = nullptr, //enable if reference to array
         std::enable_if_t<(std::extent<std::remove_reference_t<T>>() > 0)>* = nullptr //only known bounds
        >
decltype(auto) unmarshal(mpirpc::parameter_stream &s)
{
    constexpr std::size_t extent = std::extent<std::remove_reference_t<T>>();
    //typename std::allocator_traits<Allocator>::template rebind_alloc<B> a;
    using CorrectedAllocatorType = typename allocator_identifier<Allocator>::template type<B>;
    std::cout << "allocating memory for reference to array" << std::endl;
    CorrectedAllocatorType a;
    std::size_t e;
    s >> e;
    B(*t)[extent] = (B(*)[extent]) a.allocate(extent);
    T r = *t;
    for (std::size_t i = 0; i < extent; ++i)
    {
        std::cout << "extent: " << extent << std::endl;
        unmarshal_array_helper<decltype(a),std::remove_pointer_t<decltype(&r[i])>>::unmarshal(a,s,&r[i]);
    }
    return static_cast<T>(r);
}

template<typename T>
struct unmarshaller
{
    static inline T unmarshal(mpirpc::parameter_stream &s)
    {
        T ret;
        s >> ret;
        return ret;
    }
};

template<typename Key, typename T>
struct unmarshaller<std::pair<Key,T>>
{
    using R = std::pair<Key,T>;
    static inline R unmarshal(parameter_stream &s)
    {
        std::remove_cv_t<Key> k;
        std::remove_cv_t<T> t;
        s >> k >> t;
        return R(k,t);
    }
};

#endif


}

#include "internal/unmarshalling.hpp"

#endif /* MPIRPC__UNMARSHALLING_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
