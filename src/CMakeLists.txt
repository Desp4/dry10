cmake_minimum_required(VERSION 3.12)

project(dry1)
set(CMAKE_CXX_STANDARD 20)

set(VKW_SOURCES
    vkw/buffer.cpp
    vkw/framebuffer.cpp
    vkw/memory.cpp
    vkw/pipeline_g.cpp
    vkw/renderpass.cpp
    vkw/swapchain_p.cpp
    vkw/texsampler.cpp
    vkw/cmd/cmdbuffer.cpp
    vkw/cmd/cmdpool.cpp
    vkw/desc/desclayout.cpp
    vkw/desc/descpool.cpp
    vkw/desc/desc_superpool.cpp
    vkw/device/vk_functions.cpp
    vkw/device/device.cpp
    vkw/device/instance.cpp
    vkw/device/surface.cpp
    vkw/image/image.cpp
    vkw/image/imageview.cpp
    vkw/queue/queue.cpp
    vkw/queue/queue_graphics.cpp
    vkw/queue/queue_transfer.cpp
    vkw/shader/shader.cpp)

set(WINDOW_SOURCES
    window/window.cpp)

set(GRAPHICS_SOURCES
    graphics/renderer.cpp
    graphics/renderer_resource_registry.cpp)

set(ASSET_SOURCES
    asset/filesys.cpp
    asset/asset_resource_adapter.cpp
    asset/shader_reflect.cpp)

set(UTIL_SOURCES
    util/fs.cpp)

add_library(dry1 STATIC
    ${VKW_SOURCES} 
    ${WINDOW_SOURCES}
    ${GRAPHICS_SOURCES}
    ${ASSET_SOURCES}
    ${UTIL_SOURCES})

target_include_directories(dry1 PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(dry1 PUBLIC
    vulkan
    glfw
    glm
    zip
    stb
    spirv-cross-core)