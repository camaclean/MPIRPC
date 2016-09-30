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

#ifndef MPIRPC__DETAIL__MANAGER_FUNCTION_HPP
#define MPIRPC__DETAIL__MANAGER_FUNCTION_HPP

#include "../../manager.hpp"
#include "../../internal/function_attributes.hpp"
#include "../../internal/pass_back.hpp"

template<class MessageInterface, template<typename> typename Allocator>
class mpirpc::manager<MessageInterface, Allocator>::function_base
{
public:
    using generic_fp_type = void(*)();

    function_base() : m_id(make_id()), m_pointer(0) {}

    /**
        * @brief execute Execute the function
        * @param params The serialized function parameters
        * @param sender_rank The rank requesting this function be invoked
        * @param manager The manager, when sending back the function return value
        * @param get_return Only send the function return value back to the sending rank when requested, as the return value may not be needed.
        * @param object When the function is a member function, use object as the <i>this</i> pointer.
        */
    virtual void execute(::mpirpc::parameter_stream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) = 0;

    FnHandle id() const { return m_id; }
    generic_fp_type pointer() const { return m_pointer; }

    virtual ~function_base() {};

private:
    static FnHandle make_id() {
        return ++id_counter_;
    }

    FnHandle m_id;
    static FnHandle id_counter_;
protected:
    generic_fp_type m_pointer;
    std::function<void()> m_function;
};

template<typename MessageInterface, template<typename> typename Allocator>
FnHandle mpirpc::manager<MessageInterface, Allocator>::function_base::id_counter_ = 0;

template<class MessageInterface, template<typename> typename Allocator>
template<typename R, typename... Args>
class mpirpc::manager<MessageInterface, Allocator>::function<R(*)(Args...)> : public function_base
{
    using mpirpc::manager<MessageInterface, Allocator>::function_base::m_pointer;
public:
    using function_type = internal::unwrapped_function_type<R(*)(Args...)>;

    function(function_type f) : function_base(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

    virtual void execute(parameter_stream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        /*
            * func(convertData<Args>(data)...) does not work here
            * due to convertData<T>(const char *data) being evaluated
            * in an undefined order,. The side effects matter. A workaround
            * is to use uniform initilization of a struct that binds the
            * parameters. Parameter packs are expanded as comma separated,
            * but the commas cannot be used as comma operators.
            */
        assert(manager);
        ordered_call<R(*)(Args...)> call{func, unmarshal<internal::remove_all_const_type<Args>, Allocator<Args>>(params)...};
        if (get_return)
        {
            R ret(call());
            manager->function_return(sender_rank, std::move(ret), call.args_tuple, internal::bool_template_list<internal::is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
        }
        else
        {
            call();
        }
    }

protected:
    function_type func;
};

template<class MessageInterface, template<typename> typename Allocator>
template<typename... Args>
class mpirpc::manager<MessageInterface, Allocator>::function<void(*)(Args...)> : public function_base
{
    using mpirpc::manager<MessageInterface, Allocator>::function_base::m_pointer;
public:
    using function_type = internal::unwrapped_function_type<void(*)(Args...)>;//void(*)(Args...);

    function(function_type f) : function_base(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

    virtual void execute(::mpirpc::parameter_stream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        ordered_call<void(*)(Args...)> call{func, unmarshal<internal::remove_all_const_type<Args>, Allocator<Args>>(params)...};
        call();

        if (get_return)
            manager->function_return(sender_rank, call.args_tuple, internal::bool_template_list<internal::is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
    }

protected:
    function_type func;
};

template<typename MessageInterface, template<typename> typename Allocator>
template<typename Class, typename R, typename... Args>
class mpirpc::manager<MessageInterface, Allocator>::function<R(Class::*)(Args...)> : public function_base
{
public:
    using function_type = internal::unwrapped_function_type<R(Class::*)(Args...)>;

    function(function_type f) : function_base(), func(f) {  }

    virtual void execute(::mpirpc::parameter_stream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        assert(object);
        assert(manager);
        ordered_call<R(Class::*)(Args...)> call{func, static_cast<Class*>(object), unmarshal<internal::remove_all_const_type<Args>, Allocator<Args>>(params)...};
        if (get_return)
        {
            R ret(call());
            manager->function_return(sender_rank, std::move(ret), call.args_tuple, internal::bool_template_list<internal::is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
        }
        else
        {
            call();
        }
    }

    function_type func;
};

template<typename MessageInterface, template<typename> typename Allocator>
template<typename Class, typename... Args>
class mpirpc::manager<MessageInterface, Allocator>::function<void(Class::*)(Args...)> : public function_base
{
public:
    using function_type = internal::unwrapped_function_type<void(Class::*)(Args...)>;

    function(function_type f) : function_base(), func(f) { }

    virtual void execute(::mpirpc::parameter_stream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        assert(object);
        ordered_call<void(Class::*)(Args...)> call{func, static_cast<Class*>(object), unmarshal<internal::remove_all_const_type<Args>, Allocator<Args>>(params)...};
        call();
        if (get_return)
            manager->function_return(sender_rank, call.args_tuple, internal::bool_template_list<internal::is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
    }

    function_type func;
};

template<typename MessageInterface, template<typename> typename Allocator>
template<typename R, typename... Args>
class mpirpc::manager<MessageInterface, Allocator>::function<std::function<R(Args...)>> : public function_base
{
public:
    using function_type = internal::unwrapped_function_type<std::function<R(Args...)>>; // std::function<R(Args...)>;

    function(function_type& f) : function_base(), func(f) {}

    virtual void execute(::mpirpc::parameter_stream &params, int sender_rank, manager *manager, bool get_return, void *object = 0) override
    {
        assert(manager);
        ordered_call<std::function<R(Args...)>> call{func, unmarshal<internal::remove_all_const_type<Args>,Allocator<Args>>(params)...};

        if (get_return)
        {
            R ret(call());
            manager->function_return(sender_rank, std::move(ret), call.args_tuple, internal::bool_template_list<internal::is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
        }
        else
        {
            call();
        }
    }

    function_type func;
};

template<typename MessageInterface, template<typename> typename Allocator>
template<typename... Args>
class mpirpc::manager<MessageInterface, Allocator>::function<std::function<void(Args...)>> : public function_base
{
public:
    using function_type = internal::unwrapped_function_type<std::function<void(Args...)>>;

    function(function_type& f) : function_base(), func(f) {}

    virtual void execute(::mpirpc::parameter_stream &params, int sender_rank, manager *manager, bool get_return, void *object = 0) override
    {
        ordered_call<std::function<void(Args...)>> call{func, unmarshal<internal::remove_all_const_type<Args>, Allocator<Args>>(params)...};
        call();
        if (get_return)
            manager->function_return(sender_rank, call.args_tuple, internal::bool_template_list<internal::is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
    }

    function_type func;
};

#endif /* MPIRPC__DETAIL__MANAGER_FUNCTION_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
