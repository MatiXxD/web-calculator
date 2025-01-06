cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(HTTP_SERVER "http_server")

project(${HTTP_SERVER})

add_library(${PROJECT_NAME} STATIC 
  ${PROJECT_SOURCE_DIR}/src/http_server.cpp
)
add_library(sub::http_server ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)
