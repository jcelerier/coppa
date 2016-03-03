cmake_minimum_required(VERSION 3.0)

project(Coppa)

find_package(Boost 1.58 REQUIRED COMPONENTS system thread)
find_package(Threads REQUIRED)
add_subdirectory(oscpack)

option(OPTIMIZED "Heavily optimized build" FALSE)
option(EXAMPLES "Build examples" FALSE)

if(OPTIMIZED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native -flto")
endif()

find_package(Qt5 5.5 REQUIRED COMPONENTS Core Widgets Gui Multimedia )
find_package(KF5DNSSD NO_MODULE)
if(KF5DNSSD_FOUND)
# ZeroConf sutff
include_directories(/usr/include/KF5/KDNSSD)
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
      "${CMAKE_CURRENT_SOURCE_DIR}/nano-signal-slots"
      "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests"
      )

target_compile_features(coppa INTERFACE cxx_decltype_auto cxx_constexpr cxx_noexcept cxx_variadic_templates cxx_lambda_init_captures)
target_link_libraries(coppa INTERFACE "${Boost_LIBRARIES}" "${CMAKE_THREAD_LIBS_INIT}" oscpack)


# Examples and tests
if(EXAMPLES)
add_executable(server_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/server_test.cpp")
target_link_libraries(server_test coppa)

add_executable(client_test "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/client_test.cpp")
target_link_libraries(client_test coppa)

add_executable(benchmark "${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmarks/benchmark.cpp")
target_link_libraries(benchmark coppa)
add_executable(minuit_send_perf "${CMAKE_CURRENT_SOURCE_DIR}/tests/benchmarks/minuit/send_perf.cpp")
target_link_libraries(minuit_send_perf coppa)


add_executable(ossia_osc_server "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/ossia/ossia_osc_server.cpp")
target_link_libraries(ossia_osc_server coppa)
add_executable(ossia_minuit_remote "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/ossia/ossia_minuit_remote.cpp")
target_link_libraries(ossia_minuit_remote coppa)
add_executable(ossia_minuit_remote_future "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/ossia/ossia_minuit_remote_future.cpp")
target_link_libraries(ossia_minuit_remote_future coppa)
add_executable(ossia_minuit_listener_local "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/ossia/ossia_minuit_listener_local.cpp")
target_link_libraries(ossia_minuit_listener_local coppa)

add_executable(test_oscquery_map "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/oscquery/map.cpp")
target_link_libraries(test_oscquery_map coppa)
add_executable(test_json_writer "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/writer.cpp")
target_link_libraries(test_json_writer coppa)
add_executable(test_json_parser "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/parser.cpp")
target_link_libraries(test_json_parser coppa)
add_executable(test_json_parse_examples "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/parse_examples.cpp")
target_link_libraries(test_json_parse_examples coppa)
add_executable(test_ossia "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/ossia/ossia_test.cpp")
target_link_libraries(test_ossia coppa)
add_executable(test_ossia_conversion "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/ossia/conversion_tests.cpp")
target_link_libraries(test_ossia_conversion coppa)

# For tests
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/json_files"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/tests/tests/json/conformance"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# Qt examples
if(Qt5_FOUND)
  set(CMAKE_AUTOMOC ON)
  include_directories(.)


  add_executable(qt_expose_object "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/qt/qt_expose_object.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/qt/ThreadManager.cpp")
  target_link_libraries(qt_expose_object coppa)
  target_link_libraries(qt_expose_object Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Multimedia)

  add_executable(qt_update_object "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/qt/qt_update_object.cpp")
  target_link_libraries(qt_update_object coppa)
  target_link_libraries(qt_update_object Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Multimedia)

  if(KF5DNSSD_FOUND)

  add_executable(zc_server "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/zeroconf/zc_server.cpp")
  target_link_libraries(zc_server coppa)
  target_link_libraries(zc_server Qt5::Core Qt5::Network KF5DNSSD)

  add_executable(zc_client "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/zeroconf/zc_client.cpp")
  target_link_libraries(zc_client coppa)
  target_link_libraries(zc_client Qt5::Core Qt5::Network KF5DNSSD)

  endif()
endif()
endif()
