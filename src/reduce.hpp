#ifndef REDUCE_HPP
#define REDUCE_HPP

#include <type_traits>
#include <mpi.h>

namespace mpirpc
{

char reduce(char value, MPI_Op op, int count, int root, MPI_Comm comm);
short reduce(short value, MPI_Op op, int count, int root, MPI_Comm comm);
unsigned short reduce(unsigned short value, MPI_Op op, int count, int root, MPI_Comm comm);
int reduce(int value, MPI_Op op, int count, int root, MPI_Comm comm);
unsigned int reduce(unsigned int value, MPI_Op op, int count, int root, MPI_Comm comm);
long reduce(long value, MPI_Op op, int count, int root, MPI_Comm comm);
unsigned long reduce(unsigned long value, MPI_Op op, int count, int root, MPI_Comm comm);
float reduce(float value, MPI_Op op, int count, int root, MPI_Comm comm);
double reduce(double value, MPI_Op op, int count, int root, MPI_Comm comm);

char allreduce(char value, MPI_Op op, int count, MPI_Comm comm);
short allreduce(short value, MPI_Op op, int count, MPI_Comm comm);
unsigned short allreduce(unsigned short value, MPI_Op op, int count, MPI_Comm comm);
int allreduce(int value, MPI_Op op, int count, MPI_Comm comm);
unsigned int allreduce(unsigned int value, MPI_Op op, int count, MPI_Comm comm);
long allreduce(long value, MPI_Op op, int count, MPI_Comm comm);
unsigned long allreduce(unsigned long value, MPI_Op op, int count, MPI_Comm comm);
float allreduce(float value, MPI_Op op, int count, MPI_Comm comm);
double allreduce(double value, MPI_Op op, int count, MPI_Comm comm);

}

#endif // REDUCE_HPP
