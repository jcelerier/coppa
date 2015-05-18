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
add_custom_target(coppa_headers SOURCES ${COPPA_HEADERS})



add_library(coppa INTERFACE)
target_include_directories(coppa INTERFACE
      "${CMAKE_CURRENT_SOURCE_DIR}/."
      "${CMAKE_CURRENT_SOURCE_DIR}/variant/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/websocketpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/jeayeson/include/"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests")

target_compile_features(coppa INTERFACE cxx_decltype_auto cxx_constexpr cxx_noexcept cxx_variadic_templates cxx_lambda_init_captures)
target_link_libraries(coppa INTERFACE "${Boost_LIBRARIES}" "${CMAKE_THREAD_LIBS_INIT}" oscpack)

# Examples and tests
add_executable(server_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/server_test.cpp")
target_link_libraries(server_test coppa)

add_executable(client_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/client_test.cpp")
target_link_libraries(client_test coppa)

add_executable(benchmark "${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmarks/benchmark.cpp")
target_link_libraries(benchmark coppa)


add_executable(test_oscquery_map "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/oscquery/map.cpp")
target_link_libraries(test_oscquery_map coppa)
add_executable(test_json_writer "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/writer.cpp")
target_link_libraries(test_json_writer coppa)
add_executable(test_json_parser "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/parser.cpp")
target_link_libraries(test_json_parser coppa)

# Qt examples
find_package(Qt5 5.2 REQUIRED COMPONENTS Core Widgets Gui Multimedia )
if(Qt5_FOUND)
  set(CMAKE_AUTOMOC ON)
  include_directories(.)

  add_executable(qt_expose_object "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/qt/qt_expose_object.cpp")
  target_link_libraries(qt_expose_object coppa)
  target_link_libraries(qt_expose_object Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Multimedia)

  add_executable(qt_update_object "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/qt/qt_update_object.cpp")
  target_link_libraries(qt_update_object coppa)
  target_link_libraries(qt_update_object Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Multimedia)
endif()

