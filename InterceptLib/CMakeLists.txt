cmake_minimum_required(VERSION 3.16)

project(InterceptLib LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(InterceptLib_INCLUDES ${CMAKE_CURRENT_LIST_DIR} CACHE FILEPATH "")

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

add_library(InterceptLib SHARED
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

target_include_directories(InterceptLib PRIVATE "${InterceptLib_INCLUDES}")

target_link_libraries(InterceptLib PRIVATE Qt6::Core)

target_compile_definitions(InterceptLib PRIVATE INTERCEPTLIB_LIBRARY)
