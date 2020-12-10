cmake_minimum_required(VERSION 3.11)

project(gsalo)

set(CMAKE_CXX_STANDARD 20)

set(DRY1_VK "" CACHE PATH "Vulkan SDK directory.")
set(DRY1_GLFW "" CACHE PATH "GLFW source directory.") # tmp move under conditional later
set(DRY1_GLM "" CACHE PATH "GLM directory.")
set(DRY1_STB "" CACHE PATH "STB directory.")
set(DRY1_TINYOBJLOADER "" CACHE PATH "Tinyobjloader directory.")

set(VK_SDK_HEADERS "${DRY1_VK}/Include")
set(GLFW_HEADERS "${DRY1_GLFW}/include")

add_subdirectory(${DRY1_GLFW} "${CMAKE_CURRENT_BINARY_DIR}/glfw_build")
add_compile_definitions(NOMINMAX) # win bullshit >:(

file(GLOB_RECURSE VKW_SOURCES
    "${CMAKE_SOURCE_DIR}/src/vkw/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/vkw/*.cpp")

# tmp this one too to condition
file(GLOB_RECURSE WINDOW_SOURCES
    "${CMAKE_SOURCE_DIR}/src/window/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/window/*.cpp")

file(GLOB_RECURSE GRAPHICS_SOURCES
    "${CMAKE_SOURCE_DIR}/src/graphics/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/graphics/*.cpp")

file(GLOB_RECURSE SHADER_HEADERS
    "${CMAKE_SOURCE_DIR}/proc/shader_headers/*.hpp")

add_executable(buildTest "${CMAKE_SOURCE_DIR}/src/main.cpp" ${VKW_SOURCES} ${WINDOW_SOURCES} ${GRAPHICS_SOURCES} ${SHADER_HEADERS})
execute_process(COMMAND cmd /c "${CMAKE_SOURCE_DIR}/shcomd.bat"
    "${CMAKE_SOURCE_DIR}/res/shaders"
    "${DRY1_VK}/Bin32/glslc.exe"
    "${CMAKE_SOURCE_DIR}/proc/shader_bin"
    "${CMAKE_SOURCE_DIR}/pcsr.exe" 
    "${CMAKE_SOURCE_DIR}/proc/shader_headers")

target_include_directories(buildTest PRIVATE
    ${VK_SDK_HEADERS}
    ${GLFW_HEADERS}
    ${DRY1_GLM}
    ${DRY1_STB}
    ${DRY1_TINYOBJLOADER}
    "${CMAKE_SOURCE_DIR}/proc")

target_link_libraries(buildTest PRIVATE
    "${GSALO_VULKAN_SDK}/Lib/vulkan-1.lib"
    glfw)