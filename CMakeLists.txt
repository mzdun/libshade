cmake_minimum_required(VERSION 3.4)
project(shade C CXX)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.60 COMPONENTS system)
find_package(Threads)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/tangle/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/json/inc)

link_directories(${Boost_LIBRARY_DIRS})

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 5)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION_BUILD 1)
set(PROJECT_VERSION_STABILITY "-alpha")
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(PROJECT_VERSION_FULL ${PROJECT_VERSION}${PROJECT_VERSION_STABILITY}+${PROJECT_VERSION_BUILD})

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/inc/shade/version.in.h ${CMAKE_CURRENT_BINARY_DIR}/inc/shade/version.h)

set(SRCS
	src/cache.cc
	src/manager.cc
	src/heartbeat.cc
	src/discovery.cc
	src/storage.cc
)

set(INCS
	inc/shade/cache.h
	inc/shade/manager.h
	inc/shade/heartbeat.h
	inc/shade/discovery.h
	inc/shade/storage.h
	"${CMAKE_CURRENT_BINARY_DIR}/inc/shade/version.h"
	inc/shade/version.in.h
)

if (UNIX)
list(APPEND SRCS
	src/manager_linux.cc
	src/storage_linux.cc
)
elseif(WIN32)
list(APPEND SRCS
	src/manager_win.cc
	src/storage_win.cc
)
endif()
source_group("src" FILES ${SRCS})
source_group("inc" FILES ${INCS})

set(SRCS_IO
	src/io/connection.cc
)

set(INCS_IO
	inc/shade/io/connection.h
	inc/shade/io/network.h
	inc/shade/io/http.h
)

source_group("src\\io" FILES ${SRCS_IO})
source_group("inc\\io" FILES ${INCS_IO})

set(SRCS_MODEL
	src/model/bridge.cc
	src/model/light_source.cc
	src/model/light.cc
	src/model/group.cc
	src/model/host.cc
	src/model/json.h
)

set(INCS_MODEL
	inc/shade/model/light_source.h
	inc/shade/model/light.h
	inc/shade/model/group.h
	inc/shade/model/bridge.h
	inc/shade/model/host.h
)

source_group("src\\model" FILES ${SRCS_MODEL})
source_group("inc\\model" FILES ${INCS_MODEL})

set(ASIO_SRCS
	src/asio/network.cc
	src/asio/http.cc
	inc/shade/asio/network.h
	inc/shade/asio/http.h
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

set(JSON_SRCS
	3rd_party/json/src/json.cpp
	3rd_party/json/src/utf8.cpp
	3rd_party/json/inc/json.hpp
	3rd_party/json/inc/json/json.hpp
	3rd_party/json/inc/json/serdes.hpp
	3rd_party/json/inc/utf8.hpp
)

set(CLI_SRCS
	cli/main.cc
	cli/client.cc
	cli/menu.cc
	cli/client.h
	cli/menu.h
)

if (WIN32)
add_definitions(-D_WIN32_WINNT=0x0600)
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
else()
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
endif()

add_library(shade STATIC
	${SRCS}
	${INCS}
	${SRCS_IO}
	${INCS_IO}
	${SRCS_MODEL}
	${INCS_MODEL})
add_library(shade-asio STATIC ${ASIO_SRCS})
add_library(shade-tangle STATIC ${TANGLE_SRCS})
add_library(shade-json STATIC ${JSON_SRCS})

add_executable(shade-cli ${CLI_SRCS})
foreach(DEP shade shade-asio shade-tangle shade-json)
	add_dependencies(shade-cli ${DEP})
	target_link_libraries(shade-cli $<TARGET_FILE:${DEP}>)
endforeach()
target_link_libraries(shade-cli ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
