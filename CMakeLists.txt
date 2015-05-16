cmake_minimum_required(VERSION 3.0)

project(Coppa)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
add_subdirectory(oscpack)

if(${CMAKE_MINOR_VERSION} LESS 1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

option(OPTIMIZED "Heavily optimized build" FALSE)

if(OPTIMIZED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native")
endif()
# Show the includes in IDEs
file(GLOB_RECURSE COPPA_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/coppa/*.hpp")
add_custom_target(coppa SOURCES ${COPPA_HEADERS})


# A function that sets up all the required stuff for building with coppa.
function (set_coppa_properties theTarget)
  set_property(
    TARGET ${theTarget}
    PROPERTY INCLUDE_DIRECTORIES
      "${CMAKE_CURRENT_SOURCE_DIR}"
      "${CMAKE_CURRENT_SOURCE_DIR}/variant/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/websocketpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/jeayeson/include/"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests") # For catch

  if(${CMAKE_MINOR_VERSION} GREATER 0)
    target_compile_features(${theTarget} PUBLIC cxx_constexpr cxx_noexcept cxx_variadic_templates)
    set_property(TARGET ${theTarget} PROPERTY CXX_STANDARD 14)
  endif()

  target_link_libraries(${theTarget} "${Boost_LIBRARIES}" "${CMAKE_THREAD_LIBS_INIT}" oscpack)
endfunction()

# Examples and tests
add_executable(server_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/server_test.cpp")
set_coppa_properties(server_test)

add_executable(client_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/client_test.cpp")
set_coppa_properties(client_test)

add_executable(benchmark "${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmarks/benchmark.cpp")
set_coppa_properties(benchmark)


add_executable(test_oscquery_map "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/oscquery/map.cpp")
set_coppa_properties(test_oscquery_map)
add_executable(test_json_writer "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/writer.cpp")
set_coppa_properties(test_json_writer)
add_executable(test_json_parser "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/parser.cpp")
set_coppa_properties(test_json_parser)
