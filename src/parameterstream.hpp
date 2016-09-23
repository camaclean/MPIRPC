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
#include<iterator>
#include<map>

namespace mpirpc {

class ParameterStream
{
public:
    ParameterStream() = delete;
    ParameterStream(std::vector<char>* buffer = new std::vector<char>());
    //ParameterStream(const char* data, size_t length);

    void seek(std::size_t pos);
    std::size_t pos() const { return m_pos; }

    void writeBytes(const char* b, size_t length);
    void readBytes(char*& b, size_t length);
    char* data();
    const char* constData() const;
    std::vector<char>* dataVector() const;
    size_t size() const;

    ParameterStream& operator<<(const int8_t val);
    ParameterStream& operator<<(const int16_t val);
    ParameterStream& operator<<(const int32_t val);
    ParameterStream& operator<<(const int64_t val);
    ParameterStream& operator<<(const uint8_t val);
    ParameterStream& operator<<(const uint16_t val);
    ParameterStream& operator<<(const uint32_t val);
    ParameterStream& operator<<(const uint64_t val);
    ParameterStream& operator<<(const long long val);
    ParameterStream& operator<<(const unsigned long long val);

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

    ParameterStream& operator<<(const float val);
    ParameterStream& operator<<(const double val);
    ParameterStream& operator>>(float& val);
    ParameterStream& operator>>(double& val);

    ParameterStream& operator<<(const bool val);
    ParameterStream& operator>>(bool& val);

    ParameterStream& operator<<(const char* s);
    ParameterStream& operator>>(char *& s);

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
inline void marshal(ParameterStream& s, T&& val)
{
    s << val;
}



/*template<typename T>
struct unmarshaller;*/

template<typename T>
struct unmarshaller
{
    static inline T unmarshal(ParameterStream &s)
    {
        T ret;
        s >> ret;
        return ret;
    }
};

/*template<typename T>
struct unmarshaller<typename std::enable_if<(!std::is_same<std::decay_t<T>,std::remove_pointer_t<std::decay_t<T>>>::value || std::is_same<std::decay_t<T>,char*>::value) &&
                                            std::is_default_constructible<std::decay_t<T>>::value>::type>
{
    static inline std::decay_t<T> unmarshal(ParameterStream &s)
    {
        std::decay_t<T> ret;
        s >> ret;
        return ret;
    }
};*/

// if T, P*, if T*, P*
// Unmarshall non-pointer types and char* types
template<typename T,
         typename R = typename std::decay<T>::type,
         typename B = typename std::remove_pointer<R>::type,
         typename P = B*,
         typename std::enable_if<!std::is_same<R,P>::value || std::is_same<R,char*>::value>::type* = nullptr,
         typename std::enable_if<std::is_default_constructible<typename std::decay<T>::type>::value>::type* = nullptr>
inline R unmarshal(ParameterStream& s)
{
    return unmarshaller<R>::unmarshal(s);
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
    std::size_t size = detail::PointerWrapperStreamSize<N>::get(s);
    std::cout << "creating array for object. " << size << " objects. " << typeid(T).name() << " " << N << std::endl;
    T* data = a.allocate(size);
    //T* data = new T[size];
    //std::cout << "pointer: " << data << " " << size << " " << typeid(typename Allocator::value_type).name() << " " << typeid(Allocator).name() << " " << typeid(T).name() << std::endl;
    for (std::size_t i = 0; i < size; ++i)
    {
        //data[i] = unmarshal<T>(s);
        //data[i] = T();
        std::allocator_traits<Allocator>::construct(a,&data[i],unmarshal<T>(s));
        //new(&data[i]) T(unmarshal<T>(s));
    }
    return detail::PointerWrapperFactory<T,N,PassOwnership,PassBack,Allocator>::create(data,size);
};

template<typename Key, typename T>
struct unmarshaller<std::pair<Key,T>>
{
    using R = std::pair<Key,T>;
    static inline R unmarshal(ParameterStream &s)
    {
        std::remove_cv_t<Key> k;
        std::remove_cv_t<T> t;
        s >> k >> t;
        return R(k,t);
    }
};

/*template<typename T>
auto unmarshal(ParameterStream &s)
{
    return unmarshaller<T>::unmarshal(s);
}*/

#if defined(__cpp_concepts) || 1

template<class T>
concept bool Integral = std::is_integral<T>::value;

template<typename T, typename I>
concept bool HasArraySubscriptOperator = requires(T t, I i) {
        t[i];
};

template<typename T>
concept bool HasIntegralArraySubscriptOperator = HasArraySubscriptOperator<T,size_t>;

template<typename T>
concept bool HasSizeFunction = requires(T t)
{
    t.size();
};

template<typename T>
concept bool MoveAssignable = requires(T t, T&& rv)
{
    { t = rv } -> T&;
};

template<typename T>
concept bool CopyAssignable = requires(T t, T& v)
{
    { t = v } -> T&;
    requires MoveAssignable<T>;
};

template<typename T>
concept bool MoveConstructible = requires(T&& rv, T u)
{
    { u = rv };
    { T(rv) } -> T;
};

template<typename T>
concept bool CopyConstructible = requires(T v1, const T& v2, T u)
{
    { u = v1 };
    { u = v2 };
    { T(v1) } -> T;
    { T(v2) } -> T;
    requires MoveConstructible<T>;
};

template<typename T>
concept bool DefaultConstructible = requires()
{
    { T() } -> T;
    { T{} } -> T;
};

template<typename T>
concept bool Destructible = requires(T u)
{
    { u.~T() };
};

template<typename T, typename U>
concept bool Swappable = requires(T t, U u)
{
    { std::swap(t,u) };
    { std::swap(u,t) };
};

template<typename A, typename T>
concept bool CopyInsertable = requires(A m, T* p, T v)
{
    std::allocator_traits<A>::construct(m, p, v);
};

template<typename X>
concept bool CopyInsertableOptionalAllocAware =
        CopyInsertable<typename X::allocator_type, typename X::value_type> || CopyInsertable<std::allocator<typename X::value_type>, typename X::value_type>;

template<typename A, typename T>
concept bool Erasable = requires(A m, T* p)
{
    std::allocator_traits<A>::destroy(m, p);
};

template<typename X>
concept bool ErasableOptionalAllocAware =
        Erasable<typename X::allocator_type, typename X::value_type> || Erasable<std::allocator<typename X::value_type>, typename X::value_type>;

template<typename It>
concept bool Iterator = requires(It& r)
{
    { *r }  -> auto;
    { ++r } -> It&;
    requires CopyAssignable<It>;
    requires CopyConstructible<It>;
    requires Destructible<It>;
    requires Swappable<It&,It&>;
};

template<typename T>
concept bool EqualityComparable = requires(T a, T b)
{
    { a == b } -> bool;
};

template<typename It>
concept bool InputIterator = requires(It i,
                                      It j,
                                      typename std::iterator_traits<It>::reference reference)
{
    typename std::iterator_traits<It>::reference;
    typename std::iterator_traits<It>::value_type;
    { reference } -> typename std::iterator_traits<It>::value_type;
    { i != j } -> bool;
    { *i } -> typename std::iterator_traits<It>::reference;
    { (i).operator->() };
    { (void)i++ };
    { *i++ } -> typename std::iterator_traits<It>::value_type;
    requires Iterator<It>;
    requires EqualityComparable<It>;
};

template<typename It, typename T>
concept bool OutputIterator = requires(It r, T o)
{
    { *r = o };
    { *r++ = o };
    { r++ } -> const It&;
    requires Iterator<It>;
};

template<typename It>
concept bool ForwardIterator = requires(It i)
{
    { i++ }  -> It;
    { *i++ } -> typename std::iterator_traits<It>::reference;
    requires InputIterator<It>;
    requires DefaultConstructible<It>;
};

template<typename C>
concept bool Container = requires(C a, C b)
{
    typename C::value_type;
    typename C::reference;
    typename C::const_reference;
    typename C::iterator;
    typename C::const_iterator;
    { C() }                             -> C;
    { C(a) }                            -> C;
    { a = b }                           -> C&;
    { (&a)->~C() }                      -> void;
    { a.begin() }                       -> typename C::iterator;
    { const_cast<const C&>(a).begin() } -> typename C::const_iterator;
    { a.end() }                         -> typename C::iterator;
    { const_cast<const C&>(a).end() }   -> typename C::const_iterator;
    { a.cbegin() }                      -> typename C::const_iterator;
    { a.cend() }                        -> typename C::const_iterator;
    { a == b }                          -> bool;
    { a != b }                          -> bool;
    { a.swap(b) }                       -> void;
    { swap(a,b) }                       -> void;
    { a.size() }                        -> typename C::size_type;
    { a.max_size() }                    -> typename C::size_type;
    { a.empty() }                       -> bool;
    requires ForwardIterator<typename C::iterator>;
    requires OutputIterator<typename C::iterator, typename C::value_type>;
    requires ForwardIterator<typename C::const_iterator>;
    requires CopyInsertableOptionalAllocAware<C>;
    requires ErasableOptionalAllocAware<C>;
};

template<typename X>
concept bool SequenceContainer = requires(X&& a, typename X::value_type t)
{
    { a.push_front(t) } -> typename X::iterator;
    requires HasSizeFunction<X>;
    requires HasIntegralArraySubscriptOperator<X>;
    requires Container<X>;
};

template<typename C>
concept bool AssociativeContainer = requires(C a)
{
    typename C::key_type;
    typename C::mapped_type;
    { a.key_comp() }   -> typename C::key_compare;
    { a.value_comp() } -> typename C::value_compare;
    requires Container<C>;
};

template<typename Key, typename T>
ParameterStream& operator<<(ParameterStream& out, const std::pair<Key,T>& p)
{
    out << p.first << p.second;
    return out;
}

template<typename Key, typename T>
ParameterStream& operator>>(ParameterStream& in, std::pair<Key,T>& p)
{
    in >> p.first >> p.second;
    return in;
}

template<typename T>
ParameterStream& operator<<(ParameterStream& out, const T& c) requires Container<T>
{
    out << c.size();
    for (const auto& i : c)
    {
        out << i;
    }
    return out;
}

template<typename Key, typename T>
std::ostream& operator<<(std::ostream& s, const std::pair<Key,T>& p)
{
    return s;
}

template<typename T>
ParameterStream& operator>>(ParameterStream& in, T& c) requires Container<T>
{
    std::size_t size;
    in >> size;
    auto it = std::inserter(c,c.begin());
    for (std::size_t i = 0; i < size; ++i)
    {
        it = unmarshal<typename T::value_type>(in);
    }
    return in;
}

/*template<typename T>
ParameterStream& operator<<(ParameterStream& out, const T& sc) requires SequenceContainer<T>
{
    out << sc.size();
    for (std::size_t i = 0; i < sc.size(); ++i)
        out << sc[i];
    return out;
}

template<typename T>
ParameterStream& operator>>(ParameterStream& in, T& sc) requires SequenceContainer<T>
{
    std::size_t size;
    in >> size;
    sc.resize(size);
    for (std::size_t i = 0; i < size; ++i)
    {
        typename T::value_type val;
        in >> val;
        sc[i] = val;
    }
    return in;
}

template<typename T>
ParameterStream& operator<<(ParameterStream& out, const T& map) requires AssociativeContainer<T>
{
    out <<  map.size();
    for (const auto& pair : map)
    {
        out << pair.first << pair.second;
        std::cout << "streaming map: " << pair.first << " " << pair.second << std::endl;
    }
    return out;
}

template<typename T>
ParameterStream& operator>>(ParameterStream& in, T& map) requires AssociativeContainer<T>
{
    std::size_t size;
    in >> size;
    for (std::size_t i = 0; i < size; ++i)
    {
        typename T::key_type first;
        typename T::mapped_type second;
        in >> first >> second;
        std::cout << "making map: " << first << " " << second << std::endl;
        map.insert(std::make_pair(first, second));
    }
    return in;
}*/

/*template<typename T, typename U, typename... O>
ParameterStream& operator<<(ParameterStream& out, const std::map<T, U, O...>& map)
{
    out <<  map.size();
    for (auto& pair : map)
    {
        out << pair.first << pair.second;
        std::cout << "streaming map: " << pair.first << " " << pair.second << std::endl;
    }
    return out;
}*/

/*template<typename T, typename U, typename... O>
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
}*/

#else

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

#endif

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
        Applier apply = [&](auto&&... a) { Passer p{(mpirpc::marshal(ps, std::forward<decltype(a)>(a)), 0)...}; };
        apply(std::forward<Args>(args)...);
    }
};

}

}

#endif // PARAMETERSTREAM_H
