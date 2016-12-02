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

#ifndef MPIRPC__MANAGER_HPP
#define MPIRPC__MANAGER_HPP

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

#include "exceptions.hpp"
#include "buffer.hpp"
#include "objectwrapper.hpp"
#include "common.hpp"
#include "parameterstream.hpp"
#include "marshaller.hpp"
#include "unmarshaller.hpp"
#include "mpitype.hpp"

#include "internal/marshalling.hpp"
#include "internal/orderedcall.hpp"
#include "internal/function_attributes.hpp"

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
 * @brief The mpirpc::manager class
 *
 * The mpirpc::manager class provides remote procedure call functionality over MPI. It makes heavy usage of C++11 varidic
 * templates to automatically generate classes to serialize and unserialize function parameters and return values.
 *
 * To add support for serializing and unserializing a custom type, provide the following stream operators:
 * mpirpc::parameter_stream &operator<<(mpirpc::parameter_stream& stream, const Type &t);
 * mpirpc::parameter_stream &operator>>(mpirpc::parameter_stream& stream, Type &t);
 *
 * Currently, only one manager per rank is permitted. However, a tag prefix could be implemented to allow for
 * multiple managers, thus allowing multiple communicators to be used.
 *
 * @todo Allow function and type IDs to be specified by the user.
 */

template<typename MessageInterface, template<typename T> typename Allocator>
class manager;

template<typename MessageInterface, template<typename T> typename Allocator>
class manager
{
    struct object_info;

    /**
     * @brief The mpirpc::function_base class
     *
     * Using template metaprogramming, mpirpc::function<F> classes are created
     * for each function signature and std::function objects. Six specializations
     * are required, as each type must handle the special case of void return types.
     * This is because functions cannot be called passing a void argument.
     *
     * The function pointers are bound to instances of the subclasses.
     *
     * Static functions in a class behave as normal function pointers, not member
     * function pointers.
     */
    class function_base;

    template<typename Buffer>
    class function_base_buffer;

    /**
     * The general mpirpc::function<F> class, which is specialized to deduce additional typenames where required while only
     * requiring a single typename be passed when constructing a Function.
     */
    template<typename Buffer, typename F>
    class function;

    /**
     * Specialization of mpirpc::function<F> for functions with non-void return types.
     */
    template<typename Buffer, typename R, typename... Args>
    class function<Buffer, R(*)(Args...)>;

    /**
     * Specialization of mpirpc::function<F> for member functions witn non-void return types.
     */
    template<typename Buffer, typename Class, typename R, typename... Args>
    class function<Buffer, R(Class::*)(Args...)>;

    /**
     * Specialization of mpirpc::function<F> for std::function objects with non-void return types.
     */
    template<typename Buffer, typename R, typename... Args>
    class function<Buffer, std::function<R(Args...)>>;

public:
    using UserMessageHandler = void(*)(MPI_Status&&);

    manager(MPI_Comm comm = MPI_COMM_WORLD, const Allocator<void> &alloc = std::allocator<void>());

    /**
     * Register a type with the manager. This assigns a unique ID to the type.
     *
     * register_type<T>() must be called in the same order on all processes so that
     * the assigned IDs are consistent.
     *
     * @return The type ID
     */
    template<typename T>
    TypeId register_type();

    /**
     * @brief Query the type identifier for the class Class
     * @return The identifier associated with class Class
     */
    template<typename T>
    TypeId get_type_id() const;

    /**
     * @brief Register a lambda with the manager
     * @return The handle for the lambda
     */
    template<typename Lambda>
    FnHandle register_lambda(Lambda&& l);

    /**
     * This compile-time version allows for fast (hash table) function ID lookups
     *
     * @brief Register a function or member function with the manager
     * @param f A function pointer to the function to register
     * @return The handle associated with function #f
     */
    template<typename F, internal::unwrapped_function_type<F> f, typename Buffer = parameter_buffer<Allocator<char>>>
    FnHandle register_function();

    /**
     * This run-time version is incompatible with fast (hash table) function ID lookups
     */
    template<typename F, typename Buffer = parameter_buffer<Allocator<char>>>
    FnHandle register_function(F f);

    /**
     * @brief Query the function handle for the member function pointer #f
     * @return The handle associated with the member function pointer #f
     */

    template<typename F, internal::unwrapped_function_type<F> f>
    FnHandle get_fn_handle()
    {
        return m_registered_function_typeids[std::type_index(typeid(internal::function_identifier<internal::wrapped_function_type<F>,f>))];
    }

    /**
     * @brief Register an object with the manager. Other ranks are informed of the existance of this object
     * @return A wrapper for the object, containing a pointer to the object and the ids used to call its member functions
     */
    template<class Class>
    object_wrapper<Class>* register_object(Class *object);

    template<class Class, typename... Args>
    object_wrapper_base* construct_global_object(int rank, Args&&... args);

    /**
     * @brief invoke a function on rank #rank
     *
     * Note: An object's static functions behave as normal function pointers.
     *
     * Note: function pointer versions of manager::invoke_function are slightly
     * slower than fnhandle_t versions of ,anager::invoke_function when many functions
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
     * @param f The function pointer to invoke. This function pointer must be registered with the manager. See: manager::register_function.
     * @param function_handle The function handle for the registered function, f. If this value is 0, the function handle is searched for in the function map.
     * @param args... The function's parameters.
     * @return The return value of the function.
     */
    template<typename R, typename... FArgs, typename... Args>
    auto invoke_function_r(int rank, R(*f)(FArgs...), FnHandle function_handle, Args&&... args)
        -> typename std::enable_if<!std::is_same<R, void>::value, R>::type;

    template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
    auto invoke_function_r(int rank, Args&&... args)
        -> internal::function_return_type<F>;

    /**
     * Specialized for void return type
     *
     * @see manager::invoke_function()
     */
    template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
    void invoke_function(int rank, Args&&... args);

    template<typename R, typename... FArgs, typename...Args>
    void invoke_function(int rank, R(*f)(FArgs...), FnHandle function_handle, Args&&... args);

    /**
     * @see manager::invoke_function_r()
     *
     * Note: this function assumes that the types of the arguments passed are the same as what the function uses. Only the function return value is
     * passed back for looser type requirements.
     *
     * Local calls cannot be optimized to a simple function call because we don't know how to cast the Function object here
     */
    template<typename R, typename... Args>
    R invoke_function_r(int rank, FnHandle function_handle, Args&&... args);

    /**
     * Invoke a remote function, returning and updating non-const references. This expects the types of Args to match exactly, including const-ness and reference types.
     */
    template<typename R, typename... Args>
    R invoke_function_pr(int rank, FnHandle function_handle, Args&&... args);

    /**
     * Specialized for void return type
     *
     * @see manager::invoke_function()
     *
     * @todo Optimize local calls
     */
    template<typename... Args>
    void invoke_function(int rank, FnHandle function_handle, Args&&... args);

    /**
     * @brief Call the member function of an object remotely
     *
     * @param a The object wrapper identifying the object and its location.
     * @param f The member function pointer to invoke. This member function pointer must be registered with the manager. See: manager::register_function.
     * @param args... The function's parameters.
     * @return The return value of the function.
     */
    template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
    auto invoke_function_r(object_wrapper_base *a, Args&&... args)
        -> internal::function_return_type<F>;

    template<typename Lambda, typename... Args>
    auto invoke_lambda_r(int rank, Lambda&& l, FnHandle function_handle, Args&&... args)
        -> internal::lambda_return_type<Lambda>;

    /**
     * Version for void return type
     *
     * @see manager::invoke_function()
     */
    template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
    void invoke_function(object_wrapper_base *a, Args&&... args);

    /**
     * @brief Get the MPI rank of this process
     * @return The MPI rank of this process
     */
    int rank() const;

    /**
     * @brief Get the total number of MPI processes in the communicator
     * @return The total number of MPI processes in the communicator
     */
    int num_pes() const;

    /**
     * @brief Check for incoming commands. Also runs manager::checkSends()
     * @return True to continue running. False indicates this process should shut down.
     */
    bool check_messages();

    /**
     * @brief Checks on the status of the non-blocking sends and frees resources of completed sends.
     * @return True to continue running. False indicates this process should shut down.
     */
    bool check_sends();

    /**
     * @brief Get the first wrapper to the object of type #type_id
     * @param type_id The type identifier
     * @return A pointer to the object's wrapper
     */
    object_wrapper_base* get_object_of_type(TypeId type_id) const;

    /**
     * @brief Get the first wrapper to the object of type Class
     * @return A pointer to the object's wrapper
     */
    template<class Class>
    object_wrapper_base* get_object_of_type() const;

    /**
     * @brief Get the first wrapper to the object of type #type_id which exists on rank #rank
     * @param typeId The object's typeId
     * @param rank The rank on which the object exists
     * @return A pointer to the object's wrapper
     */
    object_wrapper_base* get_object_of_type(TypeId type_id, int rank) const;

    /**
     * @brief Get the first wrapper to the object of type Class which exists on rank #rank
     * @param rank The rank on which the object exists
     * @return A pointer to the object's wrapper
     */
    template<class Class>
    object_wrapper_base* get_object_of_type(int rank) const;

    /**
     * @brief Get the set of all objects of type #typeId for rank #rank
     * @param type_id The type identifier
     * @param rank The rank the objects exist on
     * @return A std::unordered_set of object wrapers for the type and rank
     */
    std::unordered_set<object_wrapper_base*> get_objects_of_type(TypeId type_id, int rank) const;

    /**
     * @brief Get the set of all objects of type Class for rank #rank
     * @param rank The rank the objects exist on
     * @return A std::unordered_set of object wrapers for the type and rank
     */
    template<class Class>
    std::unordered_set<object_wrapper_base*> get_objects_of_type(int rank) const;

    /**
     * @brief Get the set of all objects of type #typeId
     * @param type_id The type identifier
     * @return A std::unordered_set of object wrappers for the type
     */
    std::unordered_set<object_wrapper_base*> get_objects_of_type(mpirpc::TypeId type_id) const;

    /**
     * @brief Get the set of all objects of type Class
     * @return A std::unordered_set of all object wrappers for the type
     */
    template<class Class>
    std::unordered_set<object_wrapper_base*> get_objects_of_type() const;

    template<typename T>
    std::vector<T> reduce(std::vector<T>& vec, MPI_Op op, int root)
    {
        int vecsize = vec.size();
        std::vector<T> res(vecsize);
        MPI_Reduce(vec.data(), res.data(), vecsize, mpi_type<T>(), op, root, m_comm);
        return res;
    }

    template<typename T>
    std::vector<T> allreduce(std::vector<T>& vec,  MPI_Op op)
    {
        int vecsize = vec.size();
        std::vector<T> res(vecsize);
        MPI_Allreduce(vec.data(), res.data(), vecsize, mpi_type<T>(), op, m_comm);
        return res;
    }

    template<typename T>
    T* reduce(T* first, T* last, MPI_Op op, int root)
    {
        std::size_t size = last-first;
        T* res = new T[size];
        MPI_Reduce(first, res, size, mpi_type<T>(), op, root, m_comm);
        return res;
    }

    template<typename T>
    T* allreduce(T* first, T* last, MPI_Op op)
    {
        std::size_t size = last-first;
        T* res = new T[size];
        MPI_Allreduce(first, res, size, mpi_type<T>(), op, m_comm);
        return res;
    }

    template<class InputIt, class T, class BinaryOperation>
    T accumulate( InputIt first, InputIt last, T init, BinaryOperation op, MPI_Op mpi_op)
    {
        T intermediate = std::accumulate(first, last, init, op);
        T res;
        MPI_Allreduce(&intermediate, &res, 1, mpi_type<T>(), mpi_op, m_comm);
        return res;
    }

    template<class InputIt>
    typename std::iterator_traits<InputIt>::value_type
    accumulate( InputIt first, InputIt last)
    {
        typename std::iterator_traits<InputIt>::value_type intermediate = std::accumulate(first, last, static_cast<typename std::iterator_traits<InputIt>::value_type>(0));
        typename std::iterator_traits<InputIt>::value_type res;
        MPI_Allreduce(&intermediate, &res, 1, mpi_type<typename std::iterator_traits<InputIt>::value_type>(), MPI_SUM, m_comm);
        return res;
    }

    /**
     * @brief Get an object's wrapper, given it's id.
     * @param id The id of the object
     * @return The object wrapper
     */
    object_wrapper_base* get_object_wrapper(int rank, TypeId tid, ObjectId oid) const;

    /**
     * @brief Register a custom message handler to be invoked when an MPI message has been probed with tag #tag.
     * @param tag The tag identifying this type of message.
     * @param callback A void(*)(MPI_Status&&) function pointer. This function should handle the MPI_Recv.
     */
    void register_user_message_handler(int tag, UserMessageHandler callback);

    /**
     * @brief Send a buffer to rank #rank with tag #tag
     */
    void send_raw_message(int rank, const std::vector<char> *data, int tag = 0);

    /**
     * @brief Send a buffer to every other rank with tag #tag
     *
     * Does not send to self.
     */
    void send_raw_message_world(const std::vector<char> *data, int tag = 0);

    /**
     * @brief Executes an MPI_Barrier, then checks messages to ensure the state of all managers are in a valid state.
     *
     * When registering objects that depend on remote objects, they must be initialized in order (so that their ids are propagated).
     */
    void sync();

    /**
     * @brief Shut down all managers on all PEs. This must be called on only one rank.
     */
    void shutdown_all();

    /**
     * @brief Shut down this manager. This must be called on all ranks.
     */
    void shutdown();

    /**
     * @brief The number of function invocations this manager has handled.
     */
    unsigned long long stats() const;

    /**
     * @brief Get the MPI communicator
     */
    MPI_Comm comm() const;

    /**
     * @brief The size of the message send queue. Can check mesages continuously while queue size >0 to ensure all messages have been sent.
     */
    size_t queue_size() const;

    /**
     * Destroy this manager
     */
    ~manager();

protected:

    /**
     * @brief Send the result of executing a function back to the sending rank.
     * @param rank The rank which invoked the function
     * @param r The invoked functions return value
     */
    template<typename Buffer>
    void function_return(int rank, Buffer&& returnbuffer)
    {
        MPI_Send((void*) returnbuffer.data(), returnbuffer.size(), MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm);
    }

    /**
     * @brief Invoke a function on a remote process
     * @param rank The remote rank
     * @param function_handle The function's unique identifier
     * @param get_return Indicate to the remote process if this process will be expecting the function's return value
     * @param args The parameter pack of the function's arguments
     *
     */
    template<typename... Args>
    void send_function_invocation(int rank, FnHandle function_handle, bool get_return, Args&&... args);

    template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
    void send_function_invocation(int rank, bool get_return, Args&&... args);

    template<typename R, typename...FArgs, typename...Args>
    void send_lambda_invocation(int rank, FnHandle function_handle, bool get_return, R(*)(FArgs...), Args&&... args);

    /**
     * Invoke a member function on a remote process
     *
     * @see manager::send_function_invocation(int,bool,Args...)
     */
    template<typename F, internal::unwrapped_function_type<F> f, typename... Args>
    void send_member_function_invocation(object_wrapper_base* a, bool get_return, Args&&... args);

    /**
     * Wait for the remote process to run an invocation and send that function's return value back to this process.
     * Unserialize the result and return it.
     */
    template<typename R, typename...FArgs, typename... Args>
    auto process_return(int rank, internal::type_pack<FArgs...>, Args&&... args)
        -> typename std::enable_if<!std::is_same<R,void>::value,R>::type
    {
        std::cout << "processing return: " << typeid(internal::type_pack<FArgs...>).name() << " " << typeid(internal::bool_template_list<internal::is_pass_back<FArgs>::value...>).name() << std::endl;
        MPI_Status status;
        int len;
        int flag;
        bool shutdown;
        R ret;
        do {
            shutdown = !check_messages();
            MPI_Iprobe(rank, MPIRPC_TAG_RETURN, m_comm, &flag, &status);
        } while (!flag && !shutdown);
        if (!shutdown)
            MPI_Get_count(&status, MPI_CHAR, &len);
        if (!shutdown && len != MPI_UNDEFINED) {
            std::vector<char>* buffer = new std::vector<char>(len);
            parameter_stream stream(buffer);
            MPI_Recv((void*) buffer->data(), len, MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm, &status);
            //ret = unmarshal<R,Allocator<R>>(stream);
            using swallow = int[];
            (void)swallow{((internal::is_pass_back<FArgs>::value) ? ( internal::pass_back_unmarshaller<FArgs,Args>::unmarshal(stream,args) /*args = unmarshal<Args,Allocator<Args>>(stream)*/, 1) : 0)...};
            delete buffer;
        }
        return ret;
    }

    template<typename R, typename...FArgs, typename...Args>
    auto process_return(int rank, internal::type_pack<FArgs...>, Args&&... args)
        -> typename std::enable_if<std::is_same<R,void>::value,R>::type
    {
        std::cout << "processing return: " << typeid(internal::type_pack<FArgs...>).name() << " " << typeid(internal::bool_template_list<internal::is_pass_back<FArgs>::value...>).name() << std::endl;
        if (!internal::any_true<internal::is_pass_back<FArgs>::value...>::value && std::is_void<R>::value)
            return;
        MPI_Status status;
        int len;
        int flag;
        bool shutdown;
        do {
            shutdown = !check_messages();
            MPI_Iprobe(rank, MPIRPC_TAG_RETURN, m_comm, &flag, &status);
        } while (!flag && !shutdown);
        if (!shutdown)
            MPI_Get_count(&status, MPI_CHAR, &len);
        if (!shutdown && len != MPI_UNDEFINED) {
            std::vector<char>* buffer = new std::vector<char>(len);
            parameter_stream stream(buffer);
            MPI_Recv((void*) buffer->data(), len, MPI_CHAR, rank, MPIRPC_TAG_RETURN, m_comm, &status);
            using swallow = int[];
            (void)swallow{((internal::is_pass_back<FArgs>::value) ? (internal::pass_back_unmarshaller<FArgs,Args>::unmarshal(stream,args) /* = unmarshal<Args,Allocator<Args>>(stream)*/, 1) : 0)...};
            delete buffer;
        }
    }

    /**
     * @brief Handle a message indicating a remote process is registering a new object.
     */
    void register_remote_object();

    /**
     * @brief Notify other processes of an object registered on this manager.
     */
    void notify_new_object(mpirpc::TypeId type, mpirpc::ObjectId id);

    /**
     * @brief Handle a message to execute a function
     */
    void receivedInvocationCommand(MPI_Status &&);

    /**
     * @brief Handle a message to execute a member function
     */
    void receivedMemberInvocationCommand(MPI_Status &&);

    /**
     * @brief Handle a message indicating this manager should shut down.
     */
    void handleShutdown();

    /**
     * @brief Record a remote object with this manager
     */
    void register_remote_object(int rank, mpirpc::TypeId type, mpirpc::ObjectId id);

    std::unordered_map<std::type_index, TypeId> m_registered_type_ids;
    std::unordered_map<std::type_index, TypeId> m_registered_memory_managers;

    std::map<FnHandle, function_base*> m_registered_functions;
    std::unordered_map<std::type_index, FnHandle> m_registered_function_typeids;
    std::vector<object_wrapper_base*> m_registered_objects;

    std::unordered_map<MPI_Request, const std::vector<char>*> m_mpi_messages;
    std::unordered_map<MPI_Request, std::shared_ptr<object_info>> m_mpi_object_messages;

    std::unordered_map<int, UserMessageHandler> m_user_message_handlers;

    MPI_Comm m_comm;
    TypeId m_next_type_id;
    int m_rank;
    int m_num_pes;
    unsigned long long m_count;
    bool m_shutdown;
    MPI_Datatype m_mpi_object_info;
    Allocator<char> m_alloc;
};

template<class MessageInterface, template<typename> typename Allocator>
struct manager<MessageInterface, Allocator>::object_info {
    object_info() {}
    object_info(TypeId t, ObjectId i) : type(t), id(i) {}
    TypeId type;
    TypeId id;
};

#include "detail/manager/manager.hpp"
#include "detail/manager/function.hpp"
#include "detail/manager/register.hpp"
#include "detail/manager/invoke.hpp"

template<typename MessageInterface, template<typename> typename Allocator>
int manager<MessageInterface, Allocator>::rank() const
{
    return m_rank;
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::notify_new_object(TypeId type, ObjectId id)
{
    if (m_shutdown)
        return;
    std::shared_ptr<object_info> info(new object_info(type, id));
    for(int i = 0; i < m_num_pes; ++i)
    {
        if (i != m_rank)
        {
            MPI_Request req;
            MPI_Issend(info.get(), 1, m_mpi_object_info, i, MPIRPC_TAG_NEW, m_comm, &req);
            m_mpi_object_messages[req] = info;
        }
    }
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::send_raw_message(int rank, const std::vector<char> *data, int tag)
{
    if (check_sends() && !m_shutdown) {
        MPI_Request req;
        MPI_Issend((void*) data->data(), data->size(), MPI_CHAR, rank, tag, m_comm, &req);
        m_mpi_messages[req] = data;
    }
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::send_raw_message_world(const std::vector<char>* data, int tag)
{
    for (int i = 0; i < m_num_pes; ++i) {
#ifndef USE_MPI_LOCALLY
        if (i != m_rank) {
#endif
            send_raw_message(i, data, tag);
#ifndef USE_MPI_LOCALLY
        }
#endif
    }
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::register_user_message_handler(int tag, UserMessageHandler callback)
{
    m_user_message_handlers[tag] = callback;
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::register_remote_object()
{
    object_info info;
    MPI_Status status;
    MPI_Recv(&info, 1, m_mpi_object_info, MPI_ANY_SOURCE, MPIRPC_TAG_NEW, m_comm, &status);
    register_remote_object(status.MPI_SOURCE, info.type, info.id);
}

template<typename MessageInterface, template<typename> typename Allocator>
bool manager<MessageInterface, Allocator>::check_sends() {
    for (auto i = m_mpi_object_messages.begin(); i != m_mpi_object_messages.end();) {
        MPI_Request req = i->first;
        int flag;
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            i->second.reset();
            m_mpi_object_messages.erase(i++);
        } else {
            ++i;
        }
    }
    for (auto i = m_mpi_messages.begin(); i != m_mpi_messages.end();) {
        MPI_Request req = i->first;
        int flag;
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            delete i->second;
            m_mpi_messages.erase(i++);
        } else {
            ++i;
        }
    }
    if (m_shutdown) {
        return false;
    }
    return true;
}

template<typename MessageInterface, template<typename> typename Allocator>
bool manager<MessageInterface, Allocator>::check_messages() {
    if (m_shutdown)
        return false;
    check_sends();
    int flag = 1;
    while (flag) {
        MPI_Status status;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, m_comm, &flag, &status);
        if (flag) {
            switch (status.MPI_TAG) {
                case MPIRPC_TAG_SHUTDOWN:
                    m_shutdown = true;
                    handleShutdown();
                    check_sends();
                    return false;
                case MPIRPC_TAG_NEW:
                    register_remote_object();
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
                    UserMessageHandler func = m_user_message_handlers.at(status.MPI_TAG);
                    func(std::move(status));
            }
        }
    }
    return true;
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::sync() {
    while (queue_size() > 0) { check_messages(); } //block until this rank's queue is processed
    MPI_Request req;
    int flag;
    MPI_Ibarrier(m_comm, &req);
    do
    {
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        check_messages();
    } while (!flag); //wait until all other ranks queues have been processed
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::shutdown_all() {
    int buf = 0;
    for (int i = 0; i < m_num_pes; ++i)
    {
        if (i != m_rank) {
            MPI_Bsend((void*) &buf, 1, MPI_INT, i, MPIRPC_TAG_SHUTDOWN, m_comm);
        }
    }
    sync();
    m_shutdown = true;
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::shutdown()
{
    sync();
    m_shutdown = true;
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::handleShutdown()
{
    int buf;
    MPI_Status status;
    MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, MPIRPC_TAG_SHUTDOWN, m_comm, &status);
    sync();
    m_shutdown = true;
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::receivedInvocationCommand(MPI_Status&& status)
{
    m_count++;
    int len;
    MPI_Get_count(&status, MPI_CHAR, &len);
    if (len != MPI_UNDEFINED) {
        parameter_buffer<Allocator<char>> stream{std::vector<char,Allocator<char>>(len,m_alloc)};
        MPI_Status recv_status;
        MPI_Recv(stream.data(), len, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, m_comm, &recv_status);
        FnHandle function_handle = mpirpc::get<FnHandle>(stream,m_alloc);
        bool get_return = mpirpc::get<bool>(stream,m_alloc);
        function_base_buffer<parameter_buffer<Allocator<char>>> *f = dynamic_cast<function_base_buffer<parameter_buffer<Allocator<char>>>*>(m_registered_functions[function_handle]);
        f->execute(stream, m_alloc, recv_status.MPI_SOURCE, this, get_return);
    }
}

template<typename MessageInterface, template<typename> typename Allocator>
void manager<MessageInterface, Allocator>::receivedMemberInvocationCommand(MPI_Status&& status) {
    m_count++;
    int len;
    MPI_Get_count(&status, MPI_CHAR, &len);
    if (len != MPI_UNDEFINED) {
        parameter_buffer<Allocator<char>> stream{std::vector<char,Allocator<char>>(len,m_alloc)};
        MPI_Status recv_status;
        MPI_Recv(stream.data(), len, MPI_CHAR, status.MPI_SOURCE, status.MPI_TAG, m_comm, &recv_status);

        TypeId type_id = mpirpc::get<TypeId>(stream,m_alloc);
        ObjectId object_id = mpirpc::get<ObjectId>(stream,m_alloc);
        FnHandle function_handle = mpirpc::get<FnHandle>(stream,m_alloc);
        bool get_return = mpirpc::get<bool>(stream,m_alloc);

        function_base_buffer<parameter_buffer<Allocator<char>>> *f = dynamic_cast<function_base_buffer<parameter_buffer<Allocator<char>>>*>(m_registered_functions[function_handle]);
        f->execute(stream, m_alloc, recv_status.MPI_SOURCE, this, get_return, get_object_wrapper(m_rank, type_id, object_id)->object());
    }
}

template<typename MessageInterface, template<typename> typename Allocator>
MPI_Comm manager<MessageInterface, Allocator>::comm() const
{
    return m_comm;
}

template<typename MessageInterface, template<typename> typename Allocator>
unsigned long long manager<MessageInterface, Allocator>::stats() const
{
    return m_count;
}

template<typename MessageInterface, template<typename> typename Allocator>
int manager<MessageInterface, Allocator>::num_pes() const
{
    return m_num_pes;
}

template<typename MessageInterface, template<typename> typename Allocator>
size_t manager<MessageInterface, Allocator>::queue_size() const
{
    return m_mpi_object_messages.size() + m_mpi_messages.size();
}

class MpiMessageInterface {};

using mpi_manager = manager<MpiMessageInterface,std::allocator>;

}

#endif // MPIRPC__MANAGER_HPP

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
