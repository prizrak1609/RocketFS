cmake_minimum_required(VERSION 3.16)

project(ClientSharedLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core WebSockets)

add_library(ClientSharedLib SHARED
  ClientSharedLib_global.h
  clientsharedlib.cpp
  clientsharedlib.h
)

target_include_directories(ClientSharedLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(ClientSharedLib WebSocketConnectionLib)

target_link_directories(ClientSharedLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ClientSharedLib PRIVATE Qt6::Core Qt6::WebSockets WebSocketConnectionLib)

target_compile_definitions(ClientSharedLib PRIVATE CLIENTSHAREDLIB_LIBRARY)
