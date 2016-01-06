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

#ifndef COMMON_HPP
#define COMMON_HPP

#include<cstddef>
#include<type_traits>
#include<utility>
#include<iostream>
#include<memory>

#ifndef NDEBUG
#   define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__  << " line " << __LINE__ << ": " << message << std::endl; \
            std::exit(ERR_ASSERT); \
        } \
    } while (false)
#else /* NDEBUG */
#define ASSERT(condition, message) do {} while(false)
#endif

namespace mpirpc
{

/**
 * Passer can be used along with uniform initialization to unpack parameter packs
 * and execute the parameters in the order in which they appear. This is necessary
 * for correctness when side effects are important.
 */
struct Passer {
    Passer(...) {}
};

template<typename F>
struct FunctionParts;

template<typename R, class Class, typename... Args>
struct FunctionParts<R(Class::*)(Args...)>
{
    using return_type = R;
    using class_type = Class;
    using function_type = R(Class::*)(Args...);
};

template<typename R, typename... Args>
struct FunctionParts<R(*)(Args...)>
{
    using return_type = R;
    using function_type = R(*)(Args...);
};

using FunctionHandle = unsigned long long;
using TypeId = unsigned long long;
using ObjectId = unsigned long long;

template<typename T> struct remove_all_const : std::remove_const<T> {};

template<typename T> struct remove_all_const<T*> {
    typedef typename remove_all_const<T>::type *type;
};

template<typename T> struct remove_all_const<T * const> {
    typedef typename remove_all_const<T>::type *type;
};

template<typename T>
class AbstractPointerWrapper
{
public:
    AbstractPointerWrapper(T *pointer = nullptr) : m_pointer(pointer) {}
    void setPointer(T* pointer) { m_pointer = pointer; }
    virtual void free() = 0;
    T& operator[](std::size_t n) { return m_pointer[n]; }
    T const& operator[](std::size_t n) const { return m_pointer[n]; }
    operator T*() { return m_pointer; }
    operator T*&() { return m_pointer; }
    operator T*&&() { std::cout << "moved pointer" << std::endl; return std::move(m_pointer); }
protected:
    T* m_pointer;
};

template<typename T, std::size_t N = 1, bool PassOwnership = false, typename Deleter = std::default_delete<T>>
class PointerWrapper : public AbstractPointerWrapper<T>
{
    using AbstractPointerWrapper<T>::m_pointer;
public:
    PointerWrapper(T* data = nullptr) : AbstractPointerWrapper<T>(data) { }
    std::size_t size() const { return N; }
    virtual void free() override { m_deleter(m_pointer); }
    
protected:
    Deleter m_deleter;
};

template<typename T, std::size_t N, typename Deleter>
class PointerWrapper<T, N, true, Deleter> : public AbstractPointerWrapper<T>
{
public:
    PointerWrapper(T* data = nullptr) : AbstractPointerWrapper<T>(data) { }
    std::size_t size() const { return N; }
    virtual void free() override {}
};

template<typename T, std::size_t N, typename Deleter>
class PointerWrapper<T[], N, true, Deleter> : public AbstractPointerWrapper<T>
{
public:
    PointerWrapper(T* data = nullptr) : AbstractPointerWrapper<T>(data) { }
    std::size_t size() const { return N; }
    virtual void free() override {}
};

template<typename T, std::size_t N, typename Deleter>
class PointerWrapper<T[], N, false, Deleter> : public AbstractPointerWrapper<T>
{
    using AbstractPointerWrapper<T>::m_pointer;
public:
    PointerWrapper(T* data = nullptr) : AbstractPointerWrapper<T>(data) { }
    std::size_t size() const { return N; }
    virtual void free() override { m_deleter(m_pointer); }
protected:
    Deleter m_deleter;
};

template<typename T, typename Deleter>
class PointerWrapper<T, 0, false, Deleter> : public AbstractPointerWrapper<T>
{
    using AbstractPointerWrapper<T>::m_pointer;
public:
    PointerWrapper(T* data = nullptr, std::size_t size = 1) : AbstractPointerWrapper<T>(data), m_size(size) {}
    std::size_t size() const { return m_size; }
    void setSize(std::size_t size) { m_size = size; }
    virtual void free() override { m_deleter(m_pointer); }
protected:
    std::size_t m_size;
    Deleter m_deleter;
};

template<typename T, typename Deleter>
class PointerWrapper<T, 0, true, Deleter> : public AbstractPointerWrapper<T>
{
public:
    PointerWrapper(T* data = nullptr, std::size_t size = 1) : AbstractPointerWrapper<T>(data), m_size(size) {}
    std::size_t size() const { return m_size; }
    void setSize(std::size_t size) { m_size = size; }
    virtual void free() override {}
protected:
    std::size_t m_size;
};

template<typename T, typename Deleter>
class PointerWrapper<T[], 0, false, Deleter> : public AbstractPointerWrapper<T>
{
    using AbstractPointerWrapper<T>::m_pointer;
public:
    PointerWrapper(T* data = nullptr, std::size_t size = 1) : AbstractPointerWrapper<T>(data), m_size(size) {}
    std::size_t size() const { return m_size; }
    void setSize(std::size_t size) { m_size = size; }
    virtual void free() override { m_deleter(m_pointer); }
protected:
    std::size_t m_size;
    Deleter m_deleter;
};

template<typename T, typename Deleter>
class PointerWrapper<T[], 0, true, Deleter> : public AbstractPointerWrapper<T>
{
public:
    PointerWrapper(T* data = nullptr, std::size_t size = 1) : AbstractPointerWrapper<T>(data), m_size(size) {}
    std::size_t size() const { return m_size; }
    void setSize(std::size_t size) { m_size = size; }
    virtual void free() override {}
protected:
    std::size_t m_size;
};

template<typename T, bool PassOwnership, typename Deleter, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using BasicPointerWrapper = PointerWrapper<T,1,PassOwnership, Deleter>;

template<typename T, bool PassOwnership, typename Deleter, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using DynamicPointerWrapper = PointerWrapper<T,0,PassOwnership, Deleter>;

template<typename T>
constexpr std::size_t array_size(const T&) noexcept
{
    return sizeof(T)/sizeof(typename std::remove_all_extents<T>::type);
}

/*template<typename T, bool PassOwnership, typename Deleter>
class PointerWrapper<T[], 0, PassOwnership, Deleter> : public AbstractPointerWrapper<T>
{
public:
    PointerWrapper(T *pointer = nullptr, std::size_t size = 1) : AbstractPointerWrapper(pointer), m_size(size) { }
    std::size_t size() const { return m_size; }
    void setSize(std::size_t size) { m_size = size; }
    
    template<bool PO = PassOwnership, typename std::enable_if<!PO>::type* = nullptr>
    void del() { m_deleter(m_pointer); }
    template<bool PO = PassOwnership, typename std::enable_if<PO>::type* = nullptr>
    void del() {}
protected:
    std::size_t m_size;
    Deleter m_deleter;
};

template<typename T, bool PassOwnership, typename Deleter = std::default_delete<T>>
class BasicPointerWrapper : public PointerWrapper<T,1,PassOwnership,Deleter> 
{
    BasicPointerWrapper(T* pointer = nullptr) : PointerWrapper<T,1,PassOwnership,Deleter>(pointer) {}
};*/

/*template<typename T, std::size_t N>
struct CArrayWrapper
{
public:
    CArrayWrapper(T* data = nullptr, std::size_t size = N) { m_data = data; m_size = size; }
    std::size_t size() const { if (N > 0) return N; else return m_size; }
    T& operator[](std::size_t n) { return m_data[n]; }
    T const& operator[](std::size_t n) const { return m_data[n]; }
    T* data() const { return m_data; }
    void setSize(std::size_t size) { m_size = size; }
    void setData(T* data) { m_data = data; }
    void del() { delete[] m_data; }

    operator T*() { return m_data; }
    operator T*&() { return m_data; }
    operator T*&&() { return std::move(m_data); }
protected:
    T* m_data;
    std::size_t m_size;
};*/

template<typename FArg, typename Arg>
struct forward_parameter_type_helper
{
    using base_type = typename std::remove_reference<FArg>::type;
    using type = typename std::conditional<std::is_same<base_type, FArg>::value,base_type,typename std::conditional<std::is_same<base_type&,FArg>::value,base_type&,base_type&&>::type>::type;
};

template<typename FArg, typename T, std::size_t N, bool PassOwnership, typename Deleter>
struct forward_parameter_type_helper<FArg, PointerWrapper<T,N,PassOwnership,Deleter>>
{
    using base_type = PointerWrapper<T,N,PassOwnership,Deleter>;
    using type = typename std::conditional<std::is_same<base_type, FArg>::value,base_type,typename std::conditional<std::is_same<base_type&,FArg>::value,base_type&,base_type&&>::type>::type;
};

/*template<typename FArg, typename T>
struct forward_parameter_type_helper<FArg, PointerWrapper<T>>
{
    using base_type = PointerWrapper<T>;
    using type = typename std::conditional<std::is_same<base_type, FArg>::value,base_type,typename std::conditional<std::is_same<base_type&,FArg>::value,base_type&,base_type&&>::type>::type;
};*/

template<typename FT, typename T>
inline constexpr auto forward_parameter_type(typename std::remove_reference<T>::type& t) noexcept
    -> typename forward_parameter_type_helper<FT, T>::type&&
{
    return static_cast<typename forward_parameter_type_helper<FT, T>::type&&>(t);
}

template<typename FT, typename T>
inline constexpr auto forward_parameter_type(typename std::remove_reference<T>::type&& t) noexcept
    -> typename forward_parameter_type_helper<FT, T>::type&&
{
    return static_cast<typename forward_parameter_type_helper<FT, T>::type&&>(t);
}

/*template<typename FArg, typename Arg, typename T = typename std::remove_reference<FArg>::type>
struct forward_parameter_cleanup_helper
{
    using type = typename std::conditional<std::is_same<T, FArg>::value,T,typename std::conditional<std::is_same<T&,FArg>::value,T&,T&&>::type>::type;
};*/

template<typename FArg, typename Arg>
struct forward_parameter_cleanup_helper
{
    using farg_type = typename std::remove_reference<FArg>::type;
    using arg_type = typename std::remove_reference<Arg>::type;
    //using type = typename std::conditional<std::is_same<farg_type, FArg>::value,arg_type&,typename std::conditional<std::is_same<farg_type&,FArg>::value,arg_type&,arg_type&>::type>::type;
    using type = arg_type&;
};

/*template<typename FArg, typename T>
struct forward_parameter_cleanup_helper<FArg, PointerParameter<T>>
{
    //using farg_type = typename std::remove_reference<FArg>::type;
    //using arg_type = PointerParameter<T>;
    //using type = typename std::conditional<std::is_same<farg_type, FArg>::value,arg_type&,typename std::conditional<std::is_same<farg_type&,FArg>::value,arg_type&,arg_type&>::type>::type;
    using type = PointerParameter<T>&;
};*/

template<typename FArg, typename T, std::size_t N, bool PassOwnership, typename Deleter>
struct forward_parameter_cleanup_helper<FArg, PointerWrapper<T,N,PassOwnership,Deleter>>
{
    //using farg_type = typename std::remove_reference<FArg>::type;
    //using arg_type = CArrayWrapper<T>;
    //using type = typename std::conditional<std::is_same<farg_type, FArg>::value,arg_type&,typename std::conditional<std::is_same<farg_type&,FArg>::value,arg_type&,arg_type&>::type>::type;
    using type = PointerWrapper<T,N,PassOwnership,Deleter>&;
};

template<typename FT, typename T>
inline constexpr auto forward_parameter_cleanup(typename std::remove_reference<T>::type& t) noexcept
    -> typename forward_parameter_cleanup_helper<FT, T>::type
{
    return static_cast<typename forward_parameter_cleanup_helper<FT, T>::type>(t);
}

template<typename FT, typename T>
inline constexpr auto forward_parameter_cleanup(typename std::remove_reference<T>::type&& t) noexcept
    -> typename forward_parameter_cleanup_helper<FT, T>::type&&
{
    return static_cast<typename forward_parameter_cleanup_helper<FT, T>::type>(t);
}

template<typename FT, typename T, typename R = typename remove_all_const<FT>::type>
inline constexpr R&& forward_parameter_type_local(typename std::remove_reference<T>::type& t) noexcept
{
    return static_cast<R&&>(t);
}

template<typename FT, typename T, typename R = typename remove_all_const<FT>::type>
inline constexpr R&& forward_parameter_type_local(typename std::remove_reference<T>::type&& t) noexcept
{
    return static_cast<R&&>(t);
}

}

#endif // COMMON_HPP

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;