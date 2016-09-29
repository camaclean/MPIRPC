#ifndef FORWARDERS_H
#define FORWARDERS_H

#include <type_traits>
#include "pointerwrapper.hpp"

namespace mpirpc
{

namespace detail
{

/*template<typename FArg, typename Arg>
struct choose_marshaller_reference_type
{
    using type = FArg;
};

template<typename FArg, typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct choose_marshaller_reference_type<FArg, ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using type = ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>;
};*/

/*template<typename FArg, typename T, std::size_t N, bool PassOwnership, typename Allocator>
struct forward_parameter_type_helper<FArg, ::mpirpc::PointerWrapper<T,N,PassOwnership,Allocator>>
{
    using base_type = ::mpirpc::PointerWrapper<T,N,PassOwnership,Allocator>;
    using type = ::mpirpc::PointerWrapper<T,N,PassOwnership,Allocator>&;// typename std::conditional<std::is_same<base_type, FArg>::value,base_type&&,typename std::conditional<std::is_same<base_type&,FArg>::value,base_type&,base_type&&>::type>::type;
};*/

}

}

#endif /* FORWARDERS_H */
