cmake_minimum_required(VERSION 3.16)

project(ClientSharedLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets)

add_library(ClientSharedLib SHARED
  ClientSharedLib_global.h
  clientsharedlib.cpp
  clientsharedlib.h
)

add_dependencies(ClientSharedLib WebSocketConnectionLib)

target_link_libraries(ClientSharedLib PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::WebSockets WebSocketConnectionLib)

target_compile_definitions(ClientSharedLib PRIVATE CLIENTSHAREDLIB_LIBRARY)
