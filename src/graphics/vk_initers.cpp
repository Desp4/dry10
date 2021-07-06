#include "vk_initers.hpp"

#include <algorithm>

#include "vkw/queue/queue_fun.hpp"

namespace dry {

vk_vertex_input vertex_bindings_to_input(std::vector<asset::vk_shader_data::vertex_binding_info> bindings) {
    vk_vertex_input ret;
    ret.attribute_desc.reserve(bindings.size());

    std::sort(bindings.begin(), bindings.end(), [](auto& l, auto& r) { return l.location < r.location; });

    ret.binding_desc.binding = 0;
    ret.binding_desc.stride = 0;
    ret.binding_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    for (const auto& binding : bindings) {
        VkVertexInputAttributeDescription attr;
        attr.binding = ret.binding_desc.binding;
        attr.format = binding.format;
        attr.location = binding.location;
        attr.offset = ret.binding_desc.stride;

        ret.attribute_desc.push_back(attr);

        ret.binding_desc.stride += binding.stride;
    }

    return ret;
}

vkw::vk_image_view_pair create_sampled_texture(const vkw::vk_device& device,
    const vkw::vk_queue& g_queue, const vkw::vk_queue& t_queue, const_byte_span data,
    VkExtent2D dimensions, u32_t mip_lvls, VkFormat img_format)
{
    vkw::vk_buffer staging_buffer{ device,
        data.size_bytes(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY
    };
    staging_buffer.write(data);

    vkw::vk_image_view_pair texture{ device,
        dimensions,
        mip_lvls,
        VK_SAMPLE_COUNT_1_BIT,
        img_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_COLOR_BIT
    };

    // execute
    vkw::execute_cmd_once<vkw::transition_image_layout>(g_queue, texture.image(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );
    vkw::execute_cmd_once<vkw::copy_buffer_to_image>(t_queue, staging_buffer.handle(), texture.image());
    vkw::execute_cmd_once<vkw::generate_mip_maps>(g_queue, texture.image());

    return texture;
}

}