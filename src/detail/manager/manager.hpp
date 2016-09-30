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

#ifndef MPIRPC__DETAIL__MANAGER_MANAGER_HPP
#define MPIRPC__DETAIL__MANAGER_MANAGER_HPP

#include "../../manager.hpp"

template<typename MessageInterface, template<typename> typename Allocator>
mpirpc::manager<MessageInterface, Allocator>::manager(MPI_Comm comm) : m_comm(comm), m_next_type_id(0), m_count(0), m_shutdown(false)
{
    MPI_Comm_rank(m_comm, &m_rank);
    MPI_Comm_size(comm, &m_num_pes);

    const int nitems = 2;
    int blocklengths[2] = {1,1};
    MPI_Datatype types[2] = {MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG};
    MPI_Aint offsets[2];
    offsets[0] = offsetof(object_info, type);
    offsets[1] = offsetof(object_info, id);
    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &m_mpi_object_info);
    MPI_Type_commit(&m_mpi_object_info);
    void *buffer = malloc(BUFFER_SIZE);
    MPI_Buffer_attach(buffer, BUFFER_SIZE);
    MPI_Barrier(m_comm);
}

template<typename MessageInterface, template<typename> typename Allocator>
mpirpc::manager<MessageInterface, Allocator>::~manager<MessageInterface, Allocator>()
{
    MPI_Type_free(&m_mpi_object_info);
    for (auto i : m_mpi_messages)
        delete i.second;
    for (auto i : m_mpi_object_messages)
        i.second.reset();
    for (auto i : m_registered_functions)
        delete i.second;
    for (auto i : m_registered_objects)
        delete i;
}

#endif /* MPIRPC__DETAIL__MANAGER_MANAGER_HPP */

// kate: space-indent on; indent-width 4; mixedindent off; indent-mode cstyle;
