cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

project(epgdatatojson CXX)
add_executable(epgdatatojson ${CMAKE_CURRENT_SOURCE_DIR}/epgdatatojson.cpp)
target_link_libraries(epgdatatojson LibISDB ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS epgdatatojson RUNTIME DESTINATION bin)

project(tslogoextract CXX)
add_executable(tslogoextract ${CMAKE_CURRENT_SOURCE_DIR}/tslogoextract.cpp)
target_link_libraries(tslogoextract LibISDB ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS tslogoextract RUNTIME DESTINATION bin)

project(tspidinfo CXX)
add_executable(tspidinfo ${CMAKE_CURRENT_SOURCE_DIR}/tspidinfo.cpp)
target_link_libraries(tspidinfo LibISDB ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS tspidinfo RUNTIME DESTINATION bin)
