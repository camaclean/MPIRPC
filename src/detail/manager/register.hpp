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

#ifndef MPIRPC__DETAIL__MANAGER_REGISTER_HPP
#define MPIRPC__DETAIL__MANAGER_REGISTER_HPP

#include "../../manager.hpp"
#include "../../internal/function_attributes.hpp"
#include "../../exceptions.hpp"

/**
 * Manager definitions for functions relating to object, type, and function registration
 * and retrieving those handles
 */

/*************************************************************************************/
/*                              Function Registration                                */
/*************************************************************************************/

template<typename Allocator, typename Buffer, typename MessageInterface>
template<typename Lambda>
FnHandle mpirpc::manager<Allocator,Buffer,MessageInterface>::register_lambda(Lambda&& l)
{
    return register_function(static_cast<internal::lambda_stdfunction_type<Lambda>>(l));
}

template<typename Allocator, typename Buffer, typename MessageInterface>
template<typename F, internal::unwrapped_function_type<F> f>
FnHandle mpirpc::manager<Allocator,Buffer,MessageInterface>::register_function()
{
    function_base *b = new function<internal::wrapped_function_type<F>>(f);
    m_registered_functions[b->id()] = b;
    std::cout << "registered: " << b->id() << std::endl;
    m_registered_function_typeids[std::type_index(typeid(internal::function_identifier<internal::wrapped_function_type<F>,f>))] = b->id();
    return b->id();
}

template<typename Allocator, typename Buffer, typename MessageInterface>
template<typename F>
FnHandle mpirpc::manager<Allocator,Buffer,MessageInterface>::register_function(F f)
{
    function_base *b = new function<internal::wrapped_function_type<F>>(f);
    m_registered_functions[b->id()] = b;
    //m_registered_function_typeids[std::type_index(typeid(function_identifier<typename storage_function_parts<F>::function_type,f>))] = b->id();
    return b->id();
}

/*************************************************************************************/
/*                               Object Registration                                 */
/*************************************************************************************/

template<typename Allocator, typename Buffer, typename MessageInterface>
template<class Class>
object_wrapper<Class>* mpirpc::manager<Allocator,Buffer,MessageInterface>::register_object(Class *object) {
    object_wrapper<Class> *wrapper = new object_wrapper<Class>(object);
    wrapper->m_rank = m_rank;
    wrapper->m_type = type_identifier<Class>::id();
    m_registered_objects.push_back(wrapper);
    notify_new_object(wrapper->type(), wrapper->id());
    return wrapper;
}

template<typename Allocator, typename Buffer, typename MessageInterface>
template<class Class, typename... Args>
object_wrapper_base* mpirpc::manager<Allocator,Buffer,MessageInterface>::construct_global_object(int rank, Args&&... args)
{
    object_wrapper_base *wrapper;
    if (rank == m_rank)
    {
        Class *object = new Class(std::forward<Args>(args)...);
        wrapper = new object_wrapper<Class>(object);
        wrapper->m_rank = m_rank;
        wrapper->m_type = type_identifier<Class>::id();
    }
    else
    {
        wrapper = new object_wrapper_base();
        wrapper->m_rank = rank;
        wrapper->m_type = type_identifier<Class>::id();
    }
    m_registered_objects.push_back(wrapper);
    return wrapper;
}

template<typename Allocator, typename Buffer, typename MessageInterface>
void mpirpc::manager<Allocator,Buffer,MessageInterface>::register_remote_object(int rank, TypeId type, ObjectId id)
{
    object_wrapper<void> *a = new object_wrapper<void>();
    a->m_id = id;
    a->m_type = type;
    a->m_rank = rank;
    m_registered_objects.push_back(a);
}

template<typename Allocator, typename Buffer, typename MessageInterface>
object_wrapper_base* mpirpc::manager<Allocator,Buffer,MessageInterface>::get_object_of_type(mpirpc::TypeId typeId) const
{
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId)
            return i;
    }
    throw std::out_of_range("Object not found");
}

template<typename Allocator, typename Buffer, typename MessageInterface>
template<class Class>
std::unordered_set<object_wrapper_base*> mpirpc::manager<Allocator,Buffer,MessageInterface>::get_objects_of_type() const
{
    return get_objects_of_type(type_identifier<Class>::id());
}

template<typename Allocator, typename Buffer, typename MessageInterface>
template<class Class>
object_wrapper_base* mpirpc::manager<Allocator,Buffer,MessageInterface>::get_object_of_type() const
{
    return get_object_of_type(type_identifier<Class>::id());
}

template<typename Allocator, typename Buffer, typename MessageInterface>
template<class Class>
object_wrapper_base* mpirpc::manager<Allocator,Buffer,MessageInterface>::get_object_of_type(int rank) const
{
    return get_object_of_type(type_identifier<Class>::id(), rank);
}

template<typename Allocator, typename Buffer, typename MessageInterface>
object_wrapper_base* mpirpc::manager<Allocator,Buffer,MessageInterface>::get_object_of_type(TypeId typeId, int rank) const
{
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId && i->rank() == rank)
            return i;
    }
    throw mpirpc::unregistered_object_exception();
}

template<typename Allocator, typename Buffer, typename MessageInterface>
template<class Class>
std::unordered_set<object_wrapper_base*> mpirpc::manager<Allocator,Buffer,MessageInterface>::get_objects_of_type(int rank) const
{
    return get_objects_of_type(type_identifier<Class>::id(), rank);
}

template<typename Allocator, typename Buffer, typename MessageInterface>
std::unordered_set<object_wrapper_base*> mpirpc::manager<Allocator,Buffer,MessageInterface>::get_objects_of_type(TypeId typeId) const
{
    std::unordered_set<object_wrapper_base*> ret;
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId)
            ret.insert(i);
    }
    return ret;
}

template<typename Allocator, typename Buffer, typename MessageInterface>
std::unordered_set<object_wrapper_base*> mpirpc::manager<Allocator,Buffer,MessageInterface>::get_objects_of_type(TypeId typeId, int rank) const
{
    std::unordered_set<object_wrapper_base*> ret;
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId && i->rank() == rank)
            ret.insert(i);
    }
    return ret;
}

template<typename Allocator, typename Buffer, typename MessageInterface>
object_wrapper_base* mpirpc::manager<Allocator,Buffer,MessageInterface>::get_object_wrapper(int rank, TypeId tid, ObjectId oid) const {
    for (object_wrapper_base* i : m_registered_objects)
        if (i->type() == tid && i->id() == oid && i->rank() == rank)
            return i;
    throw unregistered_object_exception();
}

#endif /* MPIRPC__DETAIL__MANAGER_REGISTER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
