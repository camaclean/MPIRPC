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
#include <numeric>
#include <thread>
#include <numeric>

#include <mpi.h>

#include "objectwrapper.hpp"
#include "lambda.hpp"
#include "orderedcall.hpp"
#include "common.hpp"
#include "parameterstream.hpp"
#include "forwarders.hpp"
#include "mpitype.hpp"
#include "exceptions.hpp"

#define ERR_ASSERT     1
#define ERR_MAX_ACTORS 2
#define MAX_GENERIC_ARGUMENTS 20
#define MAX_MPI_QUEUE 10

#define MPIRPC_TAG_NEW 1
#define MPIRPC_TAG_SHUTDOWN 2
#define MPIRPC_TAG_INVOKE 3
#define MPIRPC_TAG_INVOKE_MEMBER 4
#define MPIRPC_TAG_RETURN 5

#define BUFFER_SIZE 10*1024*1024

#define CALL_MEMBER_FN(object,ptr) ((object).*(ptr))

namespace mpirpc {

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

/*template<typename MessageInterface>
class Manager;*/

template<typename MessageInterface>
class Manager
{
    struct ObjectInfo;

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
    class FunctionBase;

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
    class Function<R(*)(Args...)>;

    /**
     * Specialization of Function<F> for functions with void return types.
     */
    template<typename... Args>
    class Function<void(*)(Args...)>;

    /**
     * Specialization of Function<F> for member functions witn non-void return types.
     */
    template<typename Class, typename R, typename... Args>
    class Function<R(Class::*)(Args...)>;

    /**
     * Specialization of Function<F> for member functions with void return types.
     */
    template<typename Class, typename... Args>
    class Function<void(Class::*)(Args...)>;

    /**
     * Specialization of Function<F> for std::function objects with non-void return types.
     */
    template<typename R, typename... Args>
    class Function<std::function<R(Args...)>>;

    /**
     * Specialization of Function<F> for std::function objects with void return types.
     */
    template<typename... Args>
    class Function<std::function<void(Args...)>>;

public:
    using UserMessageHandler = void(*)(MPI_Status&&);

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
    TypeId registerType();

    /**
     * @brief Query the type identifier for the class Class
     * @return The identifier associated with class Class
     */
    template<typename T>
    TypeId getTypeId() const;

    /**
     * @brief Register a lambda with the Manager
     * @return The handle for the lambda
     */
    template<typename Lambda>
    FnHandle registerLambda(Lambda&& l);

    /*template<typename F, typename StorageFunctionParts<F>::storage_function_type f>
    fnhandle_t registerFunction()
    {
        return registerFunction<typename StorageFunctionParts<F>::storage_function_type, f>();
    }*/

    /**
     * This compile-time version allows for fast (hash table) function ID lookups
     *
     * @brief Register a function or member function with the Manager
     * @param f A function pointer to the function to register
     * @return The handle associated with function #f
     */
    template<typename F, typename storage_function_parts<F>::function_type f>
    FnHandle registerFunction();

    /**
     * This run-time version is incompatible with fast (hash table) function ID lookups
     */
    template<typename F>
    FnHandle registerFunction(F f);

    /**
     * @brief Query the function handle for the member function pointer #f
     * @return The handle associated with the member function pointer #f
     */
    /*template<typename F>
    fnhandle_t get_fn_handle(F&& f)
    {
        return m_registered_function_typeids[std::type_index(typeid(function_identifier<typename storage_function_parts<F>::function_type,f>))];
    }*/

    template<typename F, typename storage_function_parts<F>::function_type f>
    FnHandle get_fn_handle()
    {
        return m_registered_function_typeids[std::type_index(typeid(function_identifier<F,f>))];
    }

    /**
     * @brief Query the function handle for the function pointer #f
     * @return The identifier handle with the function pointer #f
     */
    /*template<typename F>
    fnhandle_t get_fn_handle(F&& f)
    {
        static_assert(std::is_function<F>::value || std::is_member_function<F>::value, "mpirpc::Manager::get_fn_handle expects a function pointer as the first argument."
        for (const auto &i : m_registered_functions)
        {
            if (i.second->pointer() == reinterpret_cast<void(*)()>(f))
                return i.first;
        }
        throw UnregisteredFunctionException();
    }*/

    /**
     * @brief Register an object with the Manager. Other ranks are informed of the existance of this object
     * @return A wrapper for the object, containing a pointer to the object and the ids used to call its member functions
     */
    template<class Class>
    ObjectWrapper<Class>* registerObject(Class *object);

    template<class Class, typename... Args>
    ObjectWrapperBase* constructGlobalObject(int rank, Args&&... args);

    /**
     * @brief invoke a function on rank #rank
     *
     * Note: An object's static functions behave as normal function pointers.
     *
     * Note: function pointer versions of Manager::invokeFunction are slightly
     * slower than fnhandle_t versions of Manager::invokeFunction when many functions
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
    auto invokeFunctionR(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args)
        -> typename std::enable_if<!std::is_same<R, void>::value, R>::type;

    template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
    auto invokeFunctionR(int rank, Args&&... args)
        -> typename detail::marshaller_function_signature<F,Args...>::void_return_type;

    template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
    auto invokeFunctionR(int rank, Args&&... args)
        -> typename detail::marshaller_function_signature<F,Args...>::non_void_return_type;

    /**
     * Specialized for void return type
     *
     * @see Manager::invokeFunction()
     */
    template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
    void invokeFunction(int rank, Args&&... args);

    template<typename R, typename... FArgs, typename...Args>
    void invokeFunction(int rank, R(*f)(FArgs...), FnHandle functionHandle, Args&&... args);

    /**
     * @see Manager::invokeFunction()
     *
     * Note: this function assumes that the arguments passed are what the function uses.
     *
     * Local calls cannot be optimized to a simple function call because we don't know how to cast the Function object here
     */
    template<typename R, typename... Args>
    R invokeFunctionR(int rank, FnHandle functionHandle, Args&&... args);

    /**
     * Specialized for void return type
     *
     * @see Manager::invokeFunction()
     *
     * @todo Optimize local calls
     */
    template<typename... Args>
    void invokeFunction(int rank, FnHandle functionHandle, Args&&... args);

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
    [[deprecated]] auto invokeFunctionR(ObjectWrapperBase *a, R(Class::*f)(FArgs...), FnHandle functionHandle, Args&&... args)
        -> typename std::enable_if<!std::is_same<R, void>::value, R>::type;

    template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
    auto invokeFunctionR(ObjectWrapperBase *a, Args&&... args)
        -> typename detail::marshaller_function_signature<F,Args...>::return_type;

    /**
     * Version for void return type
     *
     * @see Manager::invokeMemberFunction()
     */
    template<typename R, class Class, typename... FArgs, typename... Args>
    [[deprecated]] void invokeFunction(ObjectWrapperBase *a, R(Class::*f)(FArgs...), FnHandle functionHandle, Args&&... args);

    template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
    void invokeFunction(ObjectWrapperBase *a, Args&&... args);

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
    ObjectWrapperBase* getObjectOfType() const;

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
    ObjectWrapperBase* getObjectOfType(int rank) const;

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
    std::unordered_set<ObjectWrapperBase*> getObjectsOfType(int rank) const;

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
    std::unordered_set<ObjectWrapperBase*> getObjectsOfType() const;

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
     * @brief Shut down all Managers on all processes. This must be called on only one rank.
     */
    void shutdownAll();

    /**
     * @brief Shut down this Manager. This must be called on all ranks.
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
    template<typename R, typename... Args,bool...PBs,std::size_t... Is>
    void functionReturn(int rank, R&& r, std::tuple<Args...> args, bool_tuple<PBs...>, std::index_sequence<Is...>)
    {
        std::vector<char>* buffer = new std::vector<char>();
        ParameterStream stream(buffer);
        stream << std::forward<R>(r);
        using swallow = int[];
        swallow s{((PBs) ? (marshal(stream,std::get<Is>(args)), 1) : 0)...};
        std::cout << "swallow: " << sizeof...(Is) << ": ";
        for(size_t i = 0; i < sizeof...(Is); i++)
            std::cout << s[i];
        std::cout << std::endl;
        MPI_Send((void*) stream.dataVector()->data(), stream.size(), MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm);
        delete buffer;
    }

    template<typename... Args,bool...PBs,std::size_t... Is>
    void functionReturn(int rank, std::tuple<Args...> args, bool_tuple<PBs...>, std::index_sequence<Is...>)
    {
        std::vector<char>* buffer = new std::vector<char>();
        ParameterStream stream(buffer);
        using swallow = int[];
        swallow s{((PBs) ? (marshal(stream,std::get<Is>(args)), 1) : 0)...};
        std::cout << "swallow: " << sizeof...(Is) << ": ";
        for(size_t i = 0; i < sizeof...(Is); i++)
            std::cout << s[i];
        std::cout << std::endl;
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
    void sendFunctionInvocation(int rank, FnHandle functionHandle, bool getReturn, Args&&... args);

    template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
    void sendFunctionInvocation(int rank, bool getReturn, Args&&... args);

    /**
     * Invoke a member function on a remote process
     *
     * @see Manager::sendFunctionInvocation(int,fnhandle_t,bool,Args...)
     */
    template<typename... Args>
    [[deprecated]] void sendMemberFunctionInvocation(ObjectWrapperBase *a, FnHandle functionHandle, bool getReturn, Args&&... args);

    template<typename F, typename storage_function_parts<F>::function_type f, typename... Args>
    void sendMemberFunctionInvocation(ObjectWrapperBase* a, bool get_return, Args&&... args);

    /**
     * Wait for the remote process to run an invocation and send that function's return value back to this process.
     * Unserialize the result and return it.
     */
    template<typename R, typename... Args, bool...PBs, std::size_t... Is>
    processReturn(int rank, R& r, bool_tuple<PBs...>, std::index_sequence<Is...>, Args&&... args) {
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
        if (!shutdown && len != MPI_UNDEFINED) {
            std::vector<char>* buffer = new std::vector<char>(len);
            ParameterStream stream(buffer);
            MPI_Recv((void*) buffer->data(), len, MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm, &status);
            r = unmarshal<R>(stream);
            delete buffer;
        }
        return ret;
    }

    template<typename...Args>
    void processReturn(int rank, Args&&... args) {
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
        if (!shutdown && len != MPI_UNDEFINED) {
            std::vector<char>* buffer = new std::vector<char>(len);
            ParameterStream stream(buffer);
            MPI_Recv((void*) buffer->data(), len, MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm, &status);
            delete buffer;
        }
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
    std::unordered_map<std::type_index, TypeId> m_registeredMemoryManagers;

    std::map<FnHandle, FunctionBase*> m_registered_functions;
    std::unordered_map<std::type_index, FnHandle> m_registered_function_typeids;
    std::vector<ObjectWrapperBase*> m_registeredObjects;

    std::unordered_map<MPI_Request, const std::vector<char>*> m_mpiMessages;
    std::unordered_map<MPI_Request, std::shared_ptr<ObjectInfo>> m_mpiObjectMessages;

    std::unordered_map<int, UserMessageHandler> m_userMessageHandlers;

    MPI_Comm m_comm;
    TypeId m_nextTypeId;
    TypeId m_nextDeleterId;
    int m_rank;
    int m_numProcs;
    unsigned long long m_count;
    bool m_shutdown;
    MPI_Datatype MpiObjectInfo;
};

template<class MessageInterface>
struct Manager<MessageInterface>::ObjectInfo {
    ObjectInfo() {}
    ObjectInfo(TypeId t, ObjectId i) : type(t), id(i) {}
    TypeId type;
    TypeId id;
};

#include "detail/manager/manager.hpp"
#include "detail/manager/function.hpp"
#include "detail/manager/register.hpp"
#include "detail/manager/invoke.hpp"

template<typename MessageInterface>
int Manager<MessageInterface>::rank() const
{
    return m_rank;
}

template<typename MessageInterface>
void Manager<MessageInterface>::notifyNewObject(TypeId type, ObjectId id)
{
    if (m_shutdown)
        return;
    std::shared_ptr<ObjectInfo> info(new ObjectInfo(type, id));
    for(int i = 0; i < m_numProcs; ++i)
    {
        if (i != m_rank)
        {
            MPI_Request req;
            MPI_Issend(info.get(), 1, MpiObjectInfo, i, MPIRPC_TAG_NEW, m_comm, &req);
            m_mpiObjectMessages[req] = info;
        }
    }
}

template<typename MessageInterface>
void Manager<MessageInterface>::sendRawMessage(int rank, const std::vector<char> *data, int tag)
{
    if (checkSends() && !m_shutdown) {
        MPI_Request req;
        MPI_Issend((void*) data->data(), data->size(), MPI_CHAR, rank, tag, m_comm, &req);
        m_mpiMessages[req] = data;
    }
}

template<typename MessageInterface>
void Manager<MessageInterface>::sendRawMessageToAll(const std::vector<char>* data, int tag)
{
    for (int i = 0; i < m_numProcs; ++i) {
#ifndef USE_MPI_LOCALLY
        if (i != m_rank) {
#endif
            sendRawMessage(i, data, tag);
#ifndef USE_MPI_LOCALLY
        }
#endif
    }
}

template<typename MessageInterface>
void Manager<MessageInterface>::registerUserMessageHandler(int tag, UserMessageHandler callback)
{
    m_userMessageHandlers[tag] = callback;
}

template<typename MessageInterface>
void Manager<MessageInterface>::registerRemoteObject()
{
    ObjectInfo info;
    MPI_Status status;
    MPI_Recv(&info, 1, MpiObjectInfo, MPI_ANY_SOURCE, MPIRPC_TAG_NEW, m_comm, &status);
    registerRemoteObject(status.MPI_SOURCE, info.type, info.id);
}

template<typename MessageInterface>
bool Manager<MessageInterface>::checkSends() {
    for (auto i = m_mpiObjectMessages.begin(); i != m_mpiObjectMessages.end();) {
        MPI_Request req = i->first;
        int flag;
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            i->second.reset();
            m_mpiObjectMessages.erase(i++);
        } else {
            ++i;
        }
    }
    for (auto i = m_mpiMessages.begin(); i != m_mpiMessages.end();) {
        MPI_Request req = i->first;
        int flag;
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            delete i->second;
            m_mpiMessages.erase(i++);
        } else {
            ++i;
        }
    }
    if (m_shutdown) {
        return false;
    }
    return true;
}

template<typename MessageInterface>
bool Manager<MessageInterface>::checkMessages() {
    if (m_shutdown)
        return false;
    checkSends();
    int flag = 1;
    while (flag) {
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, m_comm, &flag, &status);
        if (flag) {
            switch (status.MPI_TAG) {
                case MPIRPC_TAG_SHUTDOWN:
                    m_shutdown = true;
                    handleShutdown();
                    checkSends();
                    return false;
                case MPIRPC_TAG_NEW:
                    registerRemoteObject();
                    break;
                case MPIRPC_TAG_INVOKE:
                    receivedInvocationCommand(std::move(status));
                    break;
                case MPIRPC_TAG_INVOKE_MEMBER:
                    receivedMemberInvocationCommand(std::move(status));
                    break;
                case MPIRPC_TAG_RETURN:
                    return true;
                default:
                    UserMessageHandler func = m_userMessageHandlers.at(status.MPI_TAG);
                    func(std::move(status));
            }
        }
    }
    return true;
}

template<typename MessageInterface>
void Manager<MessageInterface>::sync() {
    while (queueSize() > 0) { checkMessages(); } //block until this rank's queue is processed
    MPI_Request req;
    int flag;
    MPI_Ibarrier(m_comm, &req);
    do
    {
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        checkMessages();
    } while (!flag); //wait until all other ranks queues have been processed
}

template<typename MessageInterface>
void Manager<MessageInterface>::shutdownAll() {
    int buf = 0;
    for (int i = 0; i < m_numProcs; ++i)
    {
        if (i != m_rank) {
            MPI_Bsend((void*) &buf, 1, MPI_INT, i, MPIRPC_TAG_SHUTDOWN, m_comm);
        }
    }
    sync();
    m_shutdown = true;
}

template<typename MessageInterface>
void Manager<MessageInterface>::shutdown()
{
    sync();
    m_shutdown = true;
}

template<typename MessageInterface>
void Manager<MessageInterface>::handleShutdown()
{
    int buf;
    MPI_Status status;
    MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, MPIRPC_TAG_SHUTDOWN, m_comm, &status);
    sync();
    m_shutdown = true;
}

template<typename MessageInterface>
void Manager<MessageInterface>::receivedInvocationCommand(MPI_Status&& status)
{
    m_count++;
    int len;
    MPI_Get_count(&status, MPI_CHAR, &len);
    if (len != MPI_UNDEFINED) {
        std::vector<char>* buffer = new std::vector<char>(len);
        ParameterStream stream(buffer);
        MPI_Status recv_status;
        MPI_Recv(stream.data(), len, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, m_comm, &recv_status);
        FnHandle function_handle;
        bool get_return;
        stream >> function_handle >> get_return;
        FunctionBase *f = m_registered_functions[function_handle];
        f->execute(stream, recv_status.MPI_SOURCE, this, get_return);
        delete buffer;
    }
}

template<typename MessageInterface>
void Manager<MessageInterface>::receivedMemberInvocationCommand(MPI_Status&& status) {
    m_count++;
    int len;
    MPI_Get_count(&status, MPI_CHAR, &len);
    if (len != MPI_UNDEFINED) {
        std::vector<char>* buffer = new std::vector<char>(len);
        ParameterStream stream(buffer);
        MPI_Status recvStatus;
        MPI_Recv(stream.data(), len, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, m_comm, &recvStatus);
        FnHandle functionHandle;
        ObjectId objectId;
        TypeId typeId;
        bool getReturn;
        stream >> typeId >> objectId >> functionHandle >> getReturn;
        FunctionBase *f = m_registered_functions[functionHandle];
        f->execute(stream, recvStatus.MPI_SOURCE, this, getReturn, getObjectWrapper(m_rank, typeId, objectId)->object());
        delete buffer;
    }
}

template<typename MessageInterface>
MPI_Comm Manager<MessageInterface>::comm() const
{
    return m_comm;
}

template<typename MessageInterface>
unsigned long long Manager<MessageInterface>::stats() const
{
    return m_count;
}

template<typename MessageInterface>
int Manager<MessageInterface>::numProcs() const
{
    return m_numProcs;
}

template<typename MessageInterface>
size_t Manager<MessageInterface>::queueSize() const
{
    return m_mpiObjectMessages.size() + m_mpiMessages.size();
}

class MpiMessageInterface {};

using MpiManager = Manager<MpiMessageInterface>;

}

#endif // MPIRPCMANAGER_H

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
