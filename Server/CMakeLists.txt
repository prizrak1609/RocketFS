cmake_minimum_required(VERSION 3.16)

project(Server LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(Server
  main.cpp

  server/server.h
  server/server.cpp
)

add_dependencies(Server WebSocketConnectionLib)

target_link_libraries(Server PRIVATE WebSocketConnectionLib Qt6::Core)
target_include_directories(Server PRIVATE "${WebSocketConnectionLib_INCLUDES}" "server")
