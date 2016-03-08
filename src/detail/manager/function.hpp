#ifndef MANAGER_FUNCTION_H
#define MANAGER_FUNCTION_H

#include "../../manager.hpp"

template<class MessageInterface>
class mpirpc::Manager<MessageInterface>::FunctionBase
{
public:
    using generic_fp_type = void(*)();

    FunctionBase() : m_id(make_id()), m_pointer(0) {}

    /**
        * @brief execute Execute the function
        * @param params The serialized function parameters
        * @param sender_rank The rank requesting this function be invoked
        * @param manager The Manager, when sending back the function return value
        * @param get_return Only send the function return value back to the sending rank when requested, as the return value may not be needed.
        * @param object When the function is a member function, use object as the <i>this</i> pointer.
        */
    virtual void execute(ParameterStream& params, int sender_rank, Manager *manager, bool get_return = false, void* object = 0) = 0;

    fnhandle_t id() const { return m_id; }
    generic_fp_type pointer() const { return m_pointer; }

    virtual ~FunctionBase() {};

private:
    static fnhandle_t make_id() {
        return ++id_counter_;
    }

    fnhandle_t m_id;
    static fnhandle_t id_counter_;
protected:
    generic_fp_type m_pointer;
    std::function<void()> m_function;
};

template<typename MessageInterface>
fnhandle_t mpirpc::Manager<MessageInterface>::FunctionBase::id_counter_ = 0;

template<class MessageInterface>
template<typename R, typename... Args>
class mpirpc::Manager<MessageInterface>::Function<R(*)(Args...)> : public FunctionBase
{
    using mpirpc::Manager<MessageInterface>::FunctionBase::m_pointer;
public:
    using function_type = typename storage_function_parts<R(*)(Args...)>::function_type;

    Function(function_type f) : FunctionBase(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

    virtual void execute(ParameterStream& params, int sender_rank, Manager *manager, bool get_return = false, void* object = 0) override
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
        OrderedCall<function_type> call{func, unmarshal<typename remove_all_const<Args>::type>(params)...};
        if (get_return)
            manager->functionReturn(sender_rank, call());
        else
            call();
    }

protected:
    function_type func;
};

template<class MessageInterface>
template<typename... Args>
class mpirpc::Manager<MessageInterface>::Function<void(*)(Args...)> : public FunctionBase
{
    using mpirpc::Manager<MessageInterface>::FunctionBase::m_pointer;
public:
    using function_type = typename storage_function_parts<void(*)(Args...)>::function_type;//void(*)(Args...);

    Function(function_type f) : FunctionBase(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

    virtual void execute(ParameterStream& params, int sender_rank, Manager *manager, bool get_return = false, void* object = 0) override
    {
        OrderedCall<function_type> call{func, unmarshal<typename remove_all_const<Args>::type>(params)...};
        call();
    }

protected:
    function_type func;
};

template<typename MessageInterface>
template<typename Class, typename R, typename... Args>
class mpirpc::Manager<MessageInterface>::Function<R(Class::*)(Args...)> : public FunctionBase
{
public:
    using function_type = typename storage_function_parts<R(Class::*)(Args...)>::function_type;

    Function(function_type f) : FunctionBase(), func(f) {  }

    virtual void execute(ParameterStream& params, int sender_rank, Manager *manager, bool get_return = false, void* object = 0) override
    {
        assert(object);
        assert(manager);
        OrderedCall<function_type> call{func, static_cast<Class*>(object), unmarshal<typename remove_all_const<Args>::type>(params)...};
        if (get_return)
            manager->functionReturn(sender_rank, call());
        else
            call();
    }

    function_type func;
};

template<typename MessageInterface>
template<typename Class, typename... Args>
class mpirpc::Manager<MessageInterface>::Function<void(Class::*)(Args...)> : public FunctionBase
{
public:
    using function_type = typename storage_function_parts<void(Class::*)(Args...)>::function_type;

    Function(function_type f) : FunctionBase(), func(f) { }

    virtual void execute(ParameterStream& params, int sender_rank, Manager *manager, bool get_return = false, void* object = 0) override
    {
        assert(object);
        OrderedCall<function_type> call{func, static_cast<Class*>(object), unmarshal<typename remove_all_const<Args>::type>(params)...};
        call();
    }

    function_type func;
};

template<typename MessageInterface>
template<typename R, typename... Args>
class mpirpc::Manager<MessageInterface>::Function<std::function<R(Args...)>> : public FunctionBase
{
public:
    using function_type = std::function<R(Args...)>;

    Function(function_type& f) : FunctionBase(), func(f) {}

    virtual void execute(ParameterStream &params, int sender_rank, Manager *manager, bool get_return, void *object = 0) override
    {
        assert(manager);
        OrderedCall<function_type> call{func, unmarshal<typename remove_all_const<Args>::type>(params)...};

        if (get_return)
            manager->functionReturn(sender_rank, call());
        else
            call();
    }

    function_type func;
};

template<typename MessageInterface>
template<typename... Args>
class mpirpc::Manager<MessageInterface>::Function<std::function<void(Args...)>> : public FunctionBase
{
public:
    using function_type = std::function<void(Args...)>;

    Function(function_type& f) : FunctionBase(), func(f) {}

    virtual void execute(ParameterStream &params, int sender_rank, Manager *manager, bool get_return, void *object = 0) override
    {
        OrderedCall<function_type> call{func, unmarshal<typename remove_all_const<Args>::type>(params)...};
        call();
    }

    function_type func;
};

#endif /* MANAGER_FUNCTION_H */
