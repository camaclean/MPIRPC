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

#ifndef MPIRPC__POINTERWRAPPER_HPP
#define MPIRPC__POINTERWRAPPER_HPP

#include <cstddef>
#include <type_traits>
#include "buffer.hpp"
#include "internal/direct_initializer.hpp"
#include "marshaller.hpp"
#include "unmarshaller.hpp"
#include "polymorphic.hpp"

namespace mpirpc
{


template<typename T>
struct array_destroy_helper
{
    template<typename Allocator>
    static void destroy(Allocator&& a, T& v)
    {
        std::cout << "destroying " << typeid(v).name() <<  std::endl;
        using NA = typename std::allocator_traits<std::remove_reference_t<Allocator>>::template rebind_alloc<T>;
        NA na(a);
        std::allocator_traits<NA>::destroy(na,&v);
    }
};

template<typename T,
         std::size_t N>
struct array_destroy_helper<T[N]>
{
    template<typename Allocator>
    static void destroy(Allocator&& a, T(&arr)[N])
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            array_destroy_helper<T>::destroy(a, arr[i]);
        }
    }
};

template<typename T>
class pointer_wrapper
{
public:
    using type = T;

    explicit pointer_wrapper(T* data, std::size_t size = 1, bool pass_back = false, bool pass_ownership = false, std::size_t alignment = alignof(T))
        : m_pointer(data),
          m_size(size),
          m_alignment(alignment),
          m_pass_back(pass_back),
          m_pass_ownership(pass_ownership)
    {}

    T& operator[](std::size_t n) { return m_pointer[n]; }
    T const& operator[](std::size_t n) const { return m_pointer[n]; }
    explicit operator T*() const { return m_pointer; }
    T& operator*() const { return *m_pointer; }
    T* operator->() const { return m_pointer; }

    bool is_pass_back() const { return m_pass_back; }
    bool is_pass_ownership() const { return m_pass_ownership; }
    std::size_t alignment() const { return m_alignment; }
    std::size_t size() const { return m_size; }

    template<typename Allocator,
             typename U = T,
             std::enable_if_t<!std::is_array<U>::value>* = nullptr>
    void free(Allocator &a) {
        if (!m_pass_ownership)
        {
            using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
            AllocatorType al(a);
            for (std::size_t i = 0; i < m_size; ++i)
                std::allocator_traits<AllocatorType>::destroy(al,&m_pointer[i]);
            std::allocator_traits<AllocatorType>::deallocate(al,m_pointer,m_size);
        }
    }

    template<typename Allocator,
             typename U = T,
             std::enable_if_t<std::is_array<U>::value>* = nullptr>
    void free(Allocator &a) {
        if (!m_pass_ownership)
        {
            using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
            AllocatorType al(a);
            for (std::size_t i = 0; i < m_size; ++i)
                array_destroy_helper<U>::destroy(al,m_pointer[i]);
            std::allocator_traits<AllocatorType>::deallocate(al,m_pointer,m_size);
        }
    }

protected:
    T *m_pointer;
    std::size_t m_size;
    std::size_t m_alignment;
    bool m_pass_back;
    bool m_pass_ownership;
};

template<typename T, std::size_t Alignment>
constexpr std::size_t type_alignment<pointer_wrapper<T>,Alignment> = type_alignment<T,alignof(T)>;

template<typename T, typename Buffer, std::size_t Alignment>
struct marshaller<mpirpc::pointer_wrapper<T>, Buffer, Alignment>
{
    template<typename U = T, std::enable_if_t<!std::is_polymorphic<U>::value>* = nullptr>
    static void marshal(Buffer& b, const mpirpc::pointer_wrapper<U>& val)
    {
        b.template push<std::size_t>(val.size());
        b.template push<std::size_t>(val.alignment());
        b.template push<bool>(val.is_pass_back());
        b.template push<bool>(val.is_pass_ownership());
        std::cout << (uintptr_t) b.data_here() << std::endl;
        b.realign_append(val.alignment());
        std::cout << (uintptr_t) b.data_here() << std::endl;
        for (std::size_t i = 0; i < val.size(); ++i)
        {
            b.template push<U>(val[i]);
        }
    }

    template<typename U = T, std::enable_if_t<std::is_polymorphic<U>::value>* = nullptr>
    static void marshal(Buffer& b, const mpirpc::pointer_wrapper<U>& val)
    {
        b.template push<std::size_t>(val.size());
        b.template push<std::size_t>(val.alignment());
        std::cout << "alignment: " << val.alignment() << std::endl;
        b.template push<bool>(val.is_pass_back());
        b.template push<bool>(val.is_pass_ownership());
        b.template push<uintptr_t>(type_identifier<U>::id());
        std::cout << "type id: " << type_identifier<U>::id() << std::endl;
        std::cout << (uintptr_t) b.data_here() << std::endl;
        b.realign_append(val.alignment());
        std::cout << (uintptr_t) b.data_here() << std::endl;
        for (std::size_t i = 0; i < val.size(); ++i)
        {
            b.template push<U>(val[i]);
        }
    }
};

template<typename T, typename Buffer, std::size_t Alignment>
struct unmarshaller<mpirpc::pointer_wrapper<T>,Buffer,Alignment>
{
    template<typename Allocator>
    static std::tuple<std::piecewise_construct_t,T*,std::size_t,bool,bool> unmarshal(Allocator& a, Buffer &b)
    {
        T *ptr;
        std::size_t size = mpirpc::get<std::size_t>(b,a);
        std::size_t alignment = mpirpc::get<std::size_t>(b,a);
        bool pass_back = get<bool>(b,a);
        bool pass_ownership = get<bool>(b,a);
        std::cout << "realignment: " << size << " " <<  alignment << " " << pass_back << " " << pass_ownership << std::endl;
        std::cout << (uintptr_t) b.data_here() << std::endl;
        b.realign(alignment);
        std::cout << (uintptr_t) b.data_here() << std::endl;

        if (pass_ownership || is_buildtype<T,Buffer>)
        {
            if (std::is_polymorphic<T>::value)
            {
                using VoidAllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<char>;
                VoidAllocatorType va(a);
                uintptr_t type = mpirpc::get<uintptr_t>(b,a);
                std::cout << "polymorphic" << type << std::endl;
                ptr = static_cast<T*>(mpirpc::polymorphic_map<Buffer>.at(mpirpc::safe_type_index_map.at(type))->build(va,b,size));
            }
            else
            {
                using AllocatorType = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
                AllocatorType na(a);
                ptr = std::allocator_traits<AllocatorType>::allocate(na,size);
                for (std::size_t i = 0; i < size; ++i)
                    internal::direct_initializer<T>::construct(na,&ptr[i],b);
            }
        }
        else
            get_pointer_from_buffer<alignof(T)>(b,ptr);
        return std::tuple<std::piecewise_construct_t,T*,std::size_t,bool,bool>{std::piecewise_construct,ptr,size,pass_back,pass_ownership};
    }
};

}

#endif /* MPIRPC__POINTERWRAPPER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
