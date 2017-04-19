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

#ifndef MPIRPC__PARAMETER_BUFFER_HPP
#define MPIRPC__PARAMETER_BUFFER_HPP

#include "common.hpp"
#include "types.hpp"
#include "marshaller.hpp"
#include "unmarshaller.hpp"
#include "internal/detail/alignment.hpp"
#include "internal/type_massaging.hpp"

#include <cstring>

namespace mpirpc
{

template<typename Allocator>
class parameter_buffer
{
public:
    parameter_buffer(std::vector<char,Allocator>* buf) noexcept
        : m_buffer{buf}, m_position{}
    {}

    parameter_buffer(const Allocator& a = Allocator())
        : m_buffer{new std::vector<char,Allocator>(a)}, m_position{}
    {}

    char* data() noexcept { return m_buffer->data(); }
    const char* data() const noexcept { return m_buffer->data(); }
    char* data_here() noexcept { return &m_buffer->data()[m_position]; }
    const char* data_here() const noexcept { return &m_buffer->data()[m_position]; }
    const std::vector<char>* data_vector() const noexcept { return m_buffer; }
    std::size_t position() const noexcept { return m_position; }
    std::size_t size() const noexcept { return m_buffer->size(); }

    void seek(std::size_t pos) noexcept { m_position = pos; }

    void advance(std::size_t len) noexcept { m_position += len; }

    template<typename T>
    T* reinterpret_and_advance(std::size_t size) noexcept { T* ret = reinterpret_cast<T*>(&m_buffer->data()[m_position]); m_position+=size; return ret; }

    template<std::size_t Alignment>
    void append(const char * start, const char * end)
    {
        std::size_t padding = mpirpc::internal::calculate_alignment_padding(m_position,Alignment);
        std::size_t delta = padding + (end-start);
        std::size_t new_size = m_buffer->size() + delta;
        m_buffer->reserve(new_size);
        m_buffer->resize(m_buffer->size() + padding);
        m_buffer->insert(m_buffer->end(),start,end);
        m_position += delta;
    }

    void realign_append(std::size_t alignment)
    {
        std::size_t padding = mpirpc::internal::calculate_alignment_padding(m_position,alignment);
        std::size_t delta = padding;
        std::size_t new_size = m_buffer->size() + delta;
        m_buffer->reserve(new_size);
        m_buffer->resize(m_buffer->size() + padding);
        m_position += delta;
    }

    void realign(std::size_t alignment)
    {
        std::cout << "realign: " << ((m_position % alignment) ? (alignment - (m_position % alignment)) : 0) << std::endl;
        m_position += (m_position % alignment) ? (alignment - (m_position % alignment)) : 0;
    }

    template<std::size_t Alignment>
    void realign()
    {
        std::cout << "realign: " << ((m_position % Alignment) ? (Alignment - (m_position % Alignment)) : 0) << std::endl;
        m_position += (m_position % Alignment) ? (Alignment - (m_position % Alignment)) : 0;
    }

    template<typename Alignment, std::enable_if_t<!is_tuple<Alignment>::value>* = nullptr>
    void realign()
    {
        realign<Alignment::value>();
    }

    template<typename Alignment, std::enable_if_t<is_tuple<Alignment>::value>* = nullptr>
    void realign()
    {
        realign<std::tuple_element_t<0,Alignment>::value>();
    }

    template<typename T, typename Alloc, typename Alignment = type_default_alignment<T,alignof(T)>>
    decltype(auto) pop(Alloc&& a)
    {
        // realign<Alignment>();
        using U = mpirpc::internal::storage_type<T>;
        return mpirpc::unmarshaller<U,parameter_buffer<Allocator>,Alignment>::unmarshal(std::forward<Alloc>(a),*this);
    }

    template<typename T, typename Alignment = type_default_alignment<T,alignof(T)>, typename U>
    void push(U&& t)
    {
        mpirpc::marshaller<std::remove_cv_t<std::remove_reference_t<T>>,parameter_buffer,Alignment>::marshal(*this,std::forward<U>(t));
    }

protected:
    std::size_t m_position;
    std::vector<char,Allocator>* m_buffer;
};

template<typename Allocator>
struct is_aligned_native_binary_buffer<parameter_buffer<Allocator>> : std::true_type {};

template<typename T, typename U>
struct is_same_template : std::false_type{};

template<template <typename...> class T, typename... As, typename... Bs>
struct is_same_template<T<As...>,T<Bs...>> : std::true_type {};

template<typename T, typename U>
constexpr bool is_same_template_v = is_same_template<T,U>::value;

template<typename T>
struct is_parameter_buffer : is_same_template<T,parameter_buffer<std::allocator<char>>> {};

template<typename T>
constexpr bool is_parameter_buffer_v = is_parameter_buffer<T>::value;

template<typename T, typename Buffer, typename Alignment>
struct marshaller<T,Buffer,Alignment,std::enable_if_t<!is_buildtype_v<std::remove_reference_t<T>, Buffer> && !std::is_array<std::remove_reference_t<T>>::value && is_parameter_buffer_v<Buffer>>>
{
    template<typename U,std::enable_if_t<std::is_same<std::decay_t<T>,std::decay_t<U>>::value>* = nullptr>
    static void marshal(Buffer& b, U&& val)
    {
        const char* p = reinterpret_cast<const char*>(&val);
        b.template append<Alignment::value>(p, p+sizeof(T));
    }
};

template<typename T, typename Buffer, typename Alignment>
struct unmarshaller<T,Buffer,Alignment,std::enable_if_t<!is_buildtype_v<std::remove_all_extents_t<std::remove_reference_t<T>>,Buffer> && is_parameter_buffer_v<Buffer>>>
{
    template<typename Allocator>
    static T& unmarshal(Allocator&&, Buffer& b)
    {
        b.template realign<Alignment>();
        return *b.template reinterpret_and_advance<std::remove_reference_t<T>>(sizeof(std::remove_reference_t<T>));
    }
};

/*template<typename Buffer, typename Alignment, typename T, std::size_t N>
struct unmarshaller<T[N],Buffer,Alignment,std::enable_if_t<!is_buildtype_v<std::remove_all_extents_t<T>> && is_parameter_buffer_v<Buffer>>>
{
    template<typename Allocator>
    static T(& unmarshal(Allocator&& a, Buffer& b))[N]
    {
        return *b.template reinterpret_and_advance<T[N]>(sizeof(T[N]));
    }
};*/

template<typename Buffer, typename Alignment>
struct marshaller<char*,Buffer,Alignment,std::enable_if_t<is_parameter_buffer_v<Buffer>>>
{
    static void marshal(Buffer& b, const char* c)
    {
        std::size_t len = strlen(c);
        b.template push<std::size_t>(len);
        b.template append<alignof(char)>(c,c+len);
    }
};

template<typename Buffer, typename Alignment>
struct unmarshaller<std::string,Buffer,Alignment,std::enable_if_t<is_parameter_buffer_v<Buffer>>>
{
    template<typename Allocator>
    static std::string unmarshal(Allocator&& a, Buffer& b)
    {
        std::size_t len = get<std::size_t>(b,a);
        using NA = typename std::allocator_traits<std::remove_reference_t<Allocator>>::template rebind_alloc<char>;
        NA na(a);
        std::string ret(b.data_here(),len,na);
        b.advance(len);
        return ret;
    }
};

}

#endif /* MPIRPC__PARAMETER_BUFFER_HPP */
