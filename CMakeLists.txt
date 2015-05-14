cmake_minimum_required(VERSION 3.0)

project(Coppa)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
add_subdirectory(oscpack)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/variant/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/websocketpp")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/jeayeson/include/")

if(${CMAKE_MINOR_VERSION} LESS 1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

file(GLOB_RECURSE COPPA_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/coppa/*.hpp")

add_executable(server_test
    "${CMAKE_CURRENT_SOURCE_DIR}/server_test.cpp"
     ${COPPA_HEADERS})

if(${CMAKE_MINOR_VERSION} GREATER 0)
    target_compile_features(server_test PUBLIC cxx_constexpr cxx_noexcept cxx_variadic_templates)
    set_property(TARGET server_test PROPERTY CXX_STANDARD 14)
endif()

target_link_libraries(server_test "${Boost_LIBRARIES}" "${CMAKE_THREAD_LIBS_INIT}" oscpack)


add_executable(client_test
    "${CMAKE_CURRENT_SOURCE_DIR}/client_test.cpp"
     ${COPPA_HEADERS})

if(${CMAKE_MINOR_VERSION} GREATER 0)
    target_compile_features(client_test PUBLIC cxx_constexpr cxx_noexcept cxx_variadic_templates)
    set_property(TARGET client_test PROPERTY CXX_STANDARD 14)
endif()

target_link_libraries(client_test "${Boost_LIBRARIES}" "${CMAKE_THREAD_LIBS_INIT}" oscpack)
