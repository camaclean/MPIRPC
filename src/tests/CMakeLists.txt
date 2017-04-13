#include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Test_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

#set(streamtest_SRCS mpirpctest.cpp ../manager.hpp ../common.hpp
#    ../objectwrapper.hpp ../objectwrapper.cpp ../reduce.hpp ../reduce.cpp
#    ../parameterstream.cpp ../parameterstream.hpp)
#add_executable(streamTest ${streamtest_SRCS})
#add_test(streamTest streamTest)


#add_executable(example example.cpp)
#target_link_libraries(example mpirpc)
#target_link_libraries(streamTest ${Qt5Core_LIBRARIES} ${Qt5Test_LIBRARIES} ${MPI_CXX_LIBRARIES})

add_executable(type_massaging_test type_massaging_test.cpp)
target_link_libraries(type_massaging_test mpirpc ${GTEST_BOTH_LIBRARIES})

add_executable(reconstruct_test reconstruct_test.cpp)
target_link_libraries(reconstruct_test mpirpc ${GTEST_BOTH_LIBRARIES})

add_executable(utility_test utility_test.cpp)
target_link_libraries(utility_test mpirpc ${GTEST_BOTH_LIBRARIES})

add_executable(manager_test manager_test.cpp)
target_link_libraries(manager_test mpirpc ${GTEST_BOTH_LIBRARIES} ${MPI_CXX_LIBRARIES})
