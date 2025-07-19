cmake_minimum_required(VERSION 3.16)

find_program(MAKE_EXE NAMES gmake nmake make)
include(ExternalProject)
ExternalProject_Add(
  Detours
  GIT_REPOSITORY    https://github.com/microsoft/Detours.git
  GIT_TAG           v4.0.1
  PREFIX            "third_party/Detours"
  BUILD_COMMAND     ${MAKE_EXE} DETOURS_TARGET_PROCESSOR="X64" DETOURS_CONFIG="Release" all
)

project(InterceptLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(InterceptLib_INCLUDES ${CMAKE_CURRENT_LIST_DIR} CACHE FILEPATH "")

find_package(Qt6 REQUIRED COMPONENTS Core)

file(GLOB Detour_SRC "third_party/Detours/src/.*\.cpp")
list(FILTER Detour_SRC EXCLUDE REGEX ".*x86\.cpp")
list(FILTER Detour_SRC EXCLUDE REGEX ".*ia64\.cpp")
list(FILTER Detour_SRC EXCLUDE REGEX ".*arm64\.cpp")
list(FILTER Detour_SRC EXCLUDE REGEX ".*arm\.cpp")

set(HEADERS
  filesystemintercept.h
    )

add_library(InterceptLib SHARED
  filesystemintercept.cpp
  ${HEADERS}
  ${Detour_SRC}
)

add_dependencies(InterceptLib Detours)

target_include_directories(InterceptLib PRIVATE "${InterceptLib_INCLUDES}" "third_party/Detours/src")

target_link_libraries(InterceptLib PRIVATE Qt6::Core)

target_compile_definitions(InterceptLib PRIVATE INTERCEPTLIB_LIBRARY)
