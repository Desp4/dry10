cmake_minimum_required(VERSION 3.12)

project(dry1)

set(CMAKE_CXX_STANDARD 20)

set(DRY1_PCSR "" CACHE PATH "PCSR executable path") # still not a fan
set(DRY1_PCSR_INC "" CACHE PATH "PCSR include directory")
set(DRY1_GLFW "" CACHE PATH "GLFW source directory")
set(DRY1_GLM  "" CACHE PATH "GLM directory")
set(DRY1_STB  "" CACHE PATH "STB directory")
set(DRY1_TINYOBJLOADER "" CACHE PATH "Tinyobjloader directory")

if(WIN32)
    set(VK_INC "$ENV{VULKAN_SDK}/Include")
    set(VK_LIB "$ENV{VULKAN_SDK}/Lib/vulkan-1.lib")
    set(SHCOMD "cmd" "/c" "${CMAKE_SOURCE_DIR}/shcomd.bat")

    add_compile_definitions(NOMINMAX) # or just clean the project
else()
    set(VK_INC "")
    set(VK_LIB "-lvulkan")
    set(SHCOMD "bash" "${CMAKE_SOURCE_DIR}/shcomd.sh")
endif()

add_subdirectory(${DRY1_GLFW} "${CMAKE_CURRENT_BINARY_DIR}/glfw_build")

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

add_executable(buildTest
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    ${VKW_SOURCES}
    ${WINDOW_SOURCES}
    ${GRAPHICS_SOURCES}
    ${SHADER_HEADERS})

# create pcsr dirs
set(SH_BIN "${CMAKE_SOURCE_DIR}/proc/shader_bin")
set(SH_INC "${CMAKE_SOURCE_DIR}/proc/shader_headers")
file(MAKE_DIRECTORY ${SH_BIN} ${SH_INC})
# hack to keep things working for now
file(COPY "${DRY1_PCSR_INC}/pcsrtype.hpp" DESTINATION "${SH_INC}/pcsrtype.hpp")

# run pcsr
execute_process(COMMAND ${SHCOMD}
    "${CMAKE_SOURCE_DIR}/res/shaders"
    ${SH_BIN}
    ${DRY1_PCSR}
    ${SH_INC})

target_include_directories(drydemo PRIVATE
    ${VK_INC}
    ${GLFW_HEADERS}
    ${DRY1_GLM}
    ${DRY1_STB}
    ${DRY1_TINYOBJLOADER}
    "${CMAKE_SOURCE_DIR}/proc")

target_link_libraries(drydemo PRIVATE
    ${VK_LIB}
    glfw)
