#ifndef MANAGER_REGISTER_H
#define MANAGER_REGISTER_H

template<typename MessageInterface>
template<typename T>
TypeId Manager<MessageInterface>::registerType()
{
    TypeId id = ++m_nextTypeId;
    m_registeredTypeIds[std::type_index(typeid(typename std::decay<T>::type))] = id;
    return id;
}

template<typename MessageInterface>
template<typename T>
TypeId Manager<MessageInterface>::getTypeId() const
{
    TypeId id = m_registeredTypeIds.at(std::type_index(typeid(typename std::decay<T>::type)));
    return id;
}

template<typename MessageInterface>
template<typename Lambda>
fnhandle_t Manager<MessageInterface>::registerLambda(Lambda&& l)
{
    return registerFunction(static_cast<typename LambdaTraits<Lambda>::lambda_stdfunction>(l));
}

template<typename MessageInterface>
template<typename F, typename storage_function_parts<F>::function_type f>
fnhandle_t Manager<MessageInterface>::registerFunction()
{
    FunctionBase *b = new Function<F>(f);
    m_registered_functions[b->id()] = b;
    m_registered_function_typeids[std::type_index(typeid(function_identifier<F,f>))] = b->id();
    return b->id();
}

#endif /* MANAGER_REGISTER_H */
