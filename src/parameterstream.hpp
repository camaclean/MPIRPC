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
    parameter_stream() = delete;
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

namespace detail
{
template<std::size_t N>
struct pointer_wrapper_stream_size
{
    static std::size_t get(parameter_stream& s)
    {
        std::cout << "static size" << std::endl;
        return N;
    }
};

template<>
struct pointer_wrapper_stream_size<0>
{
    static std::size_t get(parameter_stream& s)
    {
        std::size_t size;
        s >> size;
        return size;
    }
};

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct pointer_wrapper_factory
{
    static ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator> create(T* data, std::size_t size)
    {
        return ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>(data);
    }
};

template<typename T, bool PassOwnership, bool PassBack, typename Allocator>
struct pointer_wrapper_factory<T,0,PassOwnership,PassBack,Allocator>
{
    static ::mpirpc::pointer_wrapper<T,0,PassOwnership,PassBack,Allocator> create(T* data, std::size_t size)
    {
        return ::mpirpc::pointer_wrapper<T,0,PassOwnership,PassBack,Allocator>(data,size);
    }
};

}

template<typename T>
inline void marshal(mpirpc::parameter_stream& s, T&& val)
{
    s << val;
}



/*template<typename T>
struct unmarshaller;*/

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
         template<typename> typename ManagerAllocator,
         typename R = typename std::decay<T>::type,
         typename B = typename std::remove_pointer<R>::type,
         typename P = B*,
         typename std::enable_if<!std::is_same<R,P>::value || std::is_same<R,char*>::value>::type* = nullptr,
         typename std::enable_if<std::is_default_constructible<typename std::remove_reference<T>::type>::value>::type* = nullptr>
inline R unmarshal(parameter_stream& s)
{
    return unmarshaller<R>::unmarshal(s);
}

template<typename Allocator, typename T,template<typename> typename ManagerAllocator>
struct unmarshal_array_helper
{
    template<typename U = T,
             std::enable_if_t<std::is_same<U,int>::value>* = nullptr,
             std::enable_if_t<!std::is_array<U>::value>* = nullptr>
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, U* v)
    {
        using B = std::remove_extent_t<std::remove_reference_t<T>>;
        U tmp = ::mpirpc::unmarshal<B,ManagerAllocator>(s);
        std::cout << "unmarshalling_array_helper got: " << tmp << std::endl;
        typename std::allocator_traits<Allocator>::template rebind_alloc<int> na(a);
        std::allocator_traits<decltype(na)>::construct(na,v,tmp);
        //new(v) B(tmp);
    }

    template<typename U = T,
             std::enable_if_t<!std::is_same<U,int>::value>* = nullptr,
             std::enable_if_t<!std::is_array<U>::value>* = nullptr>
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, U* v)
    {
        using B = std::remove_extent_t<std::remove_reference_t<U>>;
        typename std::allocator_traits<Allocator>::template rebind_alloc<B> na(a);
        std::allocator_traits<decltype(na)>::construct(na,v,::mpirpc::unmarshal<B,ManagerAllocator>(s));
        //new(v) B(::mpirpc::unmarshal<B,ManagerAllocator>(s));
    }

    /*template<typename U = T, std::enable_if_t<std::is_array<U>::value>* = nullptr>
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, U* arr)
    {
        for(std::size_t i = 0; i < std::extent<U>::value; ++i)
        {
            //std::cout << std::extent<U>::value << " " << i << " " << arr << " " << typeid(U).name() << " " << typeid(decltype(&arr[i])).name() << std::endl;
            unmarshal_array_helper<Allocator,U,ManagerAllocator>::unmarshal(a, s, &arr[0][i]);
        }
    }*/
};

template<typename Allocator,
         typename T,
         std::size_t N,
         template<typename> typename ManagerAllocator>
struct unmarshal_array_helper<Allocator, T[N], ManagerAllocator>
{
    /*static void unmarshal(::mpirpc::parameter_stream &s, T(*arr)[N])
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            std::cout << arr << std::endl;
            unmarshal_array_helper<T,ManagerAllocator>::unmarshal(s, (T*) &arr[N]);
            //std::allocator_traits<Allocator>::construct(a,&arr[i],unmarshal_array_helper(s,a))
        }
    }*/
    static void unmarshal(Allocator& a, ::mpirpc::parameter_stream &s, T(*arr)[N])
    {
        for(std::size_t i = 0; i < N; ++i)
        {
            std::cout << N << " " << i << " " << arr << " " << typeid(T).name() << " " << typeid(decltype(&arr[i])).name() << std::endl;
            unmarshal_array_helper<Allocator,T,ManagerAllocator>::unmarshal(a, s, &arr[0][i]);
        }
    }
};

template<typename T,
         template<typename> typename ManagerAllocator,
         typename B = std::remove_extent_t<std::remove_reference_t<T>>,
         std::enable_if_t<std::is_reference<T>::value>* = nullptr,
         std::enable_if_t<std::is_array<std::remove_reference_t<T>>::value>* = nullptr, //enable if reference to array
         std::enable_if_t<(std::extent<std::remove_reference_t<T>>() > 0)>* = nullptr //only known bounds
        >
decltype(auto) unmarshal(mpirpc::parameter_stream &s)
{
    constexpr std::size_t extent = std::extent<std::remove_reference_t<T>>();
    ManagerAllocator<std::remove_reference_t<T>> a;
    B(*t)[extent] = a.allocate(extent);
    T r = *t;
    for (std::size_t i = 0; i < extent; ++i)
    {
        std::cout << "extent: " << extent << std::endl;
        unmarshal_array_helper<decltype(a),std::remove_pointer_t<decltype(&r[i])>,ManagerAllocator>::unmarshal(a,s,&r[i]);
        //std::allocator_traits<decltype(a)>::construct(a,&t[i],unmarshal_array_helper(s,a,t[i]));
        //s >> t[i];
    }
    return static_cast<T>(r);
}

template<class PW, template<typename> typename ManagerAllocator>
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
    std::size_t size = detail::pointer_wrapper_stream_size<N>::get(s);
    std::cout << "creating array for object. " << size << " objects. " << typeid(T).name() << " " << N << std::endl;
    T* data = a.allocate(size);
    //T* data = new T[size];
    std::cout << "pointer: " << data << " " << size << " " << typeid(typename Allocator::value_type).name() << " " << typeid(Allocator).name() << " " << typeid(T).name() << std::endl;
    for (std::size_t i = 0; i < size; ++i)
    {
        unmarshal_array_helper<Allocator,T,ManagerAllocator>::unmarshal(a, s, (T*) &data[i]);
        //std::allocator_traits<Allocator>::construct(a,&data[i],unmarshal<T&,ManagerAllocator>(s));
        //new(&data[i]) T(unmarshal<T>(s));
    }
    return detail::pointer_wrapper_factory<T,N,PassOwnership,PassBack,Allocator>::create(data,size);
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

template<typename T>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const T& c) requires Container<T>
{
    out << c.size();
    std::cout << typeid(T).name() << " container has size (streaming): " << c.size() << std::endl;
    for (const auto& i : c)
    {
        std::cout << i << std::endl;
        out << i;
    }
    return out;
}

template<typename Key, typename T>
std::ostream& operator<<(std::ostream& s, const std::pair<Key,T>& p)
{
    std::cout << "[" << p.first << "," << p.second << "]";
    return s;
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
        typename T::value_type tmp = unmarshal<typename T::value_type, std::allocator>(in);
        it = tmp;
        std::cout << tmp << ",";
    }
    for (auto &i : c)
    {
        std::cout << "after getting: " << i << std::endl;
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
    std::cout << "streaming c array: ";
    for (auto& v : a)
    {
        std::cout << v;
        out << v;
    }
    std::cout << std::endl;
    return out;
}

template<typename T, std::size_t N>
mpirpc::parameter_stream& operator>>(mpirpc::parameter_stream& in, T (&a)[N])
{
    for (std::size_t i = 0; i < N; ++i)
        in >> a[i];
    return in;
}

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
mpirpc::parameter_stream& operator<<(mpirpc::parameter_stream& out, const mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>& wrapper)
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

/*template<typename FT, typename IS, typename... Args>
struct fn_type_marshaller_impl;

template<typename... FArgs, std::size_t... Is, typename... Args>
struct fn_type_marshaller_impl<std::tuple<FArgs...>, std::index_sequence<Is...>, Args...>
{
    static void marshal(mpirpc::parameter_stream& ps, Args... args)
    {
        static_assert(sizeof...(FArgs) == sizeof...(Args), "Wrong number of arguments for function.");
        using swallow = int[];
        (void)swallow{(marshal(ps, static_cast<internal::choose_wrapped_at_index_type<Is,std::tuple<FArgs...>,std::tuple<Args...>>>(args)), 0)...};
    }
};*/

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

}

}

#endif // PARAMETERSTREAM_H
