#ifndef POINTERWRAPPER_H
#define POINTERWRAPPER_H

#include <cstddef>
#include <type_traits>

namespace mpirpc
{

namespace detail
{
template<std::size_t N>
class PointerWrapperSize
{
public:
    constexpr std::size_t size() const { return N; }
};

template<>
class PointerWrapperSize<0>
{
public:
    PointerWrapperSize() = delete;
    PointerWrapperSize(std::size_t size) : m_size(size) {};
    std::size_t size() const { return m_size; }
protected:
    std::size_t m_size;
};

template<typename T, std::size_t N>
class PointerWrapper : public PointerWrapperSize<N>
{
public:
    PointerWrapper() = delete;

    template<std::size_t N2 = N, typename std::enable_if<N2 != 0>::type* = nullptr>
    PointerWrapper(T* pointer) : PointerWrapperSize<N2>(), m_pointer(pointer) {}

    template<std::size_t N2 = N, typename std::enable_if<N2 == 0>::type* = nullptr>
    PointerWrapper(T* pointer, std::size_t size) : PointerWrapperSize<N2>(size), m_pointer(pointer) {};

    T& operator[](std::size_t n) { return m_pointer[n]; }
    T const& operator[](std::size_t n) const { return m_pointer[n]; }
    operator T*() { return m_pointer; }
protected:
    T* m_pointer;
};

}

template<typename T, std::size_t N = 0, bool PassOwnership = false, typename Allocator = std::allocator<T>>
class PointerWrapper : public detail::PointerWrapper<T,N>
{
    using detail::PointerWrapper<T,N>::m_pointer;
public:
    using type = T;
    static constexpr std::size_t count = N;
    static constexpr bool pass_ownership = false;
    using allocator_t = Allocator;

    template<std::size_t N2 = N, typename std::enable_if<N2 != 0>::type* = nullptr>
    PointerWrapper(T* data) : detail::PointerWrapper<T,N2>(data) {}

    template<std::size_t N2 = N, typename std::enable_if<N2 == 0>::type* = nullptr>
    PointerWrapper(T* data, std::size_t size) : detail::PointerWrapper<T,N2>(data,size) {}

    template<bool B = PassOwnership, typename std::enable_if<B>::type* = nullptr>
    void free() {}

    template<bool B = PassOwnership, typename std::enable_if<!B>::type* = nullptr>
    void free() {
        std::size_t size = detail::PointerWrapperSize<N>::size();
        for (std::size_t i = 0; i < size; ++i)
            m_allocator.destroy(&m_pointer[i]);
        m_allocator.deallocate(m_pointer,size);
    }
protected:
    Allocator m_allocator;
};

template<typename T, bool PassOwnership, typename Allocator, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using BasicPointerWrapper = PointerWrapper<T,1,PassOwnership, Allocator>;

template<typename T, bool PassOwnership, typename Allocator, typename std::enable_if<!std::is_array<T>::value>::type* = nullptr>
using DynamicPointerWrapper = PointerWrapper<T,0,PassOwnership, Allocator>;

}

#endif /* POINTERWRAPPER_H */
