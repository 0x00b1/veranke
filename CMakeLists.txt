cmake_minimum_required(VERSION 3.2.2)

project(veranke)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/share/veranke/cmake)

add_definitions(-Wall -Wextra -std=c++0x -g)

include_directories(include)

add_executable(veranke src/main.cc)

find_package(SDL2 REQUIRED)

target_link_libraries(veranke ${SDL2_LIBRARY})
