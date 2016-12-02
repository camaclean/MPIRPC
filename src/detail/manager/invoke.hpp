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

#ifndef MPIRPC__DETAIL__MANAGER_INVOKE_HPP
#define MPIRPC__DETAIL__MANAGER_INVOKE_HPP

#include "../../manager.hpp"

/*************************************************************************************/
/*************************************************************************************/
/*                              Public Invocation API                                */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*                    Type Casting Returning Non-Member Invokers                     */
/*************************************************************************************/

template<typename MessageInterface, template<typename> typename Allocator>
template<typename F, typename ::mpirpc::internal::unwrapped_function_type<F> f, typename... Args>
auto mpirpc::manager<MessageInterface, Allocator>::invoke_function_r(int rank, Args&&... args)
    -> internal::function_return_type<F>
{
    if (rank == m_rank)
    {
        return f(std::forward<Args>(args)...);
    } else {
        send_function_invocation<internal::wrapped_function_type<F>,f>(rank, true, internal::autowrap<Args>(args)...);
        return process_return<internal::function_return_type<F>>(rank,
                                                                 typename internal::marshaller_function_signature<internal::wrapped_function_type<F>,internal::autowrapped_type<Args>...>::parameter_types{},
                                                                 //typename internal::marshaller_function_signature<internal::wrapped_function_type<F>,internal::autowrapped_type<Args>...>::pass_backs{},
                                                                 std::forward<Args>(args)...);
    }
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename R, typename... FArgs, typename... Args>
auto mpirpc::manager<MessageInterface, Allocator>::invoke_function_r(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args)
    -> typename std::enable_if<!std::is_same<R, void>::value, R>::type
{
    using pass_backs = internal::type_pack<internal::autowrapped_type<FArgs>...>;//internal::bool_template_list<internal::is_pass_back<FArgs>::value...>;
    if (rank == m_rank) {
        return f(std::forward<Args>(args)...);
    } else {
        if (functionHandle == 0)
        {
            for (const auto &i : m_registered_functions) {
                if (i.second->pointer() == reinterpret_cast<void(*)()>(f)) {
                    send_function_invocation(rank, i.first, true, internal::forward_parameter<FArgs>(args)...);
                    return process_return<R>(rank, pass_backs{}, std::forward<Args>(args)...);
                }
            }
        }
        else
        {
            send_function_invocation(rank, functionHandle, true, internal::forward_parameter<FArgs>(args)...);
            return process_return<R>(rank, pass_backs{}, std::forward<Args>(args)...);
        }
        throw unregistered_function_exception();
    }
}

/*************************************************************************************/
/*                   Type Casting Non-Returning Non-Member Invokers                  */
/*************************************************************************************/

template<typename MessageInterface, template<typename> typename Allocator>
template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
void mpirpc::manager<MessageInterface, Allocator>::invoke_function(int rank, Args&&... args)
{
    if (rank == m_rank)
    {
        f(std::forward<Args>(args)...);
    } else {
        send_function_invocation<internal::wrapped_function_type<F>,f>(rank, false, internal::autowrap<Args>(args)...);
    }
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename R, typename... FArgs, typename...Args>
void mpirpc::manager<MessageInterface, Allocator>::invoke_function(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args)
{
    if (rank == m_rank) {
        f(internal::forward_parameter<FArgs,Args>(args)...);
    } else {
        if (functionHandle == 0) {
            for (const auto &i : m_registered_functions) {
                if (i.second->pointer() == reinterpret_cast<void(*)()> (f)) {
                    send_function_invocation(rank, i.first, false, internal::forward_parameter<FArgs>(args)...);
                    return;
                }
            }
            throw unregistered_function_exception();
        }
        else
        {
            send_function_invocation(rank, functionHandle, false, internal::forward_parameter<FArgs>(args)...);
        }
    }
}

/*************************************************************************************/
/*                     Non-Typesafe Returning Non-Member Invoker                     */
/*************************************************************************************/

template<typename MessageInterface, template<typename> typename Allocator>
template<typename R, typename... Args>
R mpirpc::manager<MessageInterface, Allocator>::invoke_function_r(int rank, FnHandle functionHandle, Args&&... args)
{
    send_function_invocation(rank, functionHandle, true, internal::autowrap<Args>(args)...);
    return process_return<R>(rank, internal::type_pack<typename internal::pass_back_false<Args>::type...>{}, std::forward<Args>(args)...);
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename R, typename... Args>
R mpirpc::manager<MessageInterface, Allocator>::invoke_function_pr(int rank, FnHandle functionHandle, Args&&... args)
{
    send_function_invocation(rank, functionHandle, true, internal::autowrap<Args>(args)...);
    return process_return<R>(rank, internal::type_pack<internal::autowrapped_type<Args>...>{}, std::forward<Args>(args)...);
}

/*************************************************************************************/
/*                   Non-Typesafe Non-Returning Non-Member Invoker                   */
/*************************************************************************************/

template<typename MessageInterface, template<typename> typename Allocator>
template<typename... Args>
void mpirpc::manager<MessageInterface, Allocator>::invoke_function(int rank, FnHandle functionHandle, Args&&... args)
{
    send_function_invocation(rank, functionHandle, false, internal::autowrap<Args>(args)...);
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
        return std::get<Index>(std::make_tuple(std::forward<Args>(args)...));
    }
};
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename Lambda, typename... Args>
auto mpirpc::manager<MessageInterface, Allocator>::invoke_lambda_r(int rank, Lambda&& l, FnHandle function_handle, Args&&... args)
    -> internal::lambda_return_type<Lambda>
{
    using R = internal::lambda_return_type<Lambda>;

    using pass_backs = typename internal::lambda_traits<Lambda>::parameter_types;
    assert(function_handle != 0);
    send_lambda_invocation(rank, function_handle, true, (internal::lambda_fn_ptr_type<Lambda>)nullptr, internal::autowrap<Args>(args)...);
    return process_return<R>(rank, pass_backs{}, internal::autowrap<Args>(args)...);
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
auto mpirpc::manager<MessageInterface, Allocator>::invoke_function_r(object_wrapper_base *a, Args&&... args)
    -> internal::function_return_type<F>
{
    if (a->rank() == m_rank)
    {
        using Class = typename internal::wrapped_function_parts<F>::class_type;
        object_wrapper<Class> *o = static_cast<object_wrapper<Class>*>(a);
        return CALL_MEMBER_FN(*o->object(),f)(std::forward<Args>(args)...);
    } else {
        send_member_function_invocation<internal::wrapped_function_type<F>,f>(a, true, internal::autowrap<Args>(args)...);
        return process_return<internal::function_return_type<F>>(a->rank(), typename internal::marshaller_function_signature<F,Args...>::parameter_types{}, std::forward<Args>(args)...);
    }
}

/*************************************************************************************/
/*                    Type Casting Non-Returning Member Invokers                     */
/*************************************************************************************/

template<typename MessageInterface, template<typename> typename Allocator>
template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
void mpirpc::manager<MessageInterface, Allocator>::invoke_function(object_wrapper_base *a, Args&&... args)
{
    if (a->rank() == m_rank)
    {
        using Class = typename internal::wrapped_function_parts<F>::class_type;
        object_wrapper<Class> *o = static_cast<object_wrapper<Class>*>(a);
        CALL_MEMBER_FN(*o->object(),f)(std::forward<Args>(args)...);
    } else {
        send_member_function_invocation<internal::wrapped_function_type<F>,f>(a, false, internal::autowrap<Args>(args)...);
    }
}

/*************************************************************************************/
/*                                 Send Invocations                                  */
/*************************************************************************************/

template<typename MessageInterface, template<typename> typename Allocator>
template<typename... Args>
void mpirpc::manager<MessageInterface, Allocator>::send_function_invocation(int rank, FnHandle function_handle, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    parameter_stream stream(buffer);
    stream << function_handle << get_return;
    using swallow = int[];
    (void)swallow{(marshal(stream,std::forward<Args>(args)), 0)...};
    send_raw_message(rank, stream.dataVector(), MPIRPC_TAG_INVOKE);
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
void mpirpc::manager<MessageInterface, Allocator>::send_function_invocation(int rank, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    parameter_stream stream(buffer);
    FnHandle function_handle = this->get_fn_handle<F,f>();
    stream << function_handle << get_return;
    internal::fn_type_marshaller<F>::marshal(stream, std::forward<Args>(args)...);
    send_raw_message(rank, stream.dataVector(), MPIRPC_TAG_INVOKE);
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
void mpirpc::manager<MessageInterface, Allocator>::send_member_function_invocation(object_wrapper_base* a, bool get_return, Args&&... args)
{
    std::vector<char>* buffer = new std::vector<char>();
    parameter_stream stream(buffer);
    stream << a->type() << a->id();
    stream << this->get_fn_handle<F,f>()<< get_return;
    internal::fn_type_marshaller<F>::marshal(stream, std::forward<Args>(args)...);
    send_raw_message(a->rank(), stream.dataVector(), MPIRPC_TAG_INVOKE_MEMBER);
}

template<typename MessageInterface, template<typename> typename Allocator>
template<typename R, typename...FArgs, typename...Args>
void ::mpirpc::manager<MessageInterface, Allocator>::send_lambda_invocation(int rank, mpirpc::FnHandle function_handle, bool get_return, R(*)(FArgs...), Args&&... args)
{
    send_function_invocation(rank, function_handle, get_return, internal::forward_parameter<FArgs>(args)...);
}

#endif /* MPIRPC__DETAIL__MANAGER_INVOKE_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
