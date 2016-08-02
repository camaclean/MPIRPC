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
#include "pointerwrapper.hpp"
#include "forwarders.hpp"

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
struct PointerWrapperStreamSize
{
    static std::size_t get(ParameterStream& s)
    {
        std::cout << "static size" << std::endl;
        return N;
    }
};

template<>
struct PointerWrapperStreamSize<0>
{
    static std::size_t get(ParameterStream& s)
    {
        std::size_t size;
        s >> size;
        return size;
    }
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct PointerWrapperFactory
{
    static ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator> create(T* data, std::size_t size)
    {
        return ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator>(data);
    }
};

template<typename T, bool PassOwnership, bool PassBack, typename Allocator>
struct PointerWrapperFactory<T,0,PassOwnership,PassBack,Allocator>
{
    static ::mpirpc::PointerWrapper<T,0,PassOwnership,PassBack,Allocator> create(T* data, std::size_t size)
    {
        return ::mpirpc::PointerWrapper<T,0,PassOwnership,PassBack,Allocator>(data,size);
    }
};

}

template<typename T>
inline void marshal(ParameterStream& s, T&& val) {
    //std::cout << "marshalling type: " << typeid(T).name() << " " <<val << std::endl;
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
                       PW::pass_back,
                       typename PW::allocator_t>
{
    using T = typename PW::type;
    constexpr auto N = PW::count;
    constexpr auto PassOwnership = PW::pass_ownership;
    constexpr auto PassBack = PW::pass_back;
    using Allocator = typename PW::allocator_t;
    Allocator a;
    std::size_t size = detail::PointerWrapperStreamSize<PW::count>::get(s);
    std::cout << "creating array for object. " << size << " objects. " << typeid(T).name() << " " << PW::count << std::endl;
    T* data = a.allocate(size);
    std::cout << "pointer: " << data << std::endl;
    for (std::size_t i = 0; i < size; ++i)
    {
        data[i] = unmarshal<T>(s);
        //data[i] = T();
    }
    return detail::PointerWrapperFactory<T,N,PassOwnership,PassBack,Allocator>::create(data,size);
}

template<typename T, typename... O>
ParameterStream& operator<<(ParameterStream& out, const std::vector<T,O...>& vector)
{
    out << vector.size();
    for (std::size_t i = 0; i < vector.size(); ++i)
    {
        out << vector[i];
    }
    return out;
}

template <typename T, typename... O>
ParameterStream& operator>>(ParameterStream& in, std::vector<T,O...>& vector)
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
ParameterStream& operator<<(ParameterStream& out, const std::map<T, U, O...>& map)
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
ParameterStream& operator>>(ParameterStream& in, std::map<T, U, O...>& map)
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

template<typename T, std::size_t N>
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
}

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
ParameterStream& operator<<(ParameterStream& out, const PointerWrapper<T,N,PassOwnership,PassBack,Allocator>& wrapper)
{
    std::cout << "streaming PointerWrapper" << std::endl;
    if (N == 0)
        out << wrapper.size();
    for (std::size_t i = 0; i < wrapper.size(); ++i)
        out << wrapper[i];
    return out;
}

namespace detail
{

template<typename FT, typename IS, typename... Args>
struct fn_type_marshaller_impl;

template<typename... FArgs, std::size_t... Is, typename... Args>
struct fn_type_marshaller_impl<std::tuple<FArgs...>, std::index_sequence<Is...>, Args...>
{
    static void marshal(ParameterStream& ps, Args... args)
    {
        static_assert(sizeof...(FArgs) == sizeof...(Args), "Wrong number of arguments for function.");
        using swallow = int[];
        (void)swallow{(marshal(ps, static_cast<typename detail::types_at_index<Is,std::tuple<FArgs...>,std::tuple<Args...>>::type>(args)), 0)...};
    }
};

template<typename FA, typename A>
struct test;

template<typename... FArgs, typename... Args>
struct test<std::tuple<FArgs...>, std::tuple<Args...>>
{
    static void marshal(ParameterStream& ps, Args... args)
    {
        Passer p{(marshal(ps, detail::forward_parameter_type<FArgs,Args>(args)), std::cout << "blah" << std::endl, 0)...};
    }
};

/*template<typename F>
struct fn_type_marshaller;

template<typename R, typename... FArgs>
struct fn_type_marshaller<R(*)(FArgs...)>
{
    template<class Stream, typename... Args, std::size_t... Is>
    static void marshal(Stream& ps, Args&&... args)
    {
        //test<std::tuple<FArgs...>,std::tuple<Args...>>::marshal(ps, args...);
        detail::fn_type_marshaller_impl<std::tuple<FArgs...>, std::index_sequence_for<Args...>, decltype(std::forward<Args>(args))...>::marshal(ps, std::forward<Args>(args)...);
    }
};

template<typename R, class C, typename... FArgs>
struct fn_type_marshaller<R(C::*)(FArgs...)>
{
    template<class Stream, typename... Args, std::size_t... Is>
    static void marshal(Stream& ps, Args&&... args)
    {
        //test<std::tuple<FArgs...>,std::tuple<Args...>>::marshal(ps, args...);
        detail::fn_type_marshaller_impl<std::tuple<FArgs...>, std::index_sequence_for<Args...>, decltype(std::forward<Args>(args))...>::marshal(ps, std::forward<Args>(args)...);
    }
};

template<typename R, typename... FArgs>
struct fn_type_marshaller<std::function<R(FArgs...)>>
{
    template<class Stream, typename... Args, std::size_t... Is>
    static void marshal(Stream& ps, Args&&... args)
    {
        //test<std::tuple<FArgs...>,std::tuple<Args...>>::marshal(ps, args...);
        detail::fn_type_marshaller_impl<std::tuple<FArgs...>, std::index_sequence_for<Args...>, decltype(std::forward<Args>(args))...>::marshal(ps, std::forward<Args>(args)...);
    }
};*/

template<typename F>
struct fn_type_marshaller
{
    template<class Stream, typename... Args>
    static void marshal(Stream& ps, Args&&... args)
    {
        using Applier = typename detail::marshaller_function_signature<F,Args...>::applier;
        Applier apply = [&](auto&&... a) { Passer p{(marshal(ps, std::forward<decltype(a)>(a)), 0)...}; };
        apply(std::forward<Args>(args)...);
    }
};

}

}

#endif // PARAMETERSTREAM_H
