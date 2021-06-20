#include "vk_initers.hpp"

#include <algorithm>

namespace dry {

vk_vertex_input select_vertex_input(std::vector<asset::vk_shader_data::vertex_binding_info> binding_infos, std::span<const vertex_input_setting> filter) {
    vk_vertex_input ret;

    std::sort(binding_infos.begin(), binding_infos.end(), [](auto& l, auto& r) { return l.location < r.location; });

    for (const auto& vert_setting : filter) {
        u32_t stride = 0;
        for (auto i = vert_setting.first_location; stride < vert_setting.binding_description.stride; ++i) {
            if (i >= binding_infos.size()) {
                LOG_ERR("Vertex input stride too big, reached end of vertex binding infos");
                dbg::panic();
            }

            VkVertexInputAttributeDescription attr;
            attr.binding = vert_setting.binding_description.binding;
            attr.format = binding_infos[i].format;
            attr.location = binding_infos[i].location;
            attr.offset = stride;

            stride += binding_infos[i].stride;
            ret.attribute_desc.push_back(attr);
        }

        if (stride != vert_setting.binding_description.stride) {
            LOG_ERR("Incopatible vertex input layout, stride too big");
            dbg::panic();
        }

        ret.binding_desc.push_back(vert_setting.binding_description);
    }

    return ret;
}

}