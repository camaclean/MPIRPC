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

#ifndef MPIRPC__BUFFER_HPP
#define MPIRPC__BUFFER_HPP

namespace mpirpc
{

/*template<typename T>
struct type_alignment_helper
{
    static constexpr std::size_t alignment()
    {
        return alignof(T);
    }
};

template<typename T>
struct type_alignment_helper<mpirpc::pointer_wrapper<T>>
{
    static constexpr std::size_t alignment()
    {
        return alignof(T);
    }
};*/

template<typename T, std::size_t Alignment>
constexpr std::size_t type_alignment = Alignment;

template<typename T, typename Buffer, typename Allocator>
inline decltype(auto) get(Buffer& b, Allocator&& a)
{
    return b.template pop<T>(std::forward<Allocator>(a));
}

template<std::size_t Alignment, typename Buffer, typename T>
void get_pointer_from_buffer(Buffer& b, T*& t)
{
    b.template realign<Alignment>();
    t = b.template reinterpret_and_advance<T>(sizeof(T));
}

}

#endif /* MPIRPC__BUFFER_HPP */
