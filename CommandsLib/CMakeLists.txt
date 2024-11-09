cmake_minimum_required(VERSION 3.16)

project(CommandsLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CommandsLib_INCLUDES ${CMAKE_CURRENT_LIST_DIR} CACHE FILEPATH "")

find_package(Qt6 REQUIRED COMPONENTS Core)

set(HEADERS command/getattrcmd.h
    command/readdircmd.h
    command/rmdircmd.h
    command/mkdircmd.h
    command/renamecmd.h
    command/createfilecmd.h
    command/rmfilecmd.h
    command/openfilecmd.h
    command/readfilecmd.h
    command/writefilecmd.h
    command/closefilecmd.h
    command/statfscmd.h
    )

add_library(CommandsLib STATIC
  ICommand.h
  icommand.cpp

  command/getattrcmd.cpp
  command/readdircmd.cpp
  command/rmdircmd.cpp
  command/mkdircmd.cpp
  command/renamecmd.cpp
  command/createfilecmd.cpp
  command/rmfilecmd.cpp
  command/openfilecmd.cpp
  command/readfilecmd.cpp
  command/writefilecmd.cpp
  command/closefilecmd.cpp
  command/statfscmd.cpp

  ${HEADERS}
)


target_include_directories(CommandsLib PRIVATE "${CommandsLib_INCLUDES}")

target_link_libraries(CommandsLib PRIVATE Qt6::Core)

target_compile_definitions(CommandsLib PRIVATE COMMANDSLIB_LIBRARY)
