#ifdef WIN32
  #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include "renderer.hpp"

#include <algorithm>

#include "dbg/log.hpp"

#include "vkw/device/vk_functions.hpp"
#include "vk_initers.hpp"

namespace dry {

vulkan_renderer::vulkan_renderer(const wsi::window& window) {
    const auto& instance = vk_instance();
    _surface = vkw::vk_surface{ instance, window };
    
    const auto phys_device = find_physical_device();
    const auto queue_infos = populate_queue_infos(phys_device);
    _device = vkw::vk_device{ instance, phys_device, queue_infos.device_queue_infos, _device_extensions, _device_features };

    _present_queue = vkw::vk_queue{ _device, queue_infos.queue_init_infos[0].family_ind, queue_infos.queue_init_infos[0].queue_ind };
    _graphics_queue = vkw::vk_queue_graphics{ _device, queue_infos.queue_init_infos[1].family_ind, queue_infos.queue_init_infos[1].queue_ind };
    _transfer_queue = vkw::vk_queue_transfer{ _device, queue_infos.queue_init_infos[2].family_ind, queue_infos.queue_init_infos[2].queue_ind };

    const auto surface_capabilities = _device.surface_capabilities(_surface.handle());
    _image_count = surface_capabilities.maxImageCount == 0 ?
        surface_capabilities.minImageCount + 1 :
        (std::min)(surface_capabilities.minImageCount + 1, surface_capabilities.maxImageCount);
    _extent = surface_capabilities.currentExtent;

    _swapchain = vkw::vk_swapchain_present{
        _device, _surface.handle(),
        _extent, surface_capabilities.currentTransform, _image_count,
        _primary_image_format, _primary_image_colorspace, _primary_image_present_mode
    };
    _render_pass = vkw::vk_render_pass{
        _device, _extent,
        vkw::render_pass_flags::color | vkw::render_pass_flags::depth | vkw::render_pass_flags::msaa,
        _primary_msaa_sample_count, _primary_image_format, _primary_depth_format
    };
    _render_pass.create_framebuffers(_swapchain.swap_views());

    _instanced_pass = create_instanced_pass(_device, _image_count);
    
    _texarr = create_texture_array(_device, _graphics_queue, _image_count);

    _cmd_buffers.resize(_image_count);
    for (auto& cmd_buffer : _cmd_buffers) {
        cmd_buffer = vkw::vk_cmd_buffer{ _device, _present_queue.cmd_pool() };
    }

    _resources.cam_transform = &_default_cam_transform;
}

void vulkan_renderer::submit_frame() {
    // pre-sync operations, instance transform buffer
    {
        auto mapped_instances = _instanced_pass.instance_staging_buffer.map<instanced_pass::instance_input>();

        u32_t object_count = 0;
        for (const auto& pipeline : _resources.pipelines) {
            for (const auto& [mesh, renderables] : pipeline.renderables) {
                for (const auto& renderable : renderables) {
                    mapped_instances[object_count].transform = *renderable.transform_ptr;
                    mapped_instances[object_count].material = renderable.material;

                    object_count += 1;
                }
            }
        }

        _instanced_pass.instance_staging_buffer.unmap();
    }

    // acquire frame
    const auto frame_index = _swapchain.acquire_frame();
    const auto& cmd_buffer = _cmd_buffers[frame_index];
    const auto cmd_buffer_h = cmd_buffer.handle();

    vkResetCommandBuffer(cmd_buffer_h, 0);
    _render_pass.start_cmd_pass(cmd_buffer, frame_index);

    // === instanced pass ===

    // TODO : can be async, need interface
    _transfer_queue.copy_buffer(
        _instanced_pass.instance_staging_buffer.handle(), _instanced_pass.instance_buffers[frame_index].handle(),
        _instanced_pass.instance_staging_buffer.size()
    );

    _instanced_pass.camera_transforms[frame_index].write(std::span<const camera_transform>{ _resources.cam_transform, 1 });

    _texarr.update_descriptors(_device, frame_index);

    u32_t object_count = 0;
    constexpr std::array<VkDeviceSize, 1> offsets{ 0 };

    for (auto& pipeline : _resources.pipelines) {
        // check if material buffers are up to date, don't like it TODO :
        if (pipeline.pipeline_data.has_materials() && !pipeline.material_update_status[frame_index]) {
            auto* material_buffer = pipeline.pipeline_data.buffer_data<std::byte>(frame_index, pipeline_resources::material_ssbo_location);
            const auto material_stride = pipeline.pipeline_data.material_stride();

            for (const auto material : pipeline.material_inds) {
                _resources.materials[material]->write_material_info(material_buffer);
                material_buffer += material_stride;
            }

            pipeline.material_update_status[frame_index] = true;
        }
        
        pipeline.pipeline.bind_pipeline(cmd_buffer_h);

        // bind unshaded pass descriptors, NOTE : iirc if changing pipelines they stay bound, so redundant possibly?
        vkCmdBindDescriptorSets(cmd_buffer_h, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline.layout(),
            0, 1, &_instanced_pass.instance_descriptors[frame_index], 0, nullptr
        );
        // bind shared descriptors
        {
            auto& frame_shared_descs = pipeline.shared_descriptors[frame_index];
            if (frame_shared_descs.size() != 0) {
                vkCmdBindDescriptorSets(cmd_buffer_h, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline.layout(),
                    1, static_cast<u32_t>(frame_shared_descs.size()), frame_shared_descs.data(), 0, nullptr
                );
            }
        }

        pipeline.pipeline_data.bind_resources(frame_index, cmd_buffer_h, pipeline.pipeline.layout());

        for (const auto& [mesh, renderables] : pipeline.renderables) {
            const auto& vertex_buffer = _resources.vertex_buffers[mesh];
            const auto vertex_buffer_h = vertex_buffer.vertices.handle();

            vkCmdBindVertexBuffers(cmd_buffer_h, 0, 1, &vertex_buffer_h, offsets.data());
            vkCmdBindIndexBuffer(cmd_buffer_h, vertex_buffer.indices.handle(), 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(cmd_buffer_h, static_cast<u32_t>(vertex_buffer.indices.size() / sizeof(u32_t)),
                static_cast<u32_t>(renderables.size()), 0, 0, object_count
            );
            object_count += static_cast<u32_t>(renderables.size());
        }
    }

    vkCmdEndRenderPass(cmd_buffer_h);
    vkEndCommandBuffer(cmd_buffer_h);

    // TODO : wait for asyncs here

    _swapchain.submit_frame(_present_queue.handle(), frame_index, cmd_buffer_h);
}

VkPhysicalDevice vulkan_renderer::find_physical_device() {
    static constexpr VkQueueFlags device_queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;

    auto choose_device_lambda = [this](VkPhysicalDevice device) -> bool {
        return
            vkw::check_device_extension_support(device, _device_extensions) &&
            vkw::check_device_feature_support(device, _device_features) &&
            vkw::check_device_queue_support(device, device_queue_flags) &&
            vkw::check_swap_format_support(device, _surface.handle(), _primary_image_format, _primary_image_colorspace) &&
            vkw::check_swap_present_mode_support(device, _surface.handle(), _primary_image_present_mode);
    };
    const auto phys_devices = vk_instance().enumerate_physical_devices();
    const auto device_it = std::find_if(phys_devices.begin(), phys_devices.end(), choose_device_lambda);
    if (device_it == phys_devices.end()) {
        LOG_ERR("No suitable physical device found to initialize vulkan");
        dbg::panic();
    }
    return *device_it;
}

vulkan_renderer::populated_queue_info vulkan_renderer::populate_queue_infos(VkPhysicalDevice phys_device) {
    u32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, queue_families.data());

    using get_queue_ptr = u32_t(*)(std::span<const VkQueueFamilyProperties>);
    auto find_queue_index = [&queue_families](get_queue_ptr fun, VkQueueFlags flags) {
        u32_t ret_ind = fun(queue_families);
        if (ret_ind == (std::numeric_limits<u32_t>::max)()) {
            ret_ind = vkw::get_any_index(queue_families, flags);
        }
        return ret_ind;
    };

    // NOTE : here is the order, how they will be returned : present, graphics, transfer
    std::array<queue_family_info, 3> family_info_vals{};
    std::vector<queue_family_info*> family_infos{
            &family_info_vals[0], &family_info_vals[1], &family_info_vals[2]
    };
    // present is special, find without lambda
    family_infos[0]->family_ind = vkw::get_present_index(queue_families, phys_device, _surface.handle());
    if (family_infos[0]->family_ind == (std::numeric_limits<u32_t>::max)()) {
        LOG_ERR("Could not find present queue for initializing vulkan");
        dbg::panic();
    }

    family_infos[1]->family_ind = find_queue_index(vkw::get_separate_graphics_index, VK_QUEUE_GRAPHICS_BIT);
    family_infos[2]->family_ind = find_queue_index(vkw::get_separate_transfer_index, VK_QUEUE_TRANSFER_BIT);

    // create queue infos for every family index
    std::vector<vkw::queue_info> queue_infos;
    for (auto i = 0u; i < family_infos.size(); ++i) {
        vkw::queue_info queue_info{
            .queue_family_index = family_infos[i]->family_ind,
            .queue_count = 1
        };

        family_infos[i]->queue_ind = 0;
        // find fam infos with the same queue index, if found merge them and incrememnt queue count
        auto fam_it = family_infos.begin() + i + 1;
        while (true) {
            fam_it = std::find_if(fam_it, family_infos.end(),
                [queue_info](const auto& queue_fam) { return queue_fam->family_ind == queue_info.queue_family_index; }
            );

            if (fam_it == family_infos.end()) {
                break;
            }
            // found same family ind, assign queue index, increment queue count
            if (queue_info.queue_count >= queue_families[(*fam_it)->family_ind].queueCount) {
                LOG_WRN("Exceeded queue capacity of %i for family at index %i, using queue %i as fallback",
                    queue_info.queue_count, (*fam_it)->family_ind, queue_info.queue_count - 1
                );
                (*fam_it)->queue_ind = queue_info.queue_count - 1;
            }
            else {
                (*fam_it)->queue_ind = queue_info.queue_count;
                queue_info.queue_count += 1;
            }
            // erase, by the end all family inds with same value will be merged into 1
            fam_it = family_infos.erase(fam_it);
        }
        // push queue info
        queue_info.priorities.resize(queue_info.queue_count, 1.0f); // NOTE : default to 1
        queue_infos.push_back(std::move(queue_info));
    }
    return { std::move(family_info_vals), std::move(queue_infos) };
}

const vkw::vk_instance& vulkan_renderer::vk_instance() {
#ifdef VKW_ENABLE_DEBUG
    static constexpr std::array<const char*, 3> instance_extensions{
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME // win only TODO :
    };
    static constexpr std::array<const char*, 1> validation_layers{
        "VK_LAYER_KHRONOS_validation"
    };

    static const vkw::vk_instance main_instance{ instance_extensions, "dry1 instance", validation_layers, vk_debug_callback };
#else
    static constexpr std::array<const char*, 2> instance_extensions{
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME // win only TODO :
    };
    static const vkw::vk_instance main_instance{ instance_extensions, "dry1 instance" };
#endif
    return main_instance;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_renderer::vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data)
{
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        LOG_ERR("\033[32mVALIDATION LAYER\033[0m: %s", data->pMessage);
    }
    return VK_FALSE;
}

}