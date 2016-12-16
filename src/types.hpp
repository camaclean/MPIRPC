/*
 * MPIRPC: MPI based invocation of functions on other ranks
 * Copyright (C) 2014-2016 Colin MacLean <cmaclean@illinois.edu>
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

#ifndef MPIRPC__TYPES_HPP
#define MPIRPC__TYPES_HPP

#include <tuple>

namespace mpirpc
{

template<typename T>
struct type_identifier
{
    static constexpr uintptr_t id()
    {
        return reinterpret_cast<uintptr_t>(&type_identifier<T>::id);
    }
};

template<typename Buffer>
struct aligned_binary_buffer_identifier : std::false_type {};

template<typename Buffer>
constexpr bool is_aligned_binary_buffer = aligned_binary_buffer_identifier<std::remove_reference_t<Buffer>>::value;

template<typename T, typename Buffer,typename=void>
struct buildtype_helper
{
    constexpr static bool value = !(std::is_scalar<std::remove_reference_t<T>>::value && is_aligned_binary_buffer<Buffer> && !std::is_pointer<T>::value);
};

template<typename T, std::size_t N, typename Buffer>
struct buildtype_helper<T[N],Buffer>
{
    constexpr static bool value = !(std::is_scalar<std::remove_reference_t<T>>::value && is_aligned_binary_buffer<Buffer> && !std::is_pointer<T>::value);
};

template<typename T, typename Buffer>
constexpr bool is_buildtype = buildtype_helper<T,Buffer>::value;

template<typename T, std::size_t Alignment>
struct type_default_alignment_helper
{
    using type = std::integral_constant<std::size_t,Alignment>;
};

template<typename... Ts, std::size_t Alignment>
struct type_default_alignment_helper<std::tuple<Ts...>,Alignment>
{
    using type = std::tuple<std::integral_constant<std::size_t,Alignment>, typename type_default_alignment_helper<Ts,alignof(Ts)>::type...>;
};

template<typename T, std::size_t Alignment>
using type_default_alignment = typename type_default_alignment_helper<T,Alignment>::type;

template<typename T>
struct is_tuple : std::false_type {};

template<typename... Ts>
struct is_tuple<std::tuple<Ts...>> : std::true_type {};

}

#endif /* MPIRPC__TYPES_HPP */
