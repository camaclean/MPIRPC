#Must force set in order to be correctly set by CMake on the first run of cmake.
set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/install/" CACHE STRING "Install path" FORCE)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(MPI_C_LIBRARIES "/opt/cray/mpt/default/gni/mpich2-gnu/49/lib/libmpich_gnu_49.a")
set(MPI_C_INCLUDE_PATH "/opt/cray/mpt/default/gni/mpich2-gnu/49/include/")
set(MPI_CXX_LIBRARIES "/opt/cray/mpt/default/gni/mpich2-gnu/49/lib/libmpichcxx_gnu_49.a")
set(MPI_CXX_INCLUDE_PATH "/opt/cray/mpt/default/gni/mpich2-gnu/49/include/")

