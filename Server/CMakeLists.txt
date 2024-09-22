cmake_minimum_required(VERSION 3.14)

project(Server LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core WebSockets)

add_executable(Server
  main.cpp

  server/server.h
  server/server.cpp
)

target_link_libraries(Server Qt6::Core Qt6::WebSockets)
target_include_directories(Server PRIVATE "server")
