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
auto mpirpc::Manager<MessageInterface>::invokeFunctionR(int rank, Args&&... args)
    -> typename detail::marshaller_function_signature<F,Args...>::return_type
{
    if (rank == m_rank)
    {
        return f(std::forward<Args>(args)...);
    } else {
        sendFunctionInvocation<F,f>(rank, true, std::forward<Args>(args)...);
        return processReturn<typename detail::marshaller_function_signature<F,Args...>::return_type>(rank);
    }
}

template<typename MessageInterface>
template<typename R, typename... FArgs, typename... Args>
auto mpirpc::Manager<MessageInterface>::invokeFunctionR(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args)
    -> typename std::enable_if<!std::is_same<R, void>::value, R>::type
{
    if (rank == m_rank) {
        return f(std::forward<Args>(args)...);
    } else {
        if (functionHandle == 0)
        {
            for (const auto &i : m_registered_functions) {
                if (i.second->pointer() == reinterpret_cast<void(*)()>(f)) {
                    sendFunctionInvocation(rank, i.first, true, detail::forward_parameter_type<FArgs>(args)...);
                    return processReturn<R>(rank);
                }
            }
        }
        else
        {
            sendFunctionInvocation(rank, functionHandle, true, detail::forward_parameter_type<FArgs>(args)...);
            return processReturn<R>(rank);
        }
        throw UnregisteredFunctionException();
    }
}

/*************************************************************************************/
/*                   Type Casting Non-Returning Non-Member Invokers                  */
/*************************************************************************************/

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::Manager<MessageInterface>::invokeFunction(int rank, Args&&... args)
{
    if (rank == m_rank)
    {
        f(std::forward<Args>(args)...);
    } else {
        sendFunctionInvocation<F,f>(rank, false, std::forward<Args>(args)...);
    }
}

template<typename MessageInterface>
template<typename R, typename... FArgs, typename...Args>
void mpirpc::Manager<MessageInterface>::invokeFunction(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args)
{
    if (rank == m_rank) {
        f(detail::forward_parameter_type<FArgs,Args>(args)...);
    } else {
        if (functionHandle == 0) {
            for (const auto &i : m_registered_functions) {
                if (i.second->pointer() == reinterpret_cast<void(*)()> (f)) {
                    sendFunctionInvocation(rank, i.first, false, detail::forward_parameter_type<FArgs>(args)...);
                    return;
                }
            }
            throw UnregisteredFunctionException();
        }
        else
        {
            sendFunctionInvocation(rank, functionHandle, false, detail::forward_parameter_type<FArgs>(args)...);
        }
    }
}

/*************************************************************************************/
/*                     Non-Typesafe Returning Non-Member Invoker                     */
/*************************************************************************************/

template<typename MessageInterface>
template<typename R, typename... Args>
R mpirpc::Manager<MessageInterface>::invokeFunctionR(int rank, FnHandle functionHandle, Args&&... args)
{
    sendFunctionInvocation(rank, functionHandle, true, std::forward<Args>(args)...);
    return processReturn<R>(rank);
}

/*************************************************************************************/
/*                   Non-Typesafe Non-Returning Non-Member Invoker                   */
/*************************************************************************************/

template<typename MessageInterface>
template<typename... Args>
void mpirpc::Manager<MessageInterface>::invokeFunction(int rank, FnHandle functionHandle, Args&&... args)
{
    sendFunctionInvocation(rank, functionHandle, false, std::forward<Args>(args)...);
}

/*************************************************************************************/
/*                      Type Casting Returning Member Invokers                       */
/*************************************************************************************/

template<typename MessageInterface>
template<typename R, class Class, typename... FArgs, typename... Args>
auto mpirpc::Manager<MessageInterface>::invokeFunctionR(ObjectWrapperBase *a, R(Class::*f)(FArgs...), FnHandle functionHandle, Args&&... args)
    -> typename std::enable_if<!std::is_same<R, void>::value, R>::type
{
    if (a->rank() == m_rank)
    {
        ObjectWrapper<Class> *o = static_cast<ObjectWrapper<Class>*>(a);
        return CALL_MEMBER_FN(*o->object(),f)(detail::forward_parameter_type<FArgs>(args)...);
    } else {
        if (functionHandle == 0)
        {
            for (const auto &i : m_registered_functions)
            {
                Function<R(Class::*)(FArgs...)>* func = dynamic_cast<Function<R(Class::*)(FArgs...)>*>(i.second);
                if (func) {
                    if (func->func == f)
                    {
                        sendMemberFunctionInvocation(a, func->id(), true, detail::forward_parameter_type<FArgs>(args)...);
                        return processReturn<R>(a->rank());
                    }
                }
            }
            throw UnregisteredFunctionException();
        }
        else
        {
            sendMemberFunctionInvocation(a, functionHandle, true, detail::forward_parameter_type<FArgs>(args)...);
            return processReturn<R>(a->rank());
        }
    }
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
auto mpirpc::Manager<MessageInterface>::invokeFunctionR(ObjectWrapperBase *a, Args&&... args)
    -> typename detail::marshaller_function_signature<F,Args...>::return_type
{
    if (a->rank() == m_rank)
    {
        using Class = typename storage_function_parts<F>::class_type;
        ObjectWrapper<Class> *o = static_cast<ObjectWrapper<Class>*>(a);
        return CALL_MEMBER_FN(*o->object(),f)(std::forward<Args>(args)...);
    } else {
        sendMemberFunctionInvocation<F,f>(a, true, std::forward<Args>(args)...);
        return processReturn<typename detail::marshaller_function_signature<F,Args...>::return_type>(a->rank());
    }
}

/*************************************************************************************/
/*                    Type Casting Non-Returning Member Invokers                     */
/*************************************************************************************/

template<typename MessageInterface>
template<typename R, class Class, typename... FArgs, typename... Args>
void mpirpc::Manager<MessageInterface>::invokeFunction(ObjectWrapperBase *a, R(Class::*f)(FArgs...), FnHandle functionHandle, Args&&... args)
{
    if (a->rank() == m_rank)
    {
        ObjectWrapper<Class> *o = static_cast<ObjectWrapper<Class>*>(a);
        CALL_MEMBER_FN(*o->object(),f)(detail::forward_parameter_type<FArgs>(args)...);
    } else {
        if (functionHandle == 0)
        {
            for (const auto &i : m_registered_functions)
            {
                Function<R(Class::*)(FArgs...)>* func = dynamic_cast<Function<R(Class::*)(FArgs...)>*>(i.second);
                if (func) {
                    if (func->func == f)
                    {
                        sendMemberFunctionInvocation(a, func->id(), false, detail::forward_parameter_type<FArgs>(args)...);
                        return;
                    }
                }
            }
            throw UnregisteredFunctionException();
        }
        else
        {
            sendMemberFunctionInvocation(a, functionHandle, false, detail::forward_parameter_type<FArgs>(args)...);
        }
    }
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::Manager<MessageInterface>::invokeFunction(ObjectWrapperBase *a, Args&&... args)
{
    if (a->rank() == m_rank)
    {
        using Class = typename storage_function_parts<F>::class_type;
        ObjectWrapper<Class> *o = static_cast<ObjectWrapper<Class>*>(a);
        CALL_MEMBER_FN(*o->object(),f)(std::forward<Args>(args)...);
    } else {
        sendMemberFunctionInvocation<F,f>(a, false, std::forward<Args>(args)...);
    }
}

/*************************************************************************************/
/*                                 Send Invocations                                  */
/*************************************************************************************/

template<typename MessageInterface>
template<typename... Args>
void mpirpc::Manager<MessageInterface>::sendFunctionInvocation(int rank, FnHandle function_handle, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    ParameterStream stream(buffer);
    stream << function_handle << get_return;
    using swallow = int[];
    (void)swallow{(marshal(stream,std::forward<Args>(args)), 0)...};
    sendRawMessage(rank, stream.dataVector(), MPIRPC_TAG_INVOKE);
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::Manager<MessageInterface>::sendFunctionInvocation(int rank, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    ParameterStream stream(buffer);
    FnHandle function_handle = this->get_fn_handle<F,f>();
    stream << function_handle << get_return;
    detail::fn_type_marshaller<F>::marshal(stream, std::forward<Args>(args)...);
    sendRawMessage(rank, stream.dataVector(), MPIRPC_TAG_INVOKE);
}

template<typename MessageInterface>
template<typename... Args>
void mpirpc::Manager<MessageInterface>::sendMemberFunctionInvocation(ObjectWrapperBase* a, FnHandle function_handle, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    ParameterStream stream(buffer);
    stream << a->type() << a->id();
    stream << function_handle << get_return;
    Passer p{(marshal(stream,std::forward<Args>(args)), 0)...};
    sendRawMessage(a->rank(), stream.dataVector(), MPIRPC_TAG_INVOKE_MEMBER);
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
void mpirpc::Manager<MessageInterface>::sendMemberFunctionInvocation(ObjectWrapperBase* a, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    ParameterStream stream(buffer);
    stream << a->type() << a->id();
    stream << this->get_fn_handle<F,f>()<< get_return;
    detail::fn_type_marshaller<F>::marshal(stream, std::forward<Args>(args)...);
    sendRawMessage(a->rank(), stream.dataVector(), MPIRPC_TAG_INVOKE_MEMBER);
}

#endif /* MANAGER_INVOKE_H */
