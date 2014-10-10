#ifndef ORDEREDCALL_HPP
#define ORDEREDCALL_HPP

#include "common.hpp"

namespace mpirpc {

/**
 * @brief The OrderedCall<F> class
 *
 * OrderedCall<F> can be used to call functions such that the
 * side effects of the parameters are evaluated in the order
 * in which they appear. This is done using uniform initilization:
 * constructing using {} brackets instead of (). This bound call
 * can then be executed using OrderedCall<F>::operator().
 */
template <typename F>
struct OrderedCall;

/**
 * Specialization of OrderedCall<F> for function pointer calls.
 */
template<typename R, typename... Args>
struct OrderedCall<R(*)(Args...)>
{
    OrderedCall(R(*function)(Args...), Args&&... args)
    {
        bound = std::bind(function, std::forward<Args>(args)...);
    }

    R operator()()
    {
        return bound();
    }

    std::function<R()> bound;
};

/**
 * Specialization of OrderedCall<F> for member function pointer calls.
 */
template<typename R, typename Class, typename... Args>
struct OrderedCall<R(Class::*)(Args...)>
{
    OrderedCall(R(Class::*function)(Args...), Class *c, Args&&... args)
    {
        bound = std::bind(function, c, std::forward<Args>(args)...);
    }

    R operator()()
    {
        return bound();
    }

    std::function<R()> bound;
};

/**
 * Specialization of OrderedCall<F> for std::function calls.
 */
template<typename R, typename... Args>
struct OrderedCall<std::function<R(Args...)>>
{
    OrderedCall(std::function<R(Args...)> &function, Args&&... args)
    {
        bound = std::bind(function, std::forward<Args>(args)...);
    }

    R operator()()
    {
        return bound();
    }

    std::function<R()> bound;

};

}

#endif // ORDEREDCALL_HPP
