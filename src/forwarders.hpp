#ifndef FORWARDERS_H
#define FORWARDERS_H

#include <type_traits>
#include "pointerwrapper.hpp"

namespace mpirpc
{

namespace detail
{

template<typename FArg, typename Arg>
struct choose_base_type
{
    using base_type = typename std::remove_reference<FArg>::type;
};

template<typename FArg, typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct choose_base_type<FArg, ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using base_type = ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator>;
};

/**
 * Apply Arg reference type to base_type reference type
 */
template<typename FArg, typename Arg>
struct choose_reference_type
{
    using base_type = typename choose_base_type<FArg,Arg>::base_type;
    using type = typename std::conditional<std::is_lvalue_reference<FArg>::value,
                        base_type&,
                        typename std::conditional<std::is_rvalue_reference<FArg>::value,
                            base_type&&,
                            typename std::conditional<std::is_rvalue_reference<Arg>::value,
                                base_type&&,
                                base_type
                            >::type
                        >::type
                 >::type;
};

template<typename FArg, typename Arg>
struct choose_marshaller_reference_type
{
    using type = FArg;
};

template<typename FArg, typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct choose_marshaller_reference_type<FArg, ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using type = ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator>;
};

template<typename FArg, typename Arg>
struct choose_storage_type
{
    using type = typename std::remove_reference<FArg>::type;
};

template<typename FArg, typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct choose_storage_type<FArg, ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator>>
{
    using type = ::mpirpc::PointerWrapper<T,N,PassOwnership,PassBack,Allocator>;
};

template<typename F, typename... Args>
struct marshaller_function_signature;

template<typename R, typename... FArgs, typename... Args>
struct marshaller_function_signature<R(*)(FArgs...), Args...>
{
    static constexpr std::size_t num_args  = sizeof...(Args );
    static constexpr std::size_t num_fargs = sizeof...(FArgs);
    using return_type = R;
    using parameter_types = std::tuple<typename choose_reference_type<FArgs,Args>::type...>;
    using storage_tuple = std::tuple<typename std::remove_reference<typename choose_storage_type<FArgs,Args>::type>::type...>;
    using applier = std::function<void(typename choose_reference_type<FArgs,Args>::type...)>;
    using type = R(*)(typename choose_reference_type<FArgs,Args>::type...);
};

template<typename R, class C, typename... FArgs, typename... Args>
struct marshaller_function_signature<R(C::*)(FArgs...), Args...>
{
    static constexpr std::size_t num_args  = sizeof...(Args );
    static constexpr std::size_t num_fargs = sizeof...(FArgs);
    using return_type = R;
    using parameter_types = std::tuple<typename choose_reference_type<FArgs,Args>::type...>;
    using storage_tuple = std::tuple<typename std::remove_reference<typename choose_storage_type<FArgs,Args>::type>::type...>;
    using applier = std::function<void(typename choose_reference_type<FArgs,Args>::type...)>;
    using type = R(C::*)(typename choose_reference_type<FArgs,Args>::type...);
};

template<typename R, typename... FArgs, typename... Args>
struct marshaller_function_signature<std::function<R(FArgs...)>, Args...>
{
    static constexpr std::size_t num_args  = sizeof...(Args );
    static constexpr std::size_t num_fargs = sizeof...(FArgs);
    using return_type = R;
    using parameter_types = std::tuple<typename choose_reference_type<FArgs,Args>::type...>;
    using storage_tuple = std::tuple<typename std::remove_reference<typename choose_storage_type<FArgs,Args>::type>::type...>;
    using applier = std::function<void(typename choose_reference_type<FArgs,Args>::type...)>;
    using type = std::function<R(typename choose_reference_type<FArgs,Args>::type...)>;
};

template<std::size_t N, typename FTuple, typename Tuple>
struct types_at_index;

template<std::size_t N, typename... FArgs, typename... Args>
struct types_at_index<N, std::tuple<FArgs...>, std::tuple<Args...>>
{
    using FArg = typename std::tuple_element<N, std::tuple<FArgs...>>::type;
    using Arg  = typename std::tuple_element<N, std::tuple<Args... >>::type;
    using parameter_types = std::tuple<typename choose_reference_type<FArgs,Args>::type...>;
    using applier = std::function<void(typename choose_reference_type<FArgs,Args>::type...)>;
    using type = typename choose_marshaller_reference_type<FArg, Arg>::type;
};

/*template<typename FArg, typename T, std::size_t N, bool PassOwnership, typename Allocator>
struct forward_parameter_type_helper<FArg, ::mpirpc::PointerWrapper<T,N,PassOwnership,Allocator>>
{
    using base_type = ::mpirpc::PointerWrapper<T,N,PassOwnership,Allocator>;
    using type = ::mpirpc::PointerWrapper<T,N,PassOwnership,Allocator>&;// typename std::conditional<std::is_same<base_type, FArg>::value,base_type&&,typename std::conditional<std::is_same<base_type&,FArg>::value,base_type&,base_type&&>::type>::type;
};*/

/**
 * @brief Choose the correct argument type to pass to the type marshalling phase.
 *
 * The function arguments stored in the unmarshaller in Function<> are not necessarily the same type as
 * passed to the invoke function. Therefore, we need to cast invocation arguments to the types expected
 * by the unmarshaller so that the marshaller builds the correct data packet. However, we can't just
 * std::forward<FArgs>(args)... because we need to make an exception for special wrapper types.
 */

/*template<typename FT, typename T>
inline constexpr auto forward_parameter_type(typename std::remove_reference<T>::type& t) noexcept
    -> typename detail::choose_reference_type<FT, typename std::remove_cv<T>::type>::type&
{
    return static_cast<typename detail::choose_reference_type<FT, typename std::remove_cv<T>::type>::type&>(t);
}*/

/*template<typename FT, typename T>
inline constexpr auto forward_parameter_type(typename std::remove_reference<T>::type&& t) noexcept
    -> typename detail::choose_reference_type<FT, typename std::remove_cv<T>::type>::type&&
{
    return static_cast<typename detail::choose_reference_type<FT, typename std::remove_cv<T>::type>::type&&>(t);
}*/

template<typename FT, typename T>
inline constexpr auto forward_parameter_type(T&& t) noexcept
    -> typename detail::choose_reference_type<FT, typename std::remove_cv<T>::type>::type
{
    return static_cast<typename detail::choose_reference_type<FT, typename std::remove_cv<T>::type>::type>(t);
}

}

}

#endif /* FORWARDERS_H */
