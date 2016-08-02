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
FnHandle mpirpc::Manager<MessageInterface>::registerLambda(Lambda&& l)
{
    return registerFunction(static_cast<typename LambdaTraits<Lambda>::lambda_stdfunction>(l));
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f>
FnHandle mpirpc::Manager<MessageInterface>::registerFunction()
{
    FunctionBase *b = new Function<F>(f);
    m_registered_functions[b->id()] = b;
    m_registered_function_typeids[std::type_index(typeid(function_identifier<F,f>))] = b->id();
    return b->id();
}

template<typename MessageInterface>
template<typename F>
FnHandle mpirpc::Manager<MessageInterface>::registerFunction(F f)
{
    FunctionBase *b = new Function<F>(f);
    m_registered_functions[b->id()] = b;
    //m_registered_function_typeids[std::type_index(typeid(function_identifier<typename storage_function_parts<F>::function_type,f>))] = b->id();
    return b->id();
}

/*************************************************************************************/
/*                                Type Registration                                  */
/*************************************************************************************/

template<typename MessageInterface>
template<typename T>
TypeId mpirpc::Manager<MessageInterface>::registerType()
{
    TypeId id = ++m_nextTypeId;
    m_registeredTypeIds[std::type_index(typeid(typename std::decay<T>::type))] = id;
    return id;
}

template<typename MessageInterface>
template<typename T>
TypeId mpirpc::Manager<MessageInterface>::getTypeId() const
{
    return m_registeredTypeIds.at(std::type_index(typeid(typename std::decay<T>::type)));
}

/*************************************************************************************/
/*                               Object Registration                                 */
/*************************************************************************************/

template<typename MessageInterface>
template<class Class>
ObjectWrapper<Class>* mpirpc::Manager<MessageInterface>::registerObject(Class *object) {
    ObjectWrapper<Class> *wrapper = new ObjectWrapper<Class>(object);
    wrapper->m_rank = m_rank;
    wrapper->m_type = getTypeId<Class>();
    m_registeredObjects.push_back(wrapper);
    notifyNewObject(wrapper->type(), wrapper->id());
    return wrapper;
}

template<typename MessageInterface>
template<class Class, typename... Args>
ObjectWrapperBase* mpirpc::Manager<MessageInterface>::constructGlobalObject(int rank, Args&&... args)
{
    ObjectWrapperBase *wrapper;
    if (rank == m_rank)
    {
        Class *object = new Class(std::forward<Args>(args)...);
        wrapper = new ObjectWrapper<Class>(object);
        wrapper->m_rank = m_rank;
        wrapper->m_type = getTypeId<Class>();
    }
    else
    {
        wrapper = new ObjectWrapperBase();
        wrapper->m_rank = rank;
        wrapper->m_type = getTypeId<Class>();
    }
    m_registeredObjects.push_back(wrapper);
    return wrapper;
}

template<typename MessageInterface>
void mpirpc::Manager<MessageInterface>::registerRemoteObject(int rank, TypeId type, ObjectId id)
{
    ObjectWrapper<void> *a = new ObjectWrapper<void>();
    a->m_id = id;
    a->m_type = type;
    a->m_rank = rank;
    m_registeredObjects.push_back(a);
}

template<typename MessageInterface>
ObjectWrapperBase* mpirpc::Manager<MessageInterface>::getObjectOfType(mpirpc::TypeId typeId) const
{
    for (ObjectWrapperBase* i : m_registeredObjects)
    {
        if (i->type() == typeId)
            return i;
    }
    throw std::out_of_range("Object not found");
}

template<typename MessageInterface>
template<class Class>
std::unordered_set<ObjectWrapperBase*> mpirpc::Manager<MessageInterface>::getObjectsOfType() const
{
    return getObjectsOfType(getTypeId<Class>());
}

template<typename MessageInterface>
template<class Class>
ObjectWrapperBase* mpirpc::Manager<MessageInterface>::getObjectOfType() const
{
    return getObjectOfType(getTypeId<Class>());
}

template<typename MessageInterface>
template<class Class>
ObjectWrapperBase* mpirpc::Manager<MessageInterface>::getObjectOfType(int rank) const
{
    return getObjectOfType(getTypeId<Class>(), rank);
}

template<typename MessageInterface>
ObjectWrapperBase* mpirpc::Manager<MessageInterface>::getObjectOfType(TypeId typeId, int rank) const
{
    for (ObjectWrapperBase* i : m_registeredObjects)
    {
        if (i->type() == typeId && i->rank() == rank)
            return i;
    }
    throw UnregisteredObjectException();
}

template<typename MessageInterface>
template<class Class>
std::unordered_set<ObjectWrapperBase*> mpirpc::Manager<MessageInterface>::getObjectsOfType(int rank) const
{
    return getObjectsOfType(getTypeId<Class>(), rank);
}

template<typename MessageInterface>
std::unordered_set<ObjectWrapperBase*> mpirpc::Manager<MessageInterface>::getObjectsOfType(TypeId typeId) const
{
    std::unordered_set<ObjectWrapperBase*> ret;
    for (ObjectWrapperBase* i : m_registeredObjects)
    {
        if (i->type() == typeId)
            ret.insert(i);
    }
    return ret;
}

template<typename MessageInterface>
std::unordered_set<ObjectWrapperBase*> mpirpc::Manager<MessageInterface>::getObjectsOfType(TypeId typeId, int rank) const
{
    std::unordered_set<ObjectWrapperBase*> ret;
    for (ObjectWrapperBase* i : m_registeredObjects)
    {
        if (i->type() == typeId && i->rank() == rank)
            ret.insert(i);
    }
    return ret;
}

template<typename MessageInterface>
ObjectWrapperBase* mpirpc::Manager<MessageInterface>::getObjectWrapper(int rank, TypeId tid, ObjectId oid) const {
    for (ObjectWrapperBase* i : m_registeredObjects)
        if (i->type() == tid && i->id() == oid && i->rank() == rank)
            return i;
    throw UnregisteredObjectException();
}

#endif /* MANAGER_REGISTER_H */
