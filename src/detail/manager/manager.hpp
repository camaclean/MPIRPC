#ifndef MANAGER_MANAGER_H
#define MANAGER_MANAGER_H

#include "../../manager.hpp"

template<typename MessageInterface>
mpirpc::manager<MessageInterface>::manager(MPI_Comm comm) : m_comm(comm), m_next_type_id(0), m_count(0), m_shutdown(false)
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

template<typename MessageInterface>
mpirpc::manager<MessageInterface>::~manager<MessageInterface>()
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

#endif /* MANAGER_MANAGER_H */
