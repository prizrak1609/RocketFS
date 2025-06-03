cmake_minimum_required(VERSION 3.16)

project(InterceptLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(InterceptLib_INCLUDES ${CMAKE_CURRENT_LIST_DIR} CACHE FILEPATH "")

find_package(Qt6 REQUIRED COMPONENTS Core)

set(HEADERS
  filesystemintercept.h
    )

add_library(InterceptLib SHARED
  filesystemintercept.cpp
  ${HEADERS}
)

target_include_directories(InterceptLib PRIVATE "${InterceptLib_INCLUDES}")

target_link_libraries(InterceptLib PRIVATE Qt6::Core)

target_compile_definitions(InterceptLib PRIVATE INTERCEPTLIB_LIBRARY)
