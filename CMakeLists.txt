cmake_minimum_required(VERSION 3.4)
project(shade CXX)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.60 COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/tangle/inc)

link_directories(${Boost_LIBRARY_DIRS})

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_BUILD 1)
set(PROJECT_VERSION_STABILITY "-alpha")
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(PROJECT_VERSION_FULL ${PROJECT_VERSION}${PROJECT_VERSION_STABILITY}+${PROJECT_VERSION_BUILD})

set(SRCS
	src/discovery.cc
	inc/shade/discovery.h
	inc/shade/tcp.h
	inc/shade/udp.h
)

set(ASIO_SRCS
	src/asio/network.cc
	inc/shade/asio/network.h
)

set(TANGLE_SRCS
	3rd_party/tangle/src/uri.cpp
	3rd_party/tangle/src/http_parser.cpp
	3rd_party/tangle/src/base_parser.cpp
	3rd_party/tangle/src/hasher.cpp

	3rd_party/tangle/inc/tangle/cstring.h
	3rd_party/tangle/inc/tangle/uri.h
	3rd_party/tangle/inc/tangle/msg/http_parser.h
	3rd_party/tangle/inc/tangle/msg/base_parser.h
	3rd_party/tangle/inc/tangle/msg/hasher.h
)

if (WIN32)
add_definitions(-D_WIN32_WINNT=0x0600)
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
endif()

add_library(shade STATIC ${SRCS})
add_library(shade-asio STATIC ${ASIO_SRCS})
add_library(shade-tangle STATIC ${TANGLE_SRCS})

add_executable(shade-test playground/main.cc)
foreach(DEP shade shade-asio shade-tangle)
	add_dependencies(shade-test ${DEP})
	target_link_libraries(shade-test $<TARGET_FILE:${DEP}>)
endforeach()
target_link_libraries(shade-test ${Boost_LIBRARIES})
