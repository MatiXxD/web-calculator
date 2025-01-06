cmake_minimum_required(VERSION 3.16)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_NAME "web-calculator")
set(INCLUDE_DIRS "include")
set(HTTP_SERVER_DIR "http-server")

project(${PROJECT_NAME})

add_subdirectory(${HTTP_SERVER_DIR})

include_directories(${INCLUDE_DIRS})
add_executable(${PROJECT_NAME}
  src/main.cpp
)

target_link_libraries(${PROJECT_NAME} 
  sub::http_server
)
