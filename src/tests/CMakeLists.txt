include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Test_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

set(streamtest_SRCS mpirpctest.cpp ../manager.hpp ../common.hpp
    ../objectwrapper.hpp ../objectwrapper.cpp ../reduce.hpp ../reduce.cpp
    ../parameterstream.cpp ../parameterstream.hpp)
add_executable(streamTest ${streamtest_SRCS})
add_test(streamTest streamTest)


add_executable(example example.cpp)
target_link_libraries(example mpirpc)
target_link_libraries(streamTest ${Qt5Core_LIBRARIES} ${Qt5Test_LIBRARIES} ${MPI_CXX_LIBRARIES})
