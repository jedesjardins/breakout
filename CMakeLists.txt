cmake_minimum_required(VERSION 3.12)
project(breakout)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
	Gfx
	message(STATUS "Getting the gfx library")
	GIT_REPOSITORY "https://github.com/jedesjardins/gfx"
	GIT_TAG v0.1.1
)

FetchContent_GetProperties(Gfx)
if(NOT Gfx_POPULATED)
	FetchContent_Populate(Gfx)
	add_subdirectory(${gfx_SOURCE_DIR} ${gfx_BINARY_DIR})
endif()

FetchContent_Declare(
	Log
	message(STATUS "Getting the log library")
	GIT_REPOSITORY https://github.com/jedesjardins/log.git
)

FetchContent_GetProperties(Log)
if(NOT Log_POPULATED)
	FetchContent_Populate(Log)
	add_subdirectory(${log_SOURCE_DIR} ${log_BINARY_DIR})
endif()

list(APPEND SOURCES
	src/main.cpp
)

add_executable(breakout ${SOURCES})

target_link_libraries(breakout JED::gfx JED::log)

