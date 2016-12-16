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

#ifndef MPIRPC__REMARSHALLER_HPP
#define MPIRPC__REMARSHALLER_HPP

#include "marshaller.hpp"

namespace mpirpc
{

template<typename T, typename Buffer, typename Alignment, typename = void>
struct remarshaller
{
    static void marshal(Buffer& s, std::remove_reference_t<T>&& val)
    {
        marshaller<T,Buffer,Alignment>::marshal(s,val);
    }

    static void marshal(Buffer& s, std::remove_reference_t<T>& val)
    {
        marshaller<T,Buffer,Alignment>::marshal(s,val);
    }
};

}

#endif /* MPIRPC__REMARSHALLER_HPP */
