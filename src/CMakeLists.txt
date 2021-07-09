cmake_minimum_required(VERSION 3.12)

project(dry1)
set(CMAKE_CXX_STANDARD 20)

set(DEP_SOURCES dep_tus/vma_impl.cpp)

set(VKW_SOURCES
    vkw/buffer.cpp
    vkw/framebuffer.cpp
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
    vkw/queue/queue_fun.cpp
    vkw/shader/shader.cpp)

set(WINDOW_SOURCES
    window/window.cpp)

set(GRAPHICS_SOURCES
    graphics/renderer.cpp
    graphics/instanced_pass.cpp
    graphics/renderer_creates.cpp
    graphics/vk_initers.cpp
    graphics/texarr.cpp
    graphics/pipeline_resources.cpp)

set(ASSET_SOURCES
    asset/filesys.cpp
    asset/shader_reflect.cpp)

set(ENGINE_SOURCES engine/dry_program.cpp)

set(UTIL_SOURCES
    util/fs.cpp)

set (MATH_SOURCES math/geometry.cpp)

add_library(dry1 STATIC
    ${DEP_SOURCES}
    ${VKW_SOURCES} 
    ${WINDOW_SOURCES}
    ${GRAPHICS_SOURCES}
    ${ASSET_SOURCES}
    ${ENGINE_SOURCES}
    ${UTIL_SOURCES}
    ${MATH_SOURCES})

target_include_directories(dry1 PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(dry1 PUBLIC
    vulkan
    glfw
    glm
    zip
    stbi
    spirv-cross-core
    vma)