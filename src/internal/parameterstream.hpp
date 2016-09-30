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

#ifndef MPIRPC__INTERNAL__PARAMETERSTREAM_HPP
#define MPIRPC__INTERNAL__PARAMETERSTREAM_HPP

#include "../parameterstream.hpp"

namespace mpirpc
{

namespace internal
{

/**
 * \internal
 * Get the number of elements from the mpirpc::pointer_wrapper.
 * Now that static and dynamic sized mpirpc::pointer_wrapers, this is not
 * strictly required, since the size is now always written to the stream,
 * but it does check if the received size matches the expected size for
 * static sized wrappers.
 */
template<std::size_t N>
struct pointer_wrapper_stream_size;

/**
 * \internal
 * Create a mpirpc::pointer_wrapper. This allows for always passing the size,
 * even though static sized pointers do not take a size argument.
 */
template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct pointer_wrapper_factory;

/*************************************************************************************/
/*************************************************************************************/
/*                                  Implementation                                   */
/*************************************************************************************/
/*************************************************************************************/

/*************************************************************************************/
/*                   mpirpc::internal::pointer_wrapper_stream_size                   */
/*************************************************************************************/

template<std::size_t N>
struct pointer_wrapper_stream_size
{
    static std::size_t get(::mpirpc::parameter_stream& s)
    {
        std::size_t size;
        s >> size;
        if (size != N)
            std::cerr << "Warning! Data size in stream does not match expected size (" << size << " != " << N << ")!" << std::endl;
        return size;
    }
};

template<>
struct pointer_wrapper_stream_size<0>
{
    static std::size_t get(::mpirpc::parameter_stream& s)
    {
        std::size_t size;
        s >> size;
        return size;
    }
};

/*************************************************************************************/
/*                     mpirpc::internal::pointer_wrapper_factory                     */
/*************************************************************************************/

template<typename T, std::size_t N, bool PassOwnership, bool PassBack, typename Allocator>
struct pointer_wrapper_factory
{
    static ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator> create(T* data, std::size_t size)
    {
        return ::mpirpc::pointer_wrapper<T,N,PassOwnership,PassBack,Allocator>(data);
    }
};

template<typename T, bool PassOwnership, bool PassBack, typename Allocator>
struct pointer_wrapper_factory<T,0,PassOwnership,PassBack,Allocator>
{
    static ::mpirpc::pointer_wrapper<T,0,PassOwnership,PassBack,Allocator> create(T* data, std::size_t size)
    {
        return ::mpirpc::pointer_wrapper<T,0,PassOwnership,PassBack,Allocator>(data,size);
    }
};

}

}

#endif /* MPIRPC__DETAIL__PARAMETERSTREAM_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
