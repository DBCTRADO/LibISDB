cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

project(libisdbtest CXX)
add_executable(libisdbtest ${CMAKE_CURRENT_SOURCE_DIR}/libisdbtest.cpp)
target_link_libraries(libisdbtest LibISDB ${CMAKE_THREAD_LIBS_INIT})
