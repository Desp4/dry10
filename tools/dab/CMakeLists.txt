cmake_minimum_required(VERSION 3.12)

project(dab)

set(CMAKE_CXX_STANDARD 20)

set(DAB_STB  "" CACHE PATH "STB directory")
set(DAB_ASSIMP  "" CACHE PATH "Assimp source directory")

add_subdirectory(${DAB_ASSIMP} "${CMAKE_CURRENT_BINARY_DIR}/assimp_build")

file(GLOB_RECURSE DAB_DAB_SOURCES
    "${PROJECT_SOURCE_DIR}/src/dab/*.cpp")

file(GLOB_RECURSE DAB_LIB_SOURCES
    "${PROJECT_SOURCE_DIR}/src/lib/*.cpp")

file(GLOB_RECURSE DAB_SHPB_SOURCES
    "${PROJECT_SOURCE_DIR}/src/shpb/*.cpp")

add_executable(dab ${DAB_DAB_SOURCES})
add_executable(shpb ${DAB_SHPB_SOURCES})
add_library(dablib ${DAB_LIB_SOURCES})

target_include_directories(dab PRIVATE "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/include" ${DAB_STB} "${DAB_ASSIMP}/include")
target_link_libraries(dab PRIVATE assimp)

target_include_directories(shpb PRIVATE "${PROJECT_SOURCE_DIR}/src")

target_include_directories(dablib PRIVATE ${DAB_STB} "${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}/include" "$ENV{VULKAN_SDK}/Include")
target_link_libraries(dablib PRIVATE "$ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib")