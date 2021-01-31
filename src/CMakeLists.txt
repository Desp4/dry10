cmake_minimum_required(VERSION 3.12)

project(dry1lib)

set(CMAKE_CXX_STANDARD 20)

set(DRY1_GLFW "" CACHE PATH "GLFW source directory")
set(DRY1_GLM  "" CACHE PATH "GLM directory")

set(VK_INC "$ENV{VULKAN_SDK}/Include")
set(VK_LIB "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")

add_subdirectory(${DRY1_GLFW} "${CMAKE_CURRENT_BINARY_DIR}/glfw_build")

file(GLOB_RECURSE VKW_SOURCES
    "${PROJECT_SOURCE_DIR}/vkw/*.hpp"
    "${PROJECT_SOURCE_DIR}/vkw/*.cpp")

# tmp this one too
file(GLOB_RECURSE WINDOW_SOURCES
    "${PROJECT_SOURCE_DIR}/window/*.hpp"
    "${PROJECT_SOURCE_DIR}/window/*.cpp")

file(GLOB_RECURSE GRAPHICS_SOURCES
    "${PROJECT_SOURCE_DIR}/graphics/*.hpp"
    "${PROJECT_SOURCE_DIR}/graphics/*.cpp")

file(GLOB_RECURSE ASSET_SOURCES
    "${PROJECT_SOURCE_DIR}/asset/*.hpp"
    "${PROJECT_SOURCE_DIR}/asset/*.cpp")

file(GLOB_RECURSE DEBUG_SOURCES
    "${PROJECT_SOURCE_DIR}/dbg/*.hpp"
    "${PROJECT_SOURCE_DIR}/dbg/*.cpp")

file(GLOB_RECURSE UTIL_SOURCES
    "${PROJECT_SOURCE_DIR}/util/*.hpp"
    "${PROJECT_SOURCE_DIR}/util/*.cpp")

add_library(dry1 STATIC
    ${VKW_SOURCES}
    ${WINDOW_SOURCES}
    ${GRAPHICS_SOURCES}
    ${ASSET_SOURCES}
    ${DEBUG_SOURCES}
    ${UTIL_SOURCES})

target_include_directories(dry1 PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${DRY1_SHPB_INC}
    ${VK_INC}
    ${GLFW_HEADERS}
    ${DRY1_GLM})

target_link_libraries(dry1 PUBLIC
    ${VK_LIB}
    glfw
    dablib)