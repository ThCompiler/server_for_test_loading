cmake_minimum_required(VERSION 3.1X)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wpedantic -Werror -Wextra -ggdb3")

set(PROJECT_NAME http-proxy)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(SOURCE ${SOURCE_DIR}/main.cpp)
set(LIBRARIES_DIR ${CMAKE_SOURCE_DIR}/lib)
cmake_policy(SET CMP0079 NEW)

project(${PROJECT_NAME})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake-utilits")
include(templates)

###########
# Project #
###########

add_executable(${PROJECT_NAME} ${SOURCE})

add_subdirectory("lib/tcp_server_lib")
add_subdirectory("lib/file_client_lib")
add_subdirectory("lib/request_parser_lib")

target_link_libraries(file_client_lib tcp_server_lib request_parser_lib pthread)

target_link_libraries(${PROJECT_NAME} file_client_lib tcp_server_lib pthread)