/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014  Colin MacLean <s0838159@sms.ed.ac.uk>
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

#ifndef MPIRPCMANAGER_H
#define MPIRPCMANAGER_H

#include <iostream>
#include <utility>
#include <typeinfo>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <tuple>
#include <map>
#include <stdexcept>
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <typeindex>
#include <sstream>
#include <vector>
#include <iterator>
#include <exception>
#include <algorithm>
#include <thread>
#include <mpi.h>

#include "objectwrapper.hpp"
#include "lambda.hpp"
#include "orderedcall.hpp"
#include "common.hpp"
#include "parameterstream.hpp"
#include "mpitype.hpp"

#define ERR_ASSERT     1
#define ERR_MAX_ACTORS 2
#define MAX_GENERIC_ARGUMENTS 20
#define MAX_MPI_QUEUE 10

#define MPIRPC_TAG_NEW 1
#define MPIRPC_TAG_SHUTDOWN 2
#define MPIRPC_TAG_INVOKE 3
#define MPIRPC_TAG_INVOKE_MEMBER 4
#define MPIRPC_TAG_RETURN 5

#define CALL_MEMBER_FN(object,ptr) ((object).*(ptr))

namespace mpirpc {

struct UnregisteredFunctionException : std::exception
{
    const char* what() const noexcept override
    {
        return "Unregistered Function\n";
    }
};

struct UnregisteredObjectException : std::exception
{
    const char* what() const noexcept override
    {
        return "Unregistered Object\n";
    }
};

static std::map<std::type_index, std::string> names_map;

#define UniqueID(G)  std::type_index(typeid(G))

#define BIND(fp, cxx_target) \
    { \
        using G = FunctionId< decltype(&cxx_target), &cxx_target >; \
        fp = &G::call; \
        names_map[ UniqueID(G) ] = std::string(#cxx_target); \
    }

/**
 * @brief The Manager class
 *
 * The Manager class provides remote procedure call functionality over MPI. It makes heavy usage of C++11 varidic
 * templates to automatically generate classes to serialize and unserialize function parameters and return values.
 *
 * To add support for serializing and unserializing a custom type, provide the following stream operators:
 * ParameterStream &operator<<(ParameterStream& stream, const Type &t);
 * ParameterStream &operator>>(ParameterStream& stream, Type &t);
 *
 * Currently, only one Manager per rank is permitted. However, a tag prefix could be implemented to allow for
 * multiple Managers, thus allowing multiple communicators to be used.
 *
 * @todo Allow function and type IDs to be specified by the user.
 */
class Manager
{
    struct ObjectInfo {
        ObjectInfo() {}
        ObjectInfo(TypeId t, ObjectId i) : type(t), id(i) {}
        TypeId type;
        TypeId id;
    };

    /**
     * @brief The FunctionBase class
     *
     * Using template metaprogramming, Function<F> classes are created
     * for each function signature and std::function objects. Six specializations
     * are required, as each type must handle the special case of void return types.
     * This is because functions cannot be called passing a void argument.
     *
     * The function pointers are bound to instances of the subclasses.
     *
     * Static functions in a class behave as normal function pointers, not member
     * function pointers.
     */
    class FunctionBase
    {
    public:
        /*using GenericFunctionPointer = void(*)();*/
        typedef void(*GenericFunctionPointer)();

        FunctionBase() : m_id(makeId()), m_pointer(0), m_type_index(std::type_index(typeid(int))) {}

        /**
         * @brief execute Execute the function
         * @param params The serialized function parameters
         * @param senderRank The rank requesting this function be invoked
         * @param manager The Manager, when sending back the function return value
         * @param getReturn Only send the function return value back to the sending rank when requested, as the return value may not be needed.
         * @param object When the function is a member function, use object as the <i>this</i> pointer.
         */
        virtual void execute(ParameterStream& params, int senderRank, Manager *manager, bool getReturn = false, void* object = 0) = 0;

        FunctionHandle id() const { return m_id; }
        GenericFunctionPointer pointer() const { return m_pointer; }

        virtual ~FunctionBase() {};

    private:
        static FunctionHandle makeId() {
            return ++_idCounter;
        }

        FunctionHandle m_id;
        static FunctionHandle _idCounter;
    protected:
        GenericFunctionPointer m_pointer;
        std::function<void()> m_function;
        std::type_index m_type_index;
    };

    template<typename Functor, Functor f> struct FunctionId;

    template <typename R, class Class, typename... Args, R(Class::*target)(Args...)>
    struct FunctionId<R(Class::*)(Args...), target> {};

    template <typename R, typename... Args, R(*target)(Args...)>
    struct FunctionId<R(*)(Args...), target> {};

    /**
     * The general Function<F> class, which is specialized to deduce additional typenames where required while only
     * requiring a single typename be passed when constructing a Function.
     */
    template<typename F>
    class Function;

    /**
     * Specialization of Function<F> for functions with non-void return types.
     */
    template<typename R, typename... Args>
    class Function<R(*)(Args...)> : public FunctionBase
    {
    public:
        using FunctionType = R(*)(Args...);

        Function(R(*f)(Args...)) : FunctionBase(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

        virtual void execute(ParameterStream& params, int senderRank, Manager *manager, bool getReturn = false, void* object = 0) override
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
            OrderedCall<FunctionType> call{func, unmarshal<Args>(params)...};
            if (getReturn)
                manager->functionReturn(senderRank, call());
            else
                call();
        }

    protected:
        FunctionType func;
    };

    /**
     * Specialization of Function<F> for functions with void return types.
     */
    template<typename... Args>
    class Function<void(*)(Args...)> : public FunctionBase
    {
    public:
        using FunctionType = void(*)(Args...);

        Function(FunctionType f) : FunctionBase(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

        virtual void execute(ParameterStream& params, int senderRank, Manager *manager, bool getReturn = false, void* object = 0) override
        {
            OrderedCall<FunctionType> call{func, unmarshal<Args>(params)...};
            call();
        }

    protected:
        FunctionType func;
    };

    /**
     * Specialization of Function<F> for member functions witn non-void return types.
     */
    template<typename Class, typename R, typename... Args>
    class Function<R(Class::*)(Args...)> : public FunctionBase
    {
    public:
        using FunctionType = R(Class::*)(Args...);

        Function(FunctionType f) : FunctionBase(), func(f) {}

        virtual void execute(ParameterStream& params, int senderRank, Manager *manager, bool getReturn = false, void* object = 0) override
        {
            assert(object);
            assert(manager);
            OrderedCall<FunctionType> call{func, static_cast<Class*>(object), unmarshal<Args>(params)...};
            if (getReturn)
                manager->functionReturn(senderRank, call());
            else
                call();
        }

        FunctionType func;
    };

    /**
     * Specialization of Function<F> for member functions with void return types.
     */
    template<typename Class, typename... Args>
    class Function<void(Class::*)(Args...)> : public FunctionBase
    {
    public:
        using FunctionType = void(Class::*)(Args...);

        Function(void(Class::*f)(Args...)) : FunctionBase(), func(f) { }

        virtual void execute(ParameterStream& params, int senderRank, Manager *manager, bool getReturn = false, void* object = 0) override
        {
            assert(object);
            OrderedCall<FunctionType> call{func, static_cast<Class*>(object), unmarshal<Args>(params)...};
            call();
        }

        FunctionType func;
    };

    /**
     * Specialization of Function<F> for std::function objects with non-void return types.
     */
    template<typename R, typename... Args>
    class Function<std::function<R(Args...)>>
        : public FunctionBase
    {
    public:
        using FunctionType = std::function<R(Args...)>;

        Function(FunctionType& f) : FunctionBase(), func(f) {}

        virtual void execute(ParameterStream &params, int senderRank, Manager *manager, bool getReturn, void *object = 0) override
        {
            assert(manager);
            OrderedCall<FunctionType> call{func, unmarshal<Args>(params)...};

            if (getReturn)
                manager->functionReturn(senderRank, call());
            else
                call();
        }

        FunctionType func;
    };

    /**
     * Specialization of Function<F> for std::function objects with void return types.
     */
    template<typename... Args>
    class Function<std::function<void(Args...)>>
        : public FunctionBase
    {
    public:
        using FunctionType = std::function<void(Args...)>;

        Function(FunctionType& f) : FunctionBase(), func(f) {}

        virtual void execute(ParameterStream &params, int senderRank, Manager *manager, bool getReturn, void *object = 0) override
        {
            OrderedCall<FunctionType> call{func, unmarshal<Args>(params)...};
            call();
        }

        FunctionType func;
    };

public:
    //using UserMessageHandler = void(*)(MPI_Status&&);
    typedef void(*UserMessageHandler)(MPI_Status&&);

    Manager(MPI_Comm comm = MPI_COMM_WORLD);

    /**
     * Register a type with the Manager. This assigns a unique ID to the type.
     *
     * registerType<T>() must be called in the same order on all processes so that
     * the assigned IDs are consistent.
     *
     * @return The type ID
     */
    template<typename T>
    TypeId registerType()
    {
        TypeId id = ++m_nextTypeId;
        m_registeredTypeIds[std::type_index(typeid(typename std::decay<T>::type))] = id;
        return id;
    }

    /**
     * Get the type of a previously registered type.
     */
    template<typename T>
    TypeId getTypeId() const
    {
        TypeId id = 0;
        id = m_registeredTypeIds.at(std::type_index(typeid(typename std::decay<T>::type)));
        return id;
    }

    /**
     * @brief Register a lambda with the Manager
     * @return The handle for the lambda
     */
    template<typename Lambda>
    FunctionHandle registerLambda(Lambda&& l)
    {
        return registerFunction(static_cast<typename LambdaTraits<Lambda>::lambda_stdfunction>(l));
    }

    /**
     * This compile-time version allows for fast (hash table) function ID lookups
     *
     * @brief Register a function or member function with the Manager
     * @param f A function pointer to the function to register
     * @return The handle associated with function #f
     */
    template<typename F, F f>
    FunctionHandle registerFunction()
    {
        FunctionBase *b = new Function<F>(f);
        m_registeredFunctions[b->id()] = b;
        m_registeredFunctionTIs[std::type_index(typeid(FunctionId<F,f>))] = b->id();
        return b->id();
    }

    /**
     * This run-time version is incompatible with fast (hash table) function ID lookups
     */
    template<typename F>
    FunctionHandle registerFunction(F f)
    {
        FunctionBase *b = new Function<F>(f);
        m_registeredFunctions[b->id()] = b;
        return b->id();
    }

    /**
     * @brief Query the type identifier for the class Class
     * @return The identifier associated with class Class
     */
    template<class Class>
    TypeId getTypeId() {
        return m_registeredTypeIds.at(std::type_index(typeid(typename std::decay<Class>::type)));
    }

    /**
     * @brief Query the function handle for the member function pointer #f
     * @return The handle associated with the member function pointer #f
     */
    template<typename R, class Class, typename... Args>
    FunctionHandle getFunctionHandle(R(Class::*f)(Args...))
    {
        for (const auto &i : m_registeredFunctions)
        {
            Function<R(Class::*)(Args...)>* func = dynamic_cast<Function<R(Class::*)(Args...)>*>(i.second);
            if (func) {
                return func->id();
            }
        }
    }

    template<typename T, T t>
    FunctionHandle getFunctionHandle()
    {
        return m_registeredFunctionTIs[std::type_index(typeid(FunctionId<T,t>))];
    }

    /**
     * @brief Query the function handle for the function pointer #f
     * @return The identifier handle with the function pointer #f
     */
    template<typename R, typename... Args>
    FunctionHandle getfunctionHandle(R(*f)(Args...))
    {
        for (const auto &i : m_registeredFunctions)
        {
            if (i.second->pointer() == reinterpret_cast<void(*)()>(f))
                return i.first;
        }
        throw UnregisteredFunctionException();
    }

    /**
     * @brief Register an object with the Manager. Other ranks are informed of the existance of this object
     * @return A wrapper for the object, containing a pointer to the object and the ids used to call its member functions
     */
    template<class Class>
    ObjectWrapper<Class>* registerObject(Class *object) {
        ObjectWrapper<Class> *wrapper = new ObjectWrapper<Class>(object);
        wrapper->m_rank = m_rank;
        wrapper->m_type = getTypeId<Class>();
        m_registeredObjects.push_back(wrapper);
        notifyNewObject(wrapper->type(), wrapper->id());
        return wrapper;
    }


    /**
     * @brief invoke a function on rank #rank
     *
     * Note: An object's static functions behave as normal function pointers.
     *
     * Note: function pointer versions of Manager::invokeFunction are slightly
     * slower than FunctionHandle versions of Manager::invokeFunction when many functions
     * are registered due to the need to search for the function id associated
     * with the function pointer.
     *
     * At least when a low number of function pointers are registered, the dynaimc_cast does not have
     * a noticable performance penalty:
     * Using handle: 132,347 per second
     * Using function pointer with reinterpret_cast before dynamic_cast: 126,155 per second
     * Using virtual function pointer: 133,369 per second
     * Using function pointer: 136,878 per second
     *
     * @param rank The MPI rank to invoke the function on
     * @param f The function pointer to invoke. This function pointer must be registered with the Manager. See: Manager::registerFunction.
     * @param functionHandle The function handle for the registered function, f. If this value is 0, the function handle is searched for in the function map.
     * @param args... The function's parameters. These parameters should be treated as being passed by value. However, it would be possible to
     * use std::is_lvalue_reference to serialize the lvalue parameters and send back these potentially modified values. std::is_pointer could
     * similarly be used for pointers with serializable types.
     * @return The return value of the function if getReturn is true. Otherwise returns a default constructed R.
     */
    template<typename R, typename... FArgs, typename... Args>
    auto invokeFunctionR(int rank, R(*f)(FArgs...), FunctionHandle functionHandle, Args&&... args)
        -> typename std::enable_if<!std::is_same<R, void>::value, R>::type
    {
        if (rank == m_rank) {
            return f(std::forward<FArgs>(args)...);
        } else {
            if (functionHandle == 0)
            {
                for (const auto &i : m_registeredFunctions) {
                    if (i.second->pointer() == reinterpret_cast<void(*)()>(f)) {
                        sendFunctionInvocation<FArgs...>(rank, i.first, true, std::forward<FArgs>(args)...);
                        return processReturn<R>(rank);
                    }
                }
            }
            else
            {
                sendFunctionInvocation<FArgs...>(rank, functionHandle, true, std::forward<FArgs>(args)...);
                return processReturn<R>(rank);
            }
            throw UnregisteredFunctionException();
        }
    }

    /**
     * Specialized for void return type
     *
     * @see Manager::invokeFunction()
     */
    template<typename R, typename... FArgs, typename...Args>
    void invokeFunction(int rank, R(*f)(FArgs...), FunctionHandle functionHandle, Args&&... args)
    {
        if (rank == m_rank) {
            f(std::forward<FArgs>(args)...);
        } else {
            if (functionHandle == 0) {
                for (const auto &i : m_registeredFunctions) {
                    if (i.second->pointer() == reinterpret_cast<void(*)()> (f)) {
                        sendFunctionInvocation<FArgs...>(rank, i.first, false, std::forward<FArgs>(args)...);
                        return;
                    }
                }
                throw UnregisteredFunctionException();
            }
            else
            {
                sendFunctionInvocation<FArgs...>(rank, functionHandle, false, std::forward<FArgs>(args)...);
            }
        }
    }

    /**
     * @see Manager::invokeFunction()
     *
     * Note: this function assumes that the arguments passed are what the function uses.
     *
     * Local calls cannot be optimized to a simple function call because we don't know how to cast the Function object here
     */
    template<typename R, typename... Args>
    R invokeFunctionR(int rank, FunctionHandle functionHandle, Args&&... args)
    {
        sendFunctionInvocation(rank, functionHandle, true, std::forward<Args>(args)...);
        return processReturn<R>(rank);
    }

    /**
     * Specialized for void return type
     *
     * @see Manager::invokeFunction()
     *
     * @todo Optimize local calls
     */
    template<typename... Args>
    void invokeFunction(int rank, FunctionHandle functionHandle, Args&&... args)
    {
        sendFunctionInvocation(rank, functionHandle, false, std::forward<Args>(args)...);
    }

    /**
     * @brief Call the member function of an object remotely
     *
     * @param a The object wrapper identifying the object and its location.
     * @param f The member function pointer to invoke. This member function pointer must be registered with the Manager. See: Manager::registerFunction.
     * @param getReturn If true, the remote process will send back the function's return value. Otherwise a default constructed R is returned.
     * @param args... The function's parameters. These parameters should be treated as being passed by value. However, it would be possible to
     * use std::is_lvalue_reference to serialize the lvalue parameters and send back these potentially modified values. std::is_pointer could
     * similarly be used for pointers with serializable types.
     * @return The return value of the function if getReturn is true. Otherwise returns a default constructed R.
     */
    template<typename R, class Class, typename... FArgs, typename... Args>
    auto invokeFunctionR(ObjectWrapperBase *a, R(Class::*f)(FArgs...), FunctionHandle functionHandle, Args&&... args)
        -> typename std::enable_if<!std::is_same<R, void>::value, R>::type
    {
        if (a->rank() == m_rank)
        {
            ObjectWrapper<Class> *o = static_cast<ObjectWrapper<Class>*>(a);
            return CALL_MEMBER_FN(*o->object(),f)(std::forward<FArgs>(args)...);
        } else {
            if (functionHandle == 0)
            {
                for (const auto &i : m_registeredFunctions)
                {
                    Function<R(Class::*)(FArgs...)>* func = dynamic_cast<Function<R(Class::*)(FArgs...)>*>(i.second);
                    if (func) {
                        if (func->func == f)
                        {
                            sendMemberFunctionInvocation<FArgs...>(a, func->id(), true, std::forward<FArgs>(args)...);
                            return processReturn<R>(a->rank());
                        }
                    }
                }
                throw UnregisteredFunctionException();
            }
            else
            {
                sendMemberFunctionInvocation<FArgs...>(a, functionHandle, true, std::forward<FArgs>(args)...);
                return processReturn<R>(a->rank());
            }
        }
    }

    /**
     * Version for void return type
     *
     * @see Manager::invokeMemberFunction()
     */
    template<typename R, class Class, typename... FArgs, typename... Args>
    void invokeFunction(ObjectWrapperBase *a, R(Class::*f)(FArgs...), FunctionHandle functionHandle, Args&&... args)
    {
        if (a->rank() == m_rank)
        {
            ObjectWrapper<Class> *o = static_cast<ObjectWrapper<Class>*>(a);
            CALL_MEMBER_FN(*o->object(),f)(std::forward<FArgs>(args)...);
        } else {
            if (functionHandle == 0)
            {
                for (const auto &i : m_registeredFunctions)
                {
                    Function<R(Class::*)(FArgs...)>* func = dynamic_cast<Function<R(Class::*)(FArgs...)>*>(i.second);
                    if (func) {
                        if (func->func == f)
                        {
                            sendMemberFunctionInvocation<FArgs...>(a, func->id(), false, std::forward<FArgs>(args)...);
                            return;
                        }
                    }
                }
                throw UnregisteredFunctionException();
            }
            else
            {
                sendMemberFunctionInvocation<FArgs...>(a, functionHandle, false, std::forward<FArgs>(args)...);
            }
        }
    }

    /**
     * @see Manager::invokeFunction()
     */
    template<typename R, typename... Args>
    R invokeFunctionR(ObjectWrapperBase *a, FunctionHandle functionHandle, Args&&... args)
    {
        sendMemberFunctionInvocation(a, functionHandle, true, std::forward<Args>(args)...);
        return processReturn<R>(a->rank());
    }

    /**
     * Specialized for void return type
     *
     * @see Manager::invokeFunction()
     */
    template<typename... Args>
    void invokeFunction(ObjectWrapperBase *a, FunctionHandle functionHandle, Args&&... args)
    {
        sendMemberFunctionInvocation(a, functionHandle, false, std::forward<Args>(args)...);
    }

    /**
     * @brief Get the MPI rank of this process
     * @return The MPI rank of this process
     */
    int rank() const;

    /**
     * @brief Get the total number of MPI processes in the communicator
     * @return The total number of MPI processes in the communicator
     */
    int numProcs() const;

    /**
     * @brief Check for incoming commands. Also runs Manager::checkSends()
     * @return True to continue running. False indicates this process should shut down.
     */
    bool checkMessages();

    /**
     * @brief Checks on the status of the non-blocking sends and frees resources of completed sends.
     * @return True to continue running. False indicates this process should shut down.
     */
    bool checkSends();

    /**
     * @brief Get the first wrapper to the object of type #typeId
     * @param typeId The type identifier
     * @return A pointer to the object's wrapper
     */
    ObjectWrapperBase* getObjectOfType(TypeId typeId) const;

    /**
     * @brief Get the first wrapper to the object of type Class
     * @return A pointer to the object's wrapper
     */
    template<class Class>
    ObjectWrapperBase* getObjectOfType() const
    {
        return getObjectOfType(getTypeId<Class>());
    }

    /**
     * @brief Get the first wrapper to the object of type #typeId which exists on rank #rank
     * @param typeId The object's typeId
     * @param rank The rank on which the object exists
     * @return A pointer to the object's wrapper
     */
    ObjectWrapperBase* getObjectOfType(TypeId typeId, int rank) const;

    /**
     * @brief Get the first wrapper to the object of type Class which exists on rank #rank
     * @param rank The rank on which the object exists
     * @return A pointer to the object's wrapper
     */
    template<class Class>
    ObjectWrapperBase* getObjectOfType(int rank) const
    {
        return getObjectOfType(getTypeId<Class>(), rank);
    }

    /**
     * @brief Get the set of all objects of type #typeId for rank #rank
     * @param typeId The type identifier
     * @param rank The rank the objects exist on
     * @return A std::unordered_set of object wrapers for the type and rank
     */
    std::unordered_set<ObjectWrapperBase*> getObjectsOfType(TypeId typeId, int rank) const;

    /**
     * @brief Get the set of all objects of type Class for rank #rank
     * @param rank The rank the objects exist on
     * @return A std::unordered_set of object wrapers for the type and rank
     */
    template<class Class>
    std::unordered_set<ObjectWrapperBase*> getObjectsOfType(int rank) const
    {
        return getObjectsOfType(getTypeId<Class>(), rank);
    }

    /**
     * @brief Get the set of all objects of type #typeId
     * @param typeId The type identifier
     * @return A std::unordered_set of object wrappers for the type
     */
    std::unordered_set<ObjectWrapperBase*> getObjectsOfType(mpirpc::TypeId typeId) const;

    /**
     * @brief Get the set of all objects of type Class
     * @return A std::unordered_set of all object wrappers for the type
     */
    template<class Class>
    std::unordered_set<ObjectWrapperBase*> getObjectsOfType()
    {
        return getObjectsOfType(getTypeId<Class>());
    }

    template<typename T>
    std::vector<T> reduce(std::vector<T>& vec, MPI_Op op, int root)
    {
        int vecsize = vec.size();
        std::vector<T> res(vecsize);
        MPI_Reduce(vec.data(), res.data(), vecsize, mpiType<T>(), op, root, m_comm);
        return res;
    }

    template<typename T>
    std::vector<T> allreduce(std::vector<T>& vec,  MPI_Op op)
    {
        int vecsize = vec.size();
        std::vector<T> res(vecsize);
        MPI_Allreduce(vec.data(), res.data(), vecsize, mpiType<T>(), op, m_comm);
        return res;
    }

    template<typename T>
    T* reduce(T* first, T* last, MPI_Op mpiOp, int root)
    {
        std::size_t size = last-first;
        T* res = new T[size];
        MPI_Reduce(first, res, size, mpiType<T>(), mpiOp, root, m_comm);
        return res;
    }

    template<typename T>
    T* allreduce(T* first, T* last, MPI_Op mpiOp)
    {
        std::size_t size = last-first;
        T* res = new T[size];
        MPI_Allreduce(first, res, size, mpiType<T>(), mpiOp, m_comm);
        return res;
    }

    template<class InputIt, class T, class BinaryOperation>
    T accumulate( InputIt first, InputIt last, T init, BinaryOperation op, MPI_Op mpiOp)
    {
        T intermediate = std::accumulate(first, last, init, op);
        T res;
        MPI_Allreduce(&intermediate, &res, 1, mpiType<T>(), mpiOp, m_comm);
        return res;
    }

    template<class InputIt>
    typename std::iterator_traits<InputIt>::value_type
    accumulate( InputIt first, InputIt last)
    {
        typename std::iterator_traits<InputIt>::value_type intermediate = std::accumulate(first, last, static_cast<typename std::iterator_traits<InputIt>::value_type>(0));
        typename std::iterator_traits<InputIt>::value_type res;
        MPI_Allreduce(&intermediate, &res, 1, mpiType<typename std::iterator_traits<InputIt>::value_type>(), MPI_SUM, m_comm);
        return res;
    }

    /**
     * @brief Get an object's wrapper, given it's id.
     * @param id The id of the object
     * @return The object wrapper
     */
    ObjectWrapperBase* getObjectWrapper(int rank, TypeId tid, ObjectId oid) const;

    /**
     * @brief Register a custom message handler to be invoked when an MPI message has been probed with tag #tag.
     * @param tag The tag identifying this type of message.
     * @param callback A void(*)(MPI_Status&&) function pointer. This function should handle the MPI_Recv.
     */
    void registerUserMessageHandler(int tag, UserMessageHandler callback);

    /**
     * @brief Send a buffer to rank #rank with tag #tag
     */
    void sendRawMessage(int rank, const std::vector<char> *data, int tag = 0);

    /**
     * @brief Send a buffer to every other rank with tag #tag
     *
     * Does not send to self.
     */
    void sendRawMessageToAll(const std::vector<char> *data, int tag = 0);

    /**
     * @brief Executes an MPI_Barrier, then checks messages to ensure the state of all Managers are in a valid state.
     *
     * When registering objects that depend on remote objects, they must be initialized in order (so that their ids are propagated).
     */
    void sync();

    /**
     * @brief Shut down all Managers on all processes.
     */
    void shutdown();

    /**
     * @brief The number of function invocations this Manager has handled.
     */
    unsigned long long stats() const;

    /**
     * @brief Get the MPI communicator
     */
    MPI_Comm comm() const;

    /**
     * @brief The size of the message send queue. Can check mesages continuously while queue size >0 to ensure all messages have been sent.
     */
    size_t queueSize() const;

    /**
     * Destroy this Manager
     */
    ~Manager();

protected:

    /**
     * @brief Send the result of executing a function back to the sending rank.
     * @param rank The rank which invoked the function
     * @param r The invoked functions return value
     */
    template<typename R>
    void functionReturn(int rank, R r)
    {
        MPI_Status status;
        std::vector<char>* buffer = new std::vector<char>();
        ParameterStream stream(buffer);
        stream << r;
        MPI_Send((void*) stream.dataVector()->data(), stream.size(), MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm);
        delete buffer;
    }

    /**
     * @brief Invoke a function on a remote process
     * @param rank The remote rank
     * @param functionHandle The function's unique identifier
     * @param getReturn Indicate to the remote process if this process will be expecting the function's return value
     * @param args The parameter pack of the function's arguments
     *
     * Internally, a dummy wrapper, Passer, uses uniform initilization to ensure the side efects of the stream operator
     * occurr in the order in which they appear when the parameter pack is unpacked. GCC currently does this in reverse
     * order due Bug #51253 (http://gcc.gnu.org/bugzilla/show_bug.cgi?id=51253). However, this is inconsequential since
     * unpacking will be done in the same order.
     */
    template<typename... Args>
    void sendFunctionInvocation(int rank, FunctionHandle functionHandle, bool getReturn, Args... args)
    {
        std::vector<char>* buffer = new std::vector<char>();
        ParameterStream stream(buffer);
        stream << functionHandle << getReturn;
        Passer p{(stream << args, 0)...};
        sendRawMessage(rank, stream.dataVector(), MPIRPC_TAG_INVOKE);
    }

    /**
     * Invoke a member function on a remote process
     *
     * @see Manager::sendFunctionInvocation(int,FunctionHandle,bool,Args...)
     */
    template<typename... Args>
    void sendMemberFunctionInvocation(ObjectWrapperBase *a, FunctionHandle functionHandle, bool getReturn, Args... args)
    {
        std::vector<char>* buffer = new std::vector<char>();
        ParameterStream stream(buffer);
        stream << a->type() << a->id();
        stream << functionHandle << getReturn;
        Passer p{(stream << args, 0)...};
        sendRawMessage(a->rank(), stream.dataVector(), MPIRPC_TAG_INVOKE_MEMBER);
    }

    /**
     * Wait for the remote process to run an invocation and send that function's return value back to this process.
     * Unserialize the result and return it.
     */
    template<typename R>
    R processReturn(int rank) {
        MPI_Status status;
        int len;
        int flag;
        bool shutdown;
        do {
            shutdown = !checkMessages();
            MPI_Iprobe(rank, MPIRPC_TAG_RETURN, m_comm, &flag, &status);
        } while (!flag && !shutdown);
        if (!shutdown)
            MPI_Get_count(&status, MPI_CHAR, &len);
        R ret;
        if (!shutdown && len != MPI_UNDEFINED) {
            std::vector<char>* buffer = new std::vector<char>(len);
            ParameterStream stream(buffer);
            MPI_Recv((void*) buffer->data(), len, MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm, &status);
            ret = unmarshal<R>(stream);
            delete buffer;
        }
        return ret;
    }

    /**
     * @brief Handle a message indicating a remote process is registering a new object.
     */
    void registerRemoteObject();

    /**
     * @brief Notify other processes of an object registered on this Manager.
     */
    void notifyNewObject(mpirpc::TypeId type, mpirpc::ObjectId id);

    /**
     * @brief Handle a message to execute a function
     */
    void receivedInvocationCommand(MPI_Status &&);

    /**
     * @brief Handle a message to execute a member function
     */
    void receivedMemberInvocationCommand(MPI_Status &&);

    /**
     * @brief Handle a message indicating this Manager should shut down.
     */
    void handleShutdown();

    /**
     * @brief Record a remote object with this Manager
     */
    void registerRemoteObject(int rank, mpirpc::TypeId type, mpirpc::ObjectId id);

    std::unordered_map<std::type_index, TypeId> m_registeredTypeIds;

    std::map<FunctionHandle, FunctionBase*> m_registeredFunctions;
    std::unordered_map<std::type_index, FunctionHandle> m_registeredFunctionTIs;
    std::vector<ObjectWrapperBase*> m_registeredObjects;

    std::unordered_map<MPI_Request, const std::vector<char>*> m_mpiMessages;
    std::unordered_map<MPI_Request, std::shared_ptr<ObjectInfo>> m_mpiObjectMessages;

    std::unordered_map<int, UserMessageHandler> m_userMessageHandlers;

    MPI_Comm m_comm;
    TypeId m_nextTypeId;
    int m_rank;
    int m_numProcs;
    unsigned long long m_count;
    bool m_shutdown;
    MPI_Datatype MpiObjectInfo;
};

}

#endif // MPIRPCMANAGER_H
