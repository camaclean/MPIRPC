/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014  Colin MacLean <s0838159@sms.ed.ac.uk>
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

#ifndef PARAMETERSTREAM_H
#define PARAMETERSTREAM_H

#include "common.hpp"

#include<vector>
#include<cstddef>
#include<cstdint>
#include<string>
#include<sstream>
#include<iostream>
#include<type_traits>
#include<map>

namespace mpirpc {

class ParameterStream
{
public:
    ParameterStream() = delete;
    ParameterStream(std::vector<char>* buffer);
    //ParameterStream(const char* data, size_t length);

    void seek(std::size_t pos);
    std::size_t pos() const { return m_pos; }

    void writeBytes(const char* b, size_t length);
    void readBytes(char*& b, size_t length);
    char* data();
    const char* constData() const;
    std::vector<char>* dataVector() const;
    size_t size() const;

    ParameterStream& operator<<(int8_t val);
    ParameterStream& operator<<(int16_t val);
    ParameterStream& operator<<(int32_t val);
    ParameterStream& operator<<(int64_t val);
    ParameterStream& operator<<(uint8_t val);
    ParameterStream& operator<<(uint16_t val);
    ParameterStream& operator<<(uint32_t val);
    ParameterStream& operator<<(uint64_t val);
    ParameterStream& operator<<(long long val);
    ParameterStream& operator<<(unsigned long long val);

    ParameterStream& operator>>(int8_t& val);
    ParameterStream& operator>>(int16_t& val);
    ParameterStream& operator>>(int32_t& val);
    ParameterStream& operator>>(int64_t& val);
    ParameterStream& operator>>(uint8_t& val);
    ParameterStream& operator>>(uint16_t& val);
    ParameterStream& operator>>(uint32_t& val);
    ParameterStream& operator>>(uint64_t& val);
    ParameterStream& operator>>(long long& val);
    ParameterStream& operator>>(unsigned long long& val);

    ParameterStream& operator<<(float val);
    ParameterStream& operator<<(double val);
    ParameterStream& operator>>(float& val);
    ParameterStream& operator>>(double& val);

    ParameterStream& operator<<(bool val);
    ParameterStream& operator>>(bool& val);

    ParameterStream& operator<<(const char* s);
    ParameterStream& operator>>(char *& s);

    ParameterStream& operator<<(const char* sa[]);
    ParameterStream& operator>>(char **& sa);

    ParameterStream& operator<<(const std::string& val);
    ParameterStream& operator>>(std::string& val);

protected:
    std::vector<char> *m_data;
    std::size_t  m_pos;
};

namespace detail
{
template<std::size_t N>
struct PointerWrapperSize
{
    static std::size_t get(ParameterStream& s)
    {
        std::cout << "static size" << std::endl;
        return N;
    }
};

template<>
struct PointerWrapperSize<0>
{
    static std::size_t get(ParameterStream& s)
    {
        std::size_t size;
        s >> size;
        return size;
    }
};

template<typename T, std::size_t N, bool PassOwnership, typename Allocator>
struct PointerWrapperFactory
{
    static PointerWrapper<T,N,PassOwnership,Allocator> create(T* data, std::size_t size)
    {
        return PointerWrapper<T,N,PassOwnership,Allocator>(data);
    }
};

template<typename T, bool PassOwnership, typename Allocator>
struct PointerWrapperFactory<T,0,PassOwnership,Allocator>
{
    static PointerWrapper<T,0,PassOwnership,Allocator> create(T* data, std::size_t size)
    {
        return PointerWrapper<T,0,PassOwnership,Allocator>(data,size);
    }
};

}

template<typename T>
inline void marshal(ParameterStream& s, T&& val) {
    s << val;
}

// if T, P*, if T*, P*
template<typename T,
         typename R = typename std::decay<T>::type,
         typename B = typename std::remove_pointer<R>::type,
         typename P = B*,
         typename std::enable_if<!std::is_same<R,P>::value || std::is_same<R,char*>::value>::type* = nullptr,
         typename std::enable_if<std::is_default_constructible<typename std::decay<T>::type>::value>::type* = nullptr>
inline R unmarshal(ParameterStream& s) {
    R ret;
    s >> ret;
    return ret;
}

template<class PW>
auto unmarshal(ParameterStream& s)
     -> PointerWrapper<typename PW::type,
                       PW::count,
                       PW::pass_ownership,
                       typename PW::allocator_t>
{
    using T = typename PW::type;
    constexpr auto N = PW::count;
    constexpr auto PassOwnership = PW::pass_ownership;
    using Allocator = typename PW::allocator_t;
    Allocator a;
    std::size_t size = detail::PointerWrapperSize<PW::count>::get(s);
    std::cout << "data count: " << PW::count << " " << size << std::endl;
    T* data = a.allocate(size);
    for (std::size_t i = 0; i < size; ++i)
    {
        data[i] = unmarshal<T>(s);
    }
    return detail::PointerWrapperFactory<T,N,PassOwnership,Allocator>::create(data,size);
}

template<typename T>
ParameterStream& operator<<(ParameterStream& out, const std::vector<T>& vector)
{
    out << vector.size();
    for (std::size_t i = 0; i < vector.size(); ++i)
    {
        out << vector[i];
    }
    return out;
}

template <typename T>
ParameterStream& operator>>(ParameterStream& in, std::vector<T>& vector)
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

template<typename T, typename U>
ParameterStream& operator<<(ParameterStream& out, const std::map<T, U>& map)
{
    out <<  map.size();
    for (auto& pair : map)
    {
        out << pair.first << pair.second;
        std::cout << "streaming map: " << pair.first << " " << pair.second << std::endl;
    }
    return out;
}

template<typename T, typename U>
ParameterStream& operator>>(ParameterStream& in, std::map<T, U>& map)
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

/*template<typename T, std::size_t N>
ParameterStream& operator<<(ParameterStream& out, const T (&a)[N])
{
    for (auto& v : a)
        out << v;
    return out;
}

template<typename T, std::size_t N>
ParameterStream& operator>>(ParameterStream& in, T (&a)[N])
{
    for (std::size_t i = 0; i < N; ++i)
        in >> a[i];
    return in;
}*/

/*template<typename T>
ParameterStream& operator<<(ParameterStream& out, const PointerParameter<T>& p)
{
    out << (std::size_t) 1;
    out << *p.pointer;
    return out;
}

template<typename T>
ParameterStream& operator>>(ParameterStream& in, PointerParameter<T>& p)
{
    std::size_t num;
    in >> num;
    T* t = new T();
    in >> (*t);
    p.pointer = t;
    return in;
}*/

/*template<typename T>
ParameterStream& operator>>(ParameterStream& in, T*& p)
{
    std::size_t num;
    in >> num;
    //p = new T[num]();
    //for (std::size_t i = 0; i < num; ++i)
    //    in >> p[i];
    p = new T();
    //T ret;
    in >> (*p);
    return in;
}*/

template<typename T, std::size_t N, bool PassOwnership, typename Deleter>
ParameterStream& operator<<(ParameterStream& out, const PointerWrapper<T,N,PassOwnership,Deleter>& wrapper)
{
    std::cout << "streaming PointerWrapper" << std::endl;
    if (N == 0)
        out << wrapper.size();
    for (std::size_t i = 0; i < wrapper.size(); ++i)
        out << wrapper[i];
    return out;
}

template<typename T>
ParameterStream& operator>>(ParameterStream& in, AbstractPointerWrapper<T>& wrapper)
{
    std::cout << "setting AbstractPointerWrapper" << std::endl;
    std::size_t size;
    in >> size;
    std::cout << "number of elements: " << size << std::endl;
    wrapper.setPointer(new T[size]);
    for (std::size_t i = 0; i < size; ++i)
        in >> wrapper[i];
    return in;
}

}

#endif // PARAMETERSTREAM_H
