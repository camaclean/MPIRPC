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

#ifndef MPIRPC__PARAMETERSTREAM_HPP
#define MPIRPC__PARAMETERSTREAM_HPP

#include "common.hpp"
#include "pointerwrapper.hpp"
#include "concepts/stl.hpp"
#include "internal/function_attributes.hpp"
#include "internal/utility.hpp"

#include<vector>
#include<cstddef>
#include<cstdint>
#include<string>
#include<sstream>
#include<iostream>
#include<type_traits>
#include<iterator>
#include<map>

namespace mpirpc {

class parameter_stream
{
public:
    //parameter_stream() = delete;
    parameter_stream(std::vector<char>* buffer = new std::vector<char>());
    //parameter_stream(const char* data, size_t length);

    void seek(std::size_t pos);
    std::size_t pos() const { return m_pos; }

    void writeBytes(const char* b, size_t length);
    void readBytes(char*& b, size_t length);
    char* data();
    const char* constData() const;
    std::vector<char>* dataVector() const;
    size_t size() const;

    parameter_stream& operator<<(const int8_t val);
    parameter_stream& operator<<(const int16_t val);
    parameter_stream& operator<<(const int32_t val);
    parameter_stream& operator<<(const int64_t val);
    parameter_stream& operator<<(const uint8_t val);
    parameter_stream& operator<<(const uint16_t val);
    parameter_stream& operator<<(const uint32_t val);
    parameter_stream& operator<<(const uint64_t val);
    parameter_stream& operator<<(const long long val);
    parameter_stream& operator<<(const unsigned long long val);

    parameter_stream& operator>>(int8_t& val);
    parameter_stream& operator>>(int16_t& val);
    parameter_stream& operator>>(int32_t& val);
    parameter_stream& operator>>(int64_t& val);
    parameter_stream& operator>>(uint8_t& val);
    parameter_stream& operator>>(uint16_t& val);
    parameter_stream& operator>>(uint32_t& val);
    parameter_stream& operator>>(uint64_t& val);
    parameter_stream& operator>>(long long& val);
    parameter_stream& operator>>(unsigned long long& val);

    parameter_stream& operator<<(const float val);
    parameter_stream& operator<<(const double val);
    parameter_stream& operator>>(float& val);
    parameter_stream& operator>>(double& val);

    parameter_stream& operator<<(const bool val);
    parameter_stream& operator>>(bool& val);

    parameter_stream& operator<<(const char* s);
    parameter_stream& operator>>(char *& s);

    parameter_stream& operator<<(const std::string& val);
    parameter_stream& operator>>(std::string& val);

protected:
    std::vector<char> *m_data;
    std::size_t  m_pos;
};

template<typename A>
struct allocator_identifier;

template<template<typename> typename A, typename T>
struct allocator_identifier<A<T>>
{
    template<typename U>
    using type = A<U>;
};

template<typename Key, typename T>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const std::pair<Key,T>& p)
{
    out << p.first << p.second;
    return out;
}

template<typename Key, typename T>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, std::pair<Key,T>& p)
{
    in >> p.first >> p.second;
    return in;
}

#if defined(__cpp_concepts) || 1

#else

template<typename T, typename... O>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const std::vector<T,O...>& vector)
{
    out << vector.size();
    for (std::size_t i = 0; i < vector.size(); ++i)
    {
        out << vector[i];
    }
    return out;
}

template <typename T, typename... O>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, std::vector<T,O...>& vector)
{
    std::size_t size;
    in >> size;
    vector.resize(size);
    for (std::size_t i = 0; i < size; ++i)
    {
        T val;
        in >> val;
        vector[i] = val;
    }
    return in;
}

template<typename T, typename U, typename... O>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const std::map<T, U, O...>& map)
{
    out <<  map.size();
    for (auto& pair : map)
    {
        out << pair.first << pair.second;
        std::cout << "streaming map: " << pair.first << " " << pair.second << std::endl;
    }
    return out;
}

template<typename T, typename U, typename... O>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, std::map<T, U, O...>& map)
{
    std::size_t size;
    in >> size;
    for (std::size_t i = 0; i < size; ++i)
    {
        T first;
        U second;
        in >> first >> second;
        std::cout << "making map: " << first << " " << second << std::endl;
        map.insert(std::make_pair(first, second));
    }
    return in;
}

#endif

template<typename T, std::size_t N>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const T (&a)[N])
{
    //std::cout << "streaming c array: ";
    //out << N;
    for (auto& v : a)
    {
        //std::cout << v;
        out << v;
    }
    //std::cout << std::endl;
    return out;
}

template<typename T, std::size_t N>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, T (&a)[N])
{
    std::cout << "receiving c array: ";
    //std::size_t size;
    //in >> size;
    for (std::size_t i = 0; i < N; ++i)
    {
        in >> a[i];
        std::cout << a[i] << " ";
    }
    std::cout << std::endl;
    return in;
}

#if defined(__cpp_concepts) || 1

/*template<typename T>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const T& c) requires Container<T>
{
    out << c.size();
    for (const auto& i : c)
    {
        out << i;
    }
    return out;
}

template<typename T>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, T& c) requires Container<T>
{
    std::size_t size;
    in >> size;
    auto it = std::inserter(c,c.begin());
    std::cout << typeid(T).name() << " container has size: " << size << std::endl;
    for (std::size_t i = 0; i < size; ++i)
    {
        //Because array types are not CopyConstructable, Container requires CopyConstructable elements,
        //and the allocator is only used for unmarshalling array types, just pass std::allocator
        it = unmarshal<typename T::value_type, typename T::allocator_type>(in);
    }
    return in;
}*/

#endif

template<typename T>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const mpirpc::pointer_wrapper<T>& wrapper)
{
    std::cout << "streaming PointerWrapper of size " << wrapper.size() << std::endl;
    out << wrapper.size();
    for (std::size_t i = 0; i < wrapper.size(); ++i)
        out << wrapper[i];
    return out;
}


template<typename T>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, mpirpc::pointer_wrapper<T>& wrapper)
{
    std::size_t size;
    in >> size;
    //std::cout << "getting returned pointer of size " << size << std::endl;
    for (std::size_t i = 0; i < size; ++i)
    {
        in >> wrapper[i];
    }
    return in;
}

template<typename T, typename U = std::decay_t<T>, std::enable_if_t<!std::is_array<std::remove_reference_t<T>>::value>* = nullptr>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, U t)
{
    std::size_t size;
    in >> size;
    for (std::size_t i = 0; i < size; ++i)
        in >> t[i];
    return in;
}

}

#endif // PARAMETERSTREAM_H

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
