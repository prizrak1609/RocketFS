cmake_minimum_required(VERSION 3.29)
project(RemoteExecute)

add_subdirectory(WebSocketConnectionLib WebSocketConnectionLib)
add_subdirectory(CommandsLib CommandsLib)

add_subdirectory(Client Client)
add_subdirectory(Server Server)
