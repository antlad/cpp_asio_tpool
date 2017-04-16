cmake_minimum_required(VERSION 2.8)

project(asio_tpool)

set(CMAKE_CXX_STANDARD 14)

cmake_minimum_required(VERSION 3.1)

option(ENABLE_UNIT_TESTS ON)

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	message("Using ccache!")
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED OFF)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.63 COMPONENTS system coroutine context thread REQUIRED)

include_directories(
${Boost_INCLUDE_DIRS}
${CMAKE_CURRENT_LIST_DIR}
)

link_directories(${Boost_LIBRARY_DIRS})

set(SRC_LIST
main.cpp
asio_tpool.hpp
)

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} )

if(ENABLE_UNIT_TESTS)
	add_subdirectory(tests)
endif()

