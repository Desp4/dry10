#pragma once

#ifndef DRY_VKW_QUEUE_FUN_H
#define DRY_VKW_QUEUE_FUN_H

#include "queue.hpp"
#include "vkw/buffer.hpp"
#include "vkw/image/image.hpp"

namespace dry::vkw {

template<auto Fun, typename... Args>
auto execute_cmd_once(const vk_queue& queue, Args&&... args);

// transfer
template<typename T>
vk_buffer create_local_buffer(const vk_queue& queue, const vk_device& device, std::span<const T> values, VkBufferUsageFlags usage);
void copy_buffer(const vk_cmd_buffer& cmd, VkBuffer src, VkBuffer dst, VkDeviceSize size);
void copy_buffer_to_image(const vk_cmd_buffer& cmd, VkBuffer buffer, const vk_image& image);
// graphics
void transition_image_layout(const vk_cmd_buffer& cmd, const vk_image& image, VkImageLayout layout_old, VkImageLayout layout_new);
void generate_mip_maps(const vk_cmd_buffer& cmd, const vk_image& image);




template<auto Fun, typename... Args>
auto execute_cmd_once(const vk_queue& queue, Args&&... args) {
    using return_t = std::invoke_result_t<decltype(Fun), vk_cmd_buffer, decltype(std::forward<Args>(args))...>;

    const auto cmd = queue.create_buffer();
    cmd.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    if constexpr (std::is_void_v<return_t>) {
        Fun(cmd, std::forward<Args>(args)...);
        queue.submit(cmd);
        queue.collect();
        return;
    } else {
        auto ret = Fun(cmd, std::forward<Args>(args)...);
        queue.submit(cmd);
        queue.collect();
        return ret;
    }  
}

template<typename T>
vk_buffer create_local_buffer(const vk_queue& queue, const vk_device& device, std::span<const T> values, VkBufferUsageFlags usage) {
    vk_buffer staging_buffer{ device, values.size_bytes(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    };
    staging_buffer.write(values);

    vk_buffer ret_buf{ device, values.size_bytes(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VMA_MEMORY_USAGE_GPU_ONLY
    };
    execute_cmd_once<copy_buffer>(queue, staging_buffer.handle(), ret_buf.handle(), values.size_bytes());
    return ret_buf;
}

}

#endif