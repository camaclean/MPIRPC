project(MPIRPC)
cmake_minimum_required(VERSION 2.8.12)
set(mpirpc_VERSION_MAJOR 0)
set(mpirpc_VERSION_MINOR 1)
set(mpirpc_VERSION_PATCH 0)
option(USE_LTO "Build with Link Time Optimizations" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -O3 -march=native")

#set(CMAKE_CXX_COMPILER "clang++")

include(FindMPI REQUIRED)

if(USE_LTO)
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -fuse-linker-plugin -fuse-ld=gold -fvisibility=hidden")
        set(CMAKE_AR "gcc-ar")
        set(CMAKE_NM "gcc-nm")
        set(CMAKE_RANLIB "gcc-ranlib")
    endif(CMAKE_COMPILER_IS_GNUCXX)
endif(USE_LTO)

if(MPI_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_C_COMPILE_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MPI_CXX_LINK_FLAGS}")
    include_directories("${MPI_CXX_INCLUDE_PATH}")
endif(MPI_FOUND)

set(SRC_LIST manager.cpp objectwrapper.cpp parameterstream.cpp mpitype.cpp)
add_library(mpirpc STATIC ${SRC_LIST})
target_link_libraries(mpirpc ${MPI_CXX_LIBRARIES})

install(TARGETS mpirpc DESTINATION lib EXPORT MPIRPCTargets)
install(FILES common.hpp lambda.hpp manager.hpp objectwrapper.hpp orderedcall.hpp parameterstream.hpp mpitype.hpp DESTINATION include/mpirpc)
install(EXPORT MPIRPCTargets DESTINATION lib/cmake/mpirpc)

set(INCLUDE_INSTALL_DIR include/ CACHE STRING "MPIRPC include directory for install")
set(LIB_INSTALL_DIR lib/ CACHE STRING "MPIRPC lib directory for install")
include(CMakePackageConfigHelpers)
configure_package_config_file(MPIRPCConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/MPIRPCConfig.cmake
                              INSTALL_DESTINATION ${LIB_INSTALL_DIR}/cmake/mpirpc
                              PATH_VARS INCLUDE_INSTALL_DIR)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/MPIRPCConfig.cmake
        DESTINATION ${LIB_INSTALL_DIR}/cmake/mpirpc)

set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Core Test)
if(Qt5Test_FOUND)
    enable_testing()
    add_definitions(${Qt5Core_DEFINITIONS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS})
    include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Concurrent_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
    add_subdirectory(tests)
endif(Qt5Test_FOUND)

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MPIRPC is library for remote procedure calls over MPI.")
set(CPACK_PACKAGE_VENDOR "Colin MacLean")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Colin MacLean <cmaclean@illinois.edu>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "mpich (>= 3.0.0), libgcc1 (>= 1:4.7), libstdc++6 (>= 4.7)")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_REQUIRES "mpich >= 3.0.0, libgcc1 >= 4.7, libstdc++6 >= 4.7")
include(CPack)
