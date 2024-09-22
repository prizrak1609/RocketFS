cmake_minimum_required(VERSION 3.16)

project(CommandsLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CommandsLib_INCLUDES ${CMAKE_CURRENT_LIST_DIR} CACHE FILEPATH "")

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(CommandsLib STATIC
  ICommand.h
  icommand.cpp

  command/getattrcmd.h
  command/getattrcmd.cpp
  command/readdircmd.h
  command/readdircmd.cpp
  command/rmdircmd.h
  command/rmdircmd.cpp
  command/mkdircmd.h
  command/mkdircmd.cpp
  command/renamecmd.h
  command/renamecmd.cpp
  command/createfilecmd.h
  command/createfilecmd.cpp
  command/rmfilecmd.h
  command/rmfilecmd.cpp
  command/openfilecmd.h
  command/openfilecmd.cpp
  command/readfilecmd.h
  command/readfilecmd.cpp
  command/writefilecmd.h
  command/writefilecmd.cpp
  command/closefilecmd.h
  command/closefilecmd.cpp
  command/statfscmd.h
  command/statfscmd.cpp
)

target_include_directories(CommandsLib PRIVATE "${CommandsLib_INCLUDES}")

target_link_libraries(CommandsLib PRIVATE Qt6::Core)

target_compile_definitions(CommandsLib PRIVATE COMMANDSLIB_LIBRARY)
