cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_COMPILER clang++) # gcc-4.9 doesn't cut it...

project(Coppa)

find_package(Boost REQUIRED COMPONENTS system)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/variant/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/websocketpp")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/jeayeson/include/")

add_executable(coppa_test "${CMAKE_CURRENT_SOURCE_DIR}/test.cpp")
set_property(TARGET coppa_test PROPERTY CXX_STANDARD 14)

target_link_libraries(coppa_test "${Boost_LIBRARIES}")
