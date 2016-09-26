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

#include "mpitype.hpp"

template<>
MPI_Datatype mpi_type<double>() { return MPI_DOUBLE; }

template<>
MPI_Datatype mpi_type<long double>() { return MPI_LONG_DOUBLE; }

template<>
MPI_Datatype mpi_type<float>() { return MPI_FLOAT; }

template<>
MPI_Datatype mpi_type<char>() { return MPI_CHAR; }

template<>
MPI_Datatype mpi_type<unsigned char>() { return MPI_UNSIGNED_CHAR; }

template<>
MPI_Datatype mpi_type<short>() { return MPI_SHORT; }

template<>
MPI_Datatype mpi_type<unsigned short>() { return MPI_UNSIGNED_SHORT; }

template<>
MPI_Datatype mpi_type<int>() { return MPI_INT; }

template<>
MPI_Datatype mpi_type<unsigned int>() { return MPI_UNSIGNED; }

template<>
MPI_Datatype mpi_type<long>() { return MPI_LONG; }

template<>
MPI_Datatype mpi_type<unsigned long>() { return MPI_UNSIGNED_LONG; }

template<>
MPI_Datatype mpi_type<long long>() { return MPI_LONG_LONG; }

template<>
MPI_Datatype mpi_type<unsigned long long>() { return MPI_UNSIGNED_LONG_LONG; }
