#ifndef MANAGER_REGISTER_H
#define MANAGER_REGISTER_H

#include "../../manager.hpp"

/**
 * Manager definitions for functions relating to object, type, and function registration
 * and retrieving those handles
 */

/*************************************************************************************/
/*                              Function Registration                                */
/*************************************************************************************/

template<typename MessageInterface>
template<typename Lambda>
FnHandle mpirpc::manager<MessageInterface>::register_lambda(Lambda&& l)
{
    return register_function(static_cast<typename LambdaTraits<Lambda>::lambda_stdfunction>(l));
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f>
FnHandle mpirpc::manager<MessageInterface>::register_function()
{
    function_base *b = new function<F>(f);
    m_registered_functions[b->id()] = b;
    m_registered_function_typeids[std::type_index(typeid(function_identifier<F,f>))] = b->id();
    return b->id();
}

template<typename MessageInterface>
template<typename F>
FnHandle mpirpc::manager<MessageInterface>::register_function(F f)
{
    function_base *b = new function<F>(f);
    m_registered_functions[b->id()] = b;
    //m_registered_function_typeids[std::type_index(typeid(function_identifier<typename storage_function_parts<F>::function_type,f>))] = b->id();
    return b->id();
}

/*************************************************************************************/
/*                                Type Registration                                  */
/*************************************************************************************/

template<typename MessageInterface>
template<typename T>
TypeId mpirpc::manager<MessageInterface>::register_type()
{
    TypeId id = ++m_next_type_id;
    m_registered_type_ids[std::type_index(typeid(typename std::decay<T>::type))] = id;
    return id;
}

template<typename MessageInterface>
template<typename T>
TypeId mpirpc::manager<MessageInterface>::get_type_id() const
{
    return m_registered_type_ids.at(std::type_index(typeid(typename std::decay<T>::type)));
}

/*************************************************************************************/
/*                               Object Registration                                 */
/*************************************************************************************/

template<typename MessageInterface>
template<class Class>
object_wrapper<Class>* mpirpc::manager<MessageInterface>::register_object(Class *object) {
    object_wrapper<Class> *wrapper = new object_wrapper<Class>(object);
    wrapper->m_rank = m_rank;
    wrapper->m_type = get_type_id<Class>();
    m_registered_objects.push_back(wrapper);
    notify_new_object(wrapper->type(), wrapper->id());
    return wrapper;
}

template<typename MessageInterface>
template<class Class, typename... Args>
object_wrapper_base* mpirpc::manager<MessageInterface>::construct_global_object(int rank, Args&&... args)
{
    object_wrapper_base *wrapper;
    if (rank == m_rank)
    {
        Class *object = new Class(std::forward<Args>(args)...);
        wrapper = new object_wrapper<Class>(object);
        wrapper->m_rank = m_rank;
        wrapper->m_type = get_type_id<Class>();
    }
    else
    {
        wrapper = new object_wrapper_base();
        wrapper->m_rank = rank;
        wrapper->m_type = get_type_id<Class>();
    }
    m_registered_objects.push_back(wrapper);
    return wrapper;
}

template<typename MessageInterface>
void mpirpc::manager<MessageInterface>::register_remote_object(int rank, TypeId type, ObjectId id)
{
    object_wrapper<void> *a = new object_wrapper<void>();
    a->m_id = id;
    a->m_type = type;
    a->m_rank = rank;
    m_registered_objects.push_back(a);
}

template<typename MessageInterface>
object_wrapper_base* mpirpc::manager<MessageInterface>::get_object_of_type(mpirpc::TypeId typeId) const
{
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId)
            return i;
    }
    throw std::out_of_range("Object not found");
}

template<typename MessageInterface>
template<class Class>
std::unordered_set<object_wrapper_base*> mpirpc::manager<MessageInterface>::get_objects_of_type() const
{
    return get_objects_of_type(get_type_id<Class>());
}

template<typename MessageInterface>
template<class Class>
object_wrapper_base* mpirpc::manager<MessageInterface>::get_object_of_type() const
{
    return get_object_of_type(get_type_id<Class>());
}

template<typename MessageInterface>
template<class Class>
object_wrapper_base* mpirpc::manager<MessageInterface>::get_object_of_type(int rank) const
{
    return get_object_of_type(get_type_id<Class>(), rank);
}

template<typename MessageInterface>
object_wrapper_base* mpirpc::manager<MessageInterface>::get_object_of_type(TypeId typeId, int rank) const
{
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId && i->rank() == rank)
            return i;
    }
    throw UnregisteredObjectException();
}

template<typename MessageInterface>
template<class Class>
std::unordered_set<object_wrapper_base*> mpirpc::manager<MessageInterface>::get_objects_of_type(int rank) const
{
    return get_objects_of_type(get_type_id<Class>(), rank);
}

template<typename MessageInterface>
std::unordered_set<object_wrapper_base*> mpirpc::manager<MessageInterface>::get_objects_of_type(TypeId typeId) const
{
    std::unordered_set<object_wrapper_base*> ret;
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId)
            ret.insert(i);
    }
    return ret;
}

template<typename MessageInterface>
std::unordered_set<object_wrapper_base*> mpirpc::manager<MessageInterface>::get_objects_of_type(TypeId typeId, int rank) const
{
    std::unordered_set<object_wrapper_base*> ret;
    for (object_wrapper_base* i : m_registered_objects)
    {
        if (i->type() == typeId && i->rank() == rank)
            ret.insert(i);
    }
    return ret;
}

template<typename MessageInterface>
object_wrapper_base* mpirpc::manager<MessageInterface>::get_object_wrapper(int rank, TypeId tid, ObjectId oid) const {
    for (object_wrapper_base* i : m_registered_objects)
        if (i->type() == tid && i->id() == oid && i->rank() == rank)
            return i;
    throw UnregisteredObjectException();
}

#endif /* MANAGER_REGISTER_H */
