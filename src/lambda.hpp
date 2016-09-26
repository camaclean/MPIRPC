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

#ifndef LAMBDA_HPP
#define LAMBDA_HPP

#include <functional>
#include <tuple>
#include "common.hpp"

namespace mpirpc {

template <typename F>
struct LambdaTraits : public LambdaTraits<decltype(&std::remove_reference_t<F>::operator())>
{};

template <typename C, typename R, typename... Args>
struct LambdaTraits<R(C::*)(Args...) const>
{
    using lambda_fnPtr       = R(*)(Args...);
    using lambda_stdfunction = std::function<R(Args...)>;
    using return_type = R;
    using pass_backs = bool_tuple<is_pass_back<Args>::value...>;
    using args_tuple = std::tuple<Args...>;
};

}

#endif // LAMBDA_HPP
