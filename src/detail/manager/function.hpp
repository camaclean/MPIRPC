#ifndef MANAGER_FUNCTION_H
#define MANAGER_FUNCTION_H

#include "../../manager.hpp"

template <typename Tuple, size_t... I>
decltype(auto) pass_back_impl(ParameterStream& p, Tuple&& t, std::index_sequence<I...>)
{
    using swallow = int[];
    (void)swallow{(marshal(p,std::get<I>(std::forward<Tuple>(t))), 0)...};
}

template <typename F, typename Tuple>
decltype(auto) pass_back(F&& f, ParameterStream& p, Tuple&& t)
{
    using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>;
    return pass_back_impl(std::forward<F>(f), p, std::forward<Tuple>(t), Indices{});
}

template<std::size_t Pos, bool Head, bool... Tails>
struct bool_vartem_pos
{
    static constexpr bool value = (Pos == sizeof...(Tails)+1) ? Head : bool_vartem_pos<Pos,Tails...>::value;
};

template<std::size_t Pos, typename T>
struct get_pass_back_at;

template<std::size_t Pos, bool...Vs>
struct get_pass_back_at<Pos,bool_tuple<Vs...>>
{
    static constexpr bool value = bool_vartem_pos<Pos,Vs...>::value;
};

template<class MessageInterface>
class mpirpc::manager<MessageInterface>::function_base
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
    virtual void execute(ParameterStream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) = 0;

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

template<typename MessageInterface>
FnHandle mpirpc::manager<MessageInterface>::function_base::id_counter_ = 0;

template<class MessageInterface>
template<typename R, typename... Args>
class mpirpc::manager<MessageInterface>::function<R(*)(Args...)> : public function_base
{
    using mpirpc::manager<MessageInterface>::function_base::m_pointer;
public:
    using function_type = typename storage_function_parts<R(*)(Args...)>::function_type;

    function(function_type f) : function_base(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

    virtual void execute(ParameterStream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
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
        {
            R ret = call();
            manager->function_return(sender_rank, std::move(ret), call.args_tuple, bool_tuple<is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
        }
        else
        {
            call();
        }
    }

protected:
    function_type func;
};

template<class MessageInterface>
template<typename... Args>
class mpirpc::manager<MessageInterface>::function<void(*)(Args...)> : public function_base
{
    using mpirpc::manager<MessageInterface>::function_base::m_pointer;
public:
    using function_type = typename storage_function_parts<void(*)(Args...)>::function_type;//void(*)(Args...);

    function(function_type f) : function_base(), func(f) { m_pointer = reinterpret_cast<void(*)()>(f); }

    virtual void execute(ParameterStream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        OrderedCall<function_type> call{func, unmarshal<typename remove_all_const<Args>::type>(params)...};
        call();

        if (get_return)
            manager->function_return(sender_rank, call.args_tuple, bool_tuple<is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
    }

protected:
    function_type func;
};

template<typename MessageInterface>
template<typename Class, typename R, typename... Args>
class mpirpc::manager<MessageInterface>::function<R(Class::*)(Args...)> : public function_base
{
public:
    using function_type = typename storage_function_parts<R(Class::*)(Args...)>::function_type;

    function(function_type f) : function_base(), func(f) {  }

    virtual void execute(ParameterStream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        assert(object);
        assert(manager);
        OrderedCall<function_type> call{func, static_cast<Class*>(object), unmarshal<typename remove_all_const<Args>::type>(params)...};
        if (get_return)
        {
            R ret = call();
            manager->function_return(sender_rank, std::move(ret), call.args_tuple, bool_tuple<is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
        }
        else
        {
            call();
        }
    }

    function_type func;
};

template<typename MessageInterface>
template<typename Class, typename... Args>
class mpirpc::manager<MessageInterface>::function<void(Class::*)(Args...)> : public function_base
{
public:
    using function_type = typename storage_function_parts<void(Class::*)(Args...)>::function_type;

    function(function_type f) : function_base(), func(f) { }

    virtual void execute(ParameterStream& params, int sender_rank, manager *manager, bool get_return = false, void* object = 0) override
    {
        assert(object);
        OrderedCall<function_type> call{func, static_cast<Class*>(object), unmarshal<typename remove_all_const<Args>::type>(params)...};
        call();
        if (get_return)
            manager->function_return(sender_rank, call.args_tuple, bool_tuple<is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
    }

    function_type func;
};

template<typename MessageInterface>
template<typename R, typename... Args>
class mpirpc::manager<MessageInterface>::function<std::function<R(Args...)>> : public function_base
{
public:
    using function_type = std::function<R(Args...)>;

    function(function_type& f) : function_base(), func(f) {}

    virtual void execute(ParameterStream &params, int sender_rank, manager *manager, bool get_return, void *object = 0) override
    {
        assert(manager);
        OrderedCall<function_type> call{func, unmarshal<typename remove_all_const<Args>::type>(params)...};

        if (get_return)
        {
            R ret = call();
            manager->function_return(sender_rank, std::move(ret), call.args_tuple, bool_tuple<is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
        }
        else
        {
            call();
        }
    }

    function_type func;
};

template<typename MessageInterface>
template<typename... Args>
class mpirpc::manager<MessageInterface>::function<std::function<void(Args...)>> : public function_base
{
public:
    using function_type = std::function<void(Args...)>;

    function(function_type& f) : function_base(), func(f) {}

    virtual void execute(ParameterStream &params, int sender_rank, manager *manager, bool get_return, void *object = 0) override
    {
        OrderedCall<function_type> call{func, unmarshal<typename remove_all_const<Args>::type>(params)...};
        call();
        if (get_return)
            manager->function_return(sender_rank, call.args_tuple, bool_tuple<is_pass_back<Args>::value...>{}, std::make_index_sequence<sizeof...(Args)>());
    }

    function_type func;
};

#endif /* MANAGER_FUNCTION_H */
