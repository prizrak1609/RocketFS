cmake_minimum_required(VERSION 3.16)
project(RemoteExecute)

add_definitions(${Qt6Core_DEFINITIONS})

add_subdirectory(CommandsLib CommandsLib)
add_subdirectory(WebSocketConnectionLib WebSocketConnectionLib)

add_subdirectory(InterceptLib InterceptLib)

add_subdirectory(Client Client)
add_subdirectory(Server Server)
