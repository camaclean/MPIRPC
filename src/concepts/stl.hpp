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

#ifndef MPIRPC__CONCEPTS_STL_HPP
#define MPIRPC__CONCEPTS_STL_HPP

#if defined(__cpp_concepts) || 1

namespace mpirpc
{

template<class T>
concept bool Integral = std::is_integral<T>::value;

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

/*template<typename X>
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
};*/

}

#endif /* __cpp_concepts */

#endif /* MPIRPC__CONCEPTS_STL_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
