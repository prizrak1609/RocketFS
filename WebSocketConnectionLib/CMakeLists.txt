cmake_minimum_required(VERSION 3.16)

project(WebSocketConnectionLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WebSocketConnectionLib_INCLUDES ${CMAKE_CURRENT_LIST_DIR} CACHE FILEPATH "")

find_package(Qt6 REQUIRED COMPONENTS Core WebSockets)

add_library(WebSocketConnectionLib STATIC
  connection.h
  connection.cpp
  connection_pool.h
  connection_pool.cpp
  ICommand.h
)

target_link_libraries(WebSocketConnectionLib PUBLIC Qt6::Core Qt6::WebSockets)

target_compile_definitions(WebSocketConnectionLib PRIVATE WEBSOCKETCONNECTIONLIB_LIBRARY)
