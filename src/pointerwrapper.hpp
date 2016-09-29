#ifndef POINTERWRAPPER_H
#define POINTERWRAPPER_H

#include <cstddef>
#include <type_traits>

namespace mpirpc
{

namespace detail
{
template<std::size_t N>
class pointer_wrapper_size
{
public:
    constexpr std::size_t size() const { return N; }
};

template<>
class pointer_wrapper_size<0>
{
public:
    pointer_wrapper_size() = delete;
    pointer_wrapper_size(std::size_t size) : m_size(size) {};
    std::size_t size() const { return m_size; }
protected:
    std::size_t m_size;
};

template<typename T, std::size_t N>
class pointer_wrapper : public pointer_wrapper_size<N>
{
public:
    pointer_wrapper() = delete;

    template<std::size_t N2 = N, typename std::enable_if<N2 != 0>::type* = nullptr>
    pointer_wrapper(T* pointer) : pointer_wrapper_size<N2>(), m_pointer(pointer) {}

    template<std::size_t N2 = N, typename std::enable_if<N2 == 0>::type* = nullptr>
    pointer_wrapper(T* pointer, std::size_t size) : pointer_wrapper_size<N2>(size), m_pointer(pointer) {};

    T& operator[](std::size_t n) { return m_pointer[n]; }
    T const& operator[](std::size_t n) const { return m_pointer[n]; }
    operator T*() { return m_pointer; }
protected:
    T* m_pointer;
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
        std::size_t size = detail::pointer_wrapper_size<N>::size();
        for (std::size_t i = 0; i < size; ++i)
            m_allocator.destroy(&m_pointer[i]);
        m_allocator.deallocate(m_pointer,size);
    }

    template<bool B = PassOwnership,
             typename U = T,
             std::enable_if_t<!B>* = nullptr,
             std::enable_if_t<std::is_array<U>::value>* = nullptr>
    void free() {
        std::size_t size = detail::pointer_wrapper_size<N>::size();
        for (std::size_t i = 0; i < size; ++i)
            array_destroy_helper<U,Allocator>::destroy(m_allocator,&m_pointer[i]);
        m_allocator.deallocate(m_pointer,size);
    }

protected:
    Allocator m_allocator;
};

template<typename T, bool PassOwnership, bool PassBack, typename Allocator, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using basic_pointer_wrapper = pointer_wrapper<T,1,PassOwnership,PassBack,Allocator>;

template<typename T, bool PassOwnership, bool PassBack, typename Allocator, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using dynamic_pointer_wrapper = pointer_wrapper<T,0,PassOwnership,PassBack,Allocator>;

}

#endif /* POINTERWRAPPER_H */
