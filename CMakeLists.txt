cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(RemoteExecute)

add_subdirectory(CommandsLib CommandsLib)
add_subdirectory(WebSocketConnectionLib WebSocketConnectionLib)

add_subdirectory(Client Client)
add_subdirectory(Server Server)
