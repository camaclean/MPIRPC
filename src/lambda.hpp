#ifndef LAMBDA_HPP
#define LAMBDA_HPP

#include <functional>

namespace mpirpc {

template <typename F>
struct LambdaTraits : public LambdaTraits<decltype(&F::operator())>
{};

template <typename C, typename R, typename... Args>
struct LambdaTraits<R(C::*)(Args...) const>
{
    //using lambda_fnPtr       = R(*)(Args...);
    using lambda_stdfunction = std::function<R(Args...)>;
};

}

#endif // LAMBDA_HPP
