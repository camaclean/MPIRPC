#ifndef FORWARDERS_H
#define FORWARDERS_H

#include <type_traits>
#include "pointerwrapper.hpp"

namespace mpirpc
{

namespace detail
{
template<typename FArg, typename Arg>
struct forward_parameter_type_helper
{
    using base_type = typename std::remove_reference<FArg>::type;
    using type = typename std::conditional<std::is_same<base_type, FArg>::value,base_type,typename std::conditional<std::is_same<base_type&,FArg>::value,base_type&,base_type&&>::type>::type;
};

template<typename FArg, typename T, std::size_t N, bool PassOwnership, typename Deleter>
struct forward_parameter_type_helper<FArg, ::mpirpc::PointerWrapper<T,N,PassOwnership,Deleter>>
{
    using base_type = ::mpirpc::PointerWrapper<T,N,PassOwnership,Deleter>;
    using type = typename std::conditional<std::is_same<base_type, FArg>::value,base_type,typename std::conditional<std::is_same<base_type&,FArg>::value,base_type&,base_type&&>::type>::type;
};

template<typename FT, typename T>
inline constexpr auto forward_parameter_type(typename std::remove_reference<T>::type& t) noexcept
    -> typename detail::forward_parameter_type_helper<FT, T>::type&&
{
    return static_cast<typename detail::forward_parameter_type_helper<FT, T>::type&&>(t);
}

template<typename FT, typename T>
inline constexpr auto forward_parameter_type(typename std::remove_reference<T>::type&& t) noexcept
    -> typename detail::forward_parameter_type_helper<FT, T>::type&&
{
    return static_cast<typename detail::forward_parameter_type_helper<FT, T>::type&&>(t);
}

}

}

#endif /* FORWARDERS_H */
