cmake_minimum_required(VERSION 3.0)

project(Coppa)

find_package(Boost REQUIRED COMPONENTS system)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/variant/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/websocketpp")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/jeayeson/include/")

if(${CMAKE_MINOR_VERSION} LESS 1)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

file(GLOB_RECURSE COPPA_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/coppa/*.hpp")

add_executable(coppa_test
	"${CMAKE_CURRENT_SOURCE_DIR}/test.cpp"
	 ${COPPA_HEADERS})

if(${CMAKE_MINOR_VERSION} GREATER 0)
	target_compile_features(coppa_test PUBLIC cxx_constexpr cxx_noexcept cxx_variadic_templates)
	set_property(TARGET coppa_test PROPERTY CXX_STANDARD 14)
endif()

target_link_libraries(coppa_test "${Boost_LIBRARIES}")
