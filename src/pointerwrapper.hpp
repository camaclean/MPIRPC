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

namespace mpirpc
{

namespace detail
{


template<typename T, std::size_t N>
class pointer_wrapper
{
public:
    pointer_wrapper() = delete;

    pointer_wrapper(T* pointer, std::size_t size = N) : m_size(size), m_pointer(pointer) {}

    T& operator[](std::size_t n) { return m_pointer[n]; }
    T const& operator[](std::size_t n) const { return m_pointer[n]; }
    explicit operator T*() { return m_pointer; }
    std::size_t size() const { return m_size; }
protected:
    T* m_pointer;
    std::size_t m_size;
};

}

template<typename T,typename Allocator>
struct array_destroy_helper
{
    static void destroy(Allocator &a, T* v)
    {
        std::cout << "destroying " << *v << std::endl;
        a.destroy(v);
    }
};

template<typename T,
         std::size_t N,
         typename Allocator>
struct array_destroy_helper<T[N],Allocator>
{
    static void destroy(Allocator &a, T(*arr)[N])
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            array_destroy_helper<T,Allocator>::destroy(a, &arr[0][i]);
        }
    }
};

template<typename T, std::size_t N = 0, bool PassOwnership = false, bool PassBack = false, typename Allocator = std::allocator<T>>
class pointer_wrapper : public detail::pointer_wrapper<T,N>
{
    using detail::pointer_wrapper<T,N>::m_pointer;
    using detail::pointer_wrapper<T,N>::m_size;
public:
    using type = T;// typename std::decay_t<T>;
    static constexpr std::size_t count = N;
    static constexpr bool pass_ownership = PassOwnership;
    static constexpr bool pass_back = PassBack;
    using allocator_type = Allocator;

    template<std::size_t N2 = N, typename std::enable_if<N2 != 0>::type* = nullptr>
    pointer_wrapper(T* data) : detail::pointer_wrapper<T,N2>(data) {}

    template<std::size_t N2 = N, typename std::enable_if<N2 == 0>::type* = nullptr>
    pointer_wrapper(T* data, std::size_t size) : detail::pointer_wrapper<T,N2>(data,size) {}

    template<bool B = PassOwnership, typename std::enable_if<B>::type* = nullptr>
    void free() {}

    template<bool B = PassOwnership,
             typename U = T,
             std::enable_if_t<!B>* = nullptr,
             std::enable_if_t<!std::is_array<U>::value>* = nullptr>
    void free() {
        for (std::size_t i = 0; i < m_size; ++i)
            m_allocator.destroy(&m_pointer[i]);
        m_allocator.deallocate(m_pointer,m_size);
    }

    template<bool B = PassOwnership,
             typename U = T,
             std::enable_if_t<!B>* = nullptr,
             std::enable_if_t<std::is_array<U>::value>* = nullptr>
    void free() {
        for (std::size_t i = 0; i < m_size; ++i)
            array_destroy_helper<U,Allocator>::destroy(m_allocator,&m_pointer[i]);
        m_allocator.deallocate(m_pointer,m_size);
    }

protected:
    Allocator m_allocator;
};

template<typename T, bool PassOwnership, bool PassBack, typename Allocator, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using basic_pointer_wrapper = pointer_wrapper<T,1,PassOwnership,PassBack,Allocator>;

template<typename T, bool PassOwnership, bool PassBack, typename Allocator, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using dynamic_pointer_wrapper = pointer_wrapper<T,0,PassOwnership,PassBack,Allocator>;

}

#endif /* MPIRPC__POINTERWRAPPER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
