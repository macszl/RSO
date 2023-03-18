cmake_minimum_required(VERSION 3.24)
project(RSO_1 C)

set(CMAKE_C_STANDARD 11)

add_executable(CLIENT client/client.c client/client.h shared/protocol.h)
add_executable(SERVER server/server.c server/server.h shared/protocol.h)
