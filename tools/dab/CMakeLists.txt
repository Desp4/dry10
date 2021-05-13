cmake_minimum_required(VERSION 3.12)

project(dab)
set(CMAKE_CXX_STANDARD 20)

set(DAB_SOURCES "${PROJECT_SOURCE_DIR}/main.cpp")

add_executable(dab ${DAB_SOURCES})

target_link_libraries(dab PUBLIC zip assimp stb shaderc)