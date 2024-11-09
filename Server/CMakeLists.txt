cmake_minimum_required(VERSION 3.16)

project(Server LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(Server
  main.cpp

  server.h
  server.cpp

  pathhelper.h
  pathhelper.cpp
)

add_dependencies(Server WebSocketConnectionLib)

target_link_libraries(Server PRIVATE WebSocketConnectionLib Qt6::Core)
target_include_directories(Server PRIVATE "${WebSocketConnectionLib_INCLUDES}" "server")
