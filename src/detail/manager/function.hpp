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

template<typename Allocator, typename Buffer, typename MessageInterface>
class mpirpc::manager<Allocator,Buffer,MessageInterface>::function_base
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
    virtual void execute(Buffer& params, Allocator& a, int sender_rank, manager *manager, bool get_return = false, void* object = 0) = 0;

    FnHandle id() const { return m_id; }
    generic_fp_type pointer() const { return m_pointer; }

    virtual ~function_base() {}

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

template<typename Allocator, typename Buffer, typename MessageInterface>
FnHandle mpirpc::manager<Allocator,Buffer,MessageInterface>::function_base::id_counter_ = 0;

template<typename Allocator, typename Buffer, typename MessageInterface>
template<typename R, typename... Args>
class mpirpc::manager<Allocator,Buffer,MessageInterface>::function<R(*)(Args...)> : public function_base
{
    using mpirpc::manager<Allocator,Buffer,MessageInterface>::function_base::m_pointer;
public:
    using function_type = internal::unwrapped_function_type<R(*)(Args...)>;

    function(function_type f) : function_base(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

    virtual void execute(Buffer& params, Allocator& a, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
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
        std::vector<char,Allocator> data(a);
        Buffer outbuffer(&data);
        internal::apply(func,a,params,outbuffer,get_return);
        if (get_return)
            manager->function_return(sender_rank, std::move(outbuffer));
    }

protected:
    function_type func;
};

template<typename Allocator, typename Buffer, typename MessageInterface>
template<typename Class, typename R, typename... Args>
class mpirpc::manager<Allocator, Buffer, MessageInterface>::function<R(Class::*)(Args...)> : public function_base
{
public:
    using function_type = internal::unwrapped_function_type<R(Class::*)(Args...)>;

    function(function_type f) : function_base(), func(f) {  }

    virtual void execute(Buffer& params, Allocator& a, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        assert(object);
        assert(manager);
        std::vector<char,Allocator> data(a);
        Buffer outbuffer(&data);
        internal::apply(func,static_cast<Class*>(object),a,params,outbuffer,get_return);
        if (get_return)
            manager->function_return(sender_rank, std::move(outbuffer));
    }

    function_type func;
};

template<typename Allocator, typename Buffer, typename MessageInterface>
template<typename R, typename... Args>
class mpirpc::manager<Allocator, Buffer, MessageInterface>::function<std::function<R(Args...)> > : public function_base
{
public:
    using function_type = internal::unwrapped_function_type<std::function<R(Args...)> >; // std::function<R(Args...)>;

    function(function_type& f) : function_base(), func(f) {}

    virtual void execute(Buffer &params, Allocator& a, int sender_rank, manager *manager, bool get_return = false, void *object = 0) override
    {
        assert(manager);
        std::vector<char,Allocator> data(a);
        Buffer outbuffer(&data);
        internal::apply(func,a,params,outbuffer,get_return);
        if (get_return)
            manager->function_return(sender_rank, std::move(outbuffer));
    }

    function_type func;
};

#endif /* MPIRPC__DETAIL__MANAGER_FUNCTION_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
