#ifndef MANAGER_INVOKE_H
#define MANAGER_INVOKE_H

#include "../../manager.hpp"

/*************************************************************************************/
/*************************************************************************************/
/*                              Public Invocation API                                */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*                    Type Casting Returning Non-Member Invokers                     */
/*************************************************************************************/

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
auto mpirpc::manager<MessageInterface>::invoke_function_r(int rank, Args&&... args)
    -> typename detail::marshaller_function_signature<F,Args...>::return_type
{
    if (rank == m_rank)
    {
        return f(std::forward<Args>(args)...);
    } else {
        send_function_invocation<F,f>(rank, true, std::forward<Args>(args)...);
        return process_return<typename detail::marshaller_function_signature<F,Args...>::return_type>(rank, typename detail::marshaller_function_signature<F,Args...>::pass_backs{}, std::forward<Args>(args)...);
    }
}

template<typename MessageInterface>
template<typename R, typename... FArgs, typename... Args>
auto mpirpc::manager<MessageInterface>::invoke_function_r(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args)
    -> typename std::enable_if<!std::is_same<R, void>::value, R>::type
{
    using pass_backs = bool_tuple<is_pass_back<FArgs>::value...>;
    if (rank == m_rank) {
        return f(std::forward<Args>(args)...);
    } else {
        if (functionHandle == 0)
        {
            for (const auto &i : m_registered_functions) {
                if (i.second->pointer() == reinterpret_cast<void(*)()>(f)) {
                    send_function_invocation(rank, i.first, true, detail::forward_parameter_type<FArgs>(args)...);
                    return process_return<R>(rank, pass_backs{}, std::forward<Args>(args)...);
                }
            }
        }
        else
        {
            send_function_invocation(rank, functionHandle, true, detail::forward_parameter_type<FArgs>(args)...);
            return process_return<R>(rank, pass_backs{}, std::forward<Args>(args)...);
        }
        throw UnregisteredFunctionException();
    }
}

/*************************************************************************************/
/*                   Type Casting Non-Returning Non-Member Invokers                  */
/*************************************************************************************/

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::manager<MessageInterface>::invoke_function(int rank, Args&&... args)
{
    if (rank == m_rank)
    {
        f(std::forward<Args>(args)...);
    } else {
        send_function_invocation<F,f>(rank, false, std::forward<Args>(args)...);
    }
}

template<typename MessageInterface>
template<typename R, typename... FArgs, typename...Args>
void mpirpc::manager<MessageInterface>::invoke_function(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args)
{
    if (rank == m_rank) {
        f(detail::forward_parameter_type<FArgs,Args>(args)...);
    } else {
        if (functionHandle == 0) {
            for (const auto &i : m_registered_functions) {
                if (i.second->pointer() == reinterpret_cast<void(*)()> (f)) {
                    send_function_invocation(rank, i.first, false, detail::forward_parameter_type<FArgs>(args)...);
                    return;
                }
            }
            throw UnregisteredFunctionException();
        }
        else
        {
            send_function_invocation(rank, functionHandle, false, detail::forward_parameter_type<FArgs>(args)...);
        }
    }
}

/*************************************************************************************/
/*                     Non-Typesafe Returning Non-Member Invoker                     */
/*************************************************************************************/

template<typename MessageInterface>
template<typename R, typename... Args>
R mpirpc::manager<MessageInterface>::invoke_function_r(int rank, FnHandle functionHandle, Args&&... args)
{
    send_function_invocation(rank, functionHandle, true, std::forward<Args>(args)...);
    return process_return<R>(rank, bool_tuple<pass_back_false<Args>::value...>{}, std::forward<Args>(args)...);
}

template<typename MessageInterface>
template<typename R, typename... Args>
R mpirpc::manager<MessageInterface>::invoke_function_pr(int rank, FnHandle functionHandle, Args&&... args)
{
    send_function_invocation(rank, functionHandle, true, std::forward<Args>(args)...);
    return process_return<R>(rank, bool_tuple<is_pass_back<Args>::value...>{}, std::forward<Args>(args)...);
}

/*************************************************************************************/
/*                   Non-Typesafe Non-Returning Non-Member Invoker                   */
/*************************************************************************************/

template<typename MessageInterface>
template<typename... Args>
void mpirpc::manager<MessageInterface>::invoke_function(int rank, FnHandle functionHandle, Args&&... args)
{
    send_function_invocation(rank, functionHandle, false, std::forward<Args>(args)...);
}

/*************************************************************************************/
/*                      Type Casting Returning Member Invokers                       */
/*************************************************************************************/

namespace detail {

template<typename... FArgs>
struct forward_parameter_type_helper
{
    template<std::size_t Index, typename...Args>
    auto get(Args&&...args)
    {
        return std::get<Index>(std::make_tuple(args...));
    }
};
};

template<typename MessageInterface>
template<typename Lambda, typename... Args>
auto mpirpc::manager<MessageInterface>::invoke_lambda_r(int rank, Lambda&& l, FnHandle functionHandle, Args&&... args)
    -> typename LambdaTraits<Lambda>::return_type
{
    using R = typename LambdaTraits<Lambda>::return_type;

    using pass_backs = typename LambdaTraits<Lambda>::pass_backs;
    assert(functionHandle != 0);
    using args_tuple = typename detail::marshaller_function_signature<typename LambdaTraits<Lambda>::lambda_stdfunction,Args...>::parameter_types;
    using ptr_type = typename LambdaTraits<Lambda>::lambda_fnPtr;
    send_lambda_invocation(rank, functionHandle, true, (ptr_type)nullptr, std::forward<Args>(args)...);
    return process_return<R>(rank, pass_backs{}, std::forward<Args>(args)...);
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
auto mpirpc::manager<MessageInterface>::invoke_function_r(object_wrapper_base *a, Args&&... args)
    -> typename detail::marshaller_function_signature<F,Args...>::return_type
{
    if (a->rank() == m_rank)
    {
        using Class = typename storage_function_parts<F>::class_type;
        object_wrapper<Class> *o = static_cast<object_wrapper<Class>*>(a);
        return CALL_MEMBER_FN(*o->object(),f)(std::forward<Args>(args)...);
    } else {
        send_member_function_invocation<F,f>(a, true, std::forward<Args>(args)...);
        return process_return<typename detail::marshaller_function_signature<F,Args...>::return_type>(a->rank(), typename detail::marshaller_function_signature<F,Args...>::pass_backs{}, std::forward<Args>(args)...);
    }
}

/*************************************************************************************/
/*                    Type Casting Non-Returning Member Invokers                     */
/*************************************************************************************/

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::manager<MessageInterface>::invoke_function(object_wrapper_base *a, Args&&... args)
{
    if (a->rank() == m_rank)
    {
        using Class = typename storage_function_parts<F>::class_type;
        object_wrapper<Class> *o = static_cast<object_wrapper<Class>*>(a);
        CALL_MEMBER_FN(*o->object(),f)(std::forward<Args>(args)...);
    } else {
        send_member_function_invocation<F,f>(a, false, std::forward<Args>(args)...);
    }
}

/*************************************************************************************/
/*                                 Send Invocations                                  */
/*************************************************************************************/

template<typename MessageInterface>
template<typename... Args>
void mpirpc::manager<MessageInterface>::send_function_invocation(int rank, FnHandle function_handle, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    ParameterStream stream(buffer);
    stream << function_handle << get_return;
    using swallow = int[];
    (void)swallow{(marshal(stream,std::forward<Args>(args)), 0)...};
    send_raw_message(rank, stream.dataVector(), MPIRPC_TAG_INVOKE);
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::manager<MessageInterface>::send_function_invocation(int rank, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    ParameterStream stream(buffer);
    FnHandle function_handle = this->get_fn_handle<F,f>();
    stream << function_handle << get_return;
    detail::fn_type_marshaller<F>::marshal(stream, std::forward<Args>(args)...);
    send_raw_message(rank, stream.dataVector(), MPIRPC_TAG_INVOKE);
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::manager<MessageInterface>::send_member_function_invocation(object_wrapper_base* a, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    ParameterStream stream(buffer);
    stream << a->type() << a->id();
    stream << this->get_fn_handle<F,f>()<< get_return;
    detail::fn_type_marshaller<F>::marshal(stream, std::forward<Args>(args)...);
    send_raw_message(a->rank(), stream.dataVector(), MPIRPC_TAG_INVOKE_MEMBER);
}

template<typename MessageInterface>
template<typename R, typename...FArgs, typename...Args>
void mpirpc::manager<MessageInterface>::send_lambda_invocation(int rank, mpirpc::FnHandle function_handle, bool get_return, R(*)(FArgs...), Args&&... args)
{
    send_function_invocation(rank, function_handle, get_return, detail::forward_parameter_type<FArgs>(args)...);
}

#endif /* MANAGER_INVOKE_H */
