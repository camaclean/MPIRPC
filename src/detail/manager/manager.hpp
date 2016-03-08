#ifndef MANAGER_MANAGER_H
#define MANAGER_MANAGER_H

#include "../../manager.hpp"

template<typename MessageInterface>
mpirpc::Manager<MessageInterface>::Manager(MPI_Comm comm) : m_comm(comm), m_nextTypeId(0), m_nextDeleterId(0), m_count(0), m_shutdown(false)
{
    MPI_Comm_rank(m_comm, &m_rank);
    MPI_Comm_size(comm, &m_numProcs);

    const int nitems = 2;
    int blocklengths[2] = {1,1};
    MPI_Datatype types[2] = {MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG};
    MPI_Aint offsets[2];
    offsets[0] = offsetof(ObjectInfo, type);
    offsets[1] = offsetof(ObjectInfo, id);
    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &MpiObjectInfo);
    MPI_Type_commit(&MpiObjectInfo);
    void *buffer = malloc(BUFFER_SIZE);
    MPI_Buffer_attach(buffer, BUFFER_SIZE);
    MPI_Barrier(m_comm);
}

template<typename MessageInterface>
mpirpc::Manager<MessageInterface>::~Manager<MessageInterface>()
{
    MPI_Type_free(&MpiObjectInfo);
    for (auto i : m_mpiMessages)
        delete i.second;
    for (auto i : m_mpiObjectMessages)
        i.second.reset();
    for (auto i : m_registered_functions)
        delete i.second;
    for (auto i : m_registeredObjects)
        delete i;
}

#endif /* MANAGER_MANAGER_H */
