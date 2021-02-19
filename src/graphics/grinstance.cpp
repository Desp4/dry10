#include "grinstance.hpp"
#include "vkw/device/instance.hpp"
#include "vkw/device/device.hpp"
#include "vkw/device/defconf.hpp"
#include "dbg/log.hpp"

namespace dry::gr {

VKAPI_ATTR VkBool32 VKAPI_CALL graphics_instance::debug_callback(
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

graphics_instance::graphics_instance(wsi::native_handle window) {
#ifdef VKW_ENABLE_VAL_LAYERS
    vkw::instance_main::create(EXTENSIONS, VAL_LAYERS, debug_callback);
#else
    vkw::instance_main::create(EXTENSIONS, {}, nullptr);
#endif

    _surface = vkw::surface(window);
    const auto phys_devices = vkw::instance_main::enumerate_physical_devices();
    VkPhysicalDevice phys_device = VK_NULL_HANDLE;
    // NOTE : requested settings or bust, can write a few lines to have fallback options
    for (const auto device : phys_devices) {
        if (vkw::conf::check_device_extension_support(device, DEV_EXTENSIONS) &&
            vkw::conf::check_device_feature_support(device, FEATURES) &&
            vkw::conf::check_device_queue_support(device, QUEUES) &&
            vkw::conf::check_swap_format_support(device, _surface.vk_surface(), IMAGE_FORMAT, IMAGE_COLORSPACE) &&
            vkw::conf::check_swap_present_mode_support(device, _surface.vk_surface(), IMAGE_PRESENT_MODE))
        {
            phys_device = device;
            break;
        }
    }

    PANIC_ASSERT(phys_device != VK_NULL_HANDLE, "no suitable device found");
    // further down just figuring out queue indices up until the very end

    // NOTE : graphics needs a compute or sparse binding for transition img layout, idk which one
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(phys_device, &queue_family_count, queue_families.data());

    // present, graphics, transfer
    int32_t tmp_queue_index = -1;
    std::vector<queue_ind_info*> family_inds(3);

    // get all queues, prefer dedicated, if not pick the first that satisfies the usage
    tmp_queue_index = vkw::conf::get_present_index(queue_families, phys_device, _surface.vk_surface());
    PANIC_ASSERT(tmp_queue_index != -1, "could not find present queue");
    family_inds[0] = &_present_queue;
    family_inds[0]->family_ind = tmp_queue_index;

    tmp_queue_index = vkw::conf::get_separate_graphics_index(queue_families);
    if (tmp_queue_index == -1) {
        tmp_queue_index = vkw::conf::get_any_index(queue_families, VK_QUEUE_GRAPHICS_BIT);
    }
    family_inds[1] = &_graphics_worker_queue;
    family_inds[1]->family_ind = tmp_queue_index;

    tmp_queue_index = vkw::conf::get_separate_transfer_index(queue_families);
    if (tmp_queue_index == -1) {
        tmp_queue_index = vkw::conf::get_any_index(queue_families, VK_QUEUE_TRANSFER_BIT);
    }
    family_inds[2] = &_transfer_worker_queue;
    family_inds[2]->family_ind = tmp_queue_index;

    // what it does: create queueInfo for each separate family, if multiple of one - count them for submission
    std::vector<vkw::device_main::queue_info> queue_infos;
    for (int i = 0; i < family_inds.size(); ++i) {
        vkw::device_main::queue_info queue_info;
        queue_info.queue_family_index = family_inds[i]->family_ind;
        queue_info.queue_count = 1;

        family_inds[i]->queue_ind = 0;
        for (auto p = family_inds.begin() + i + 1; p != family_inds.end();) {
            if (family_inds[i]->family_ind == (*p)->family_ind) {
                if (queue_info.queue_count == queue_families[(*p)->family_ind].queueCount) {
                    LOG_WRN("exceeded queue capacity of %i for family at index %i, using queue %i as fallback",
                        queue_info.queue_count, (*p)->family_ind, queue_info.queue_count - 1
                    );
                    (*p)->queue_ind = queue_info.queue_count - 1;
                }
                else {
                    (*p)->queue_ind = queue_info.queue_count;
                    queue_info.queue_count += 1;
                }
                p = family_inds.erase(p);
            }
            else {
                p += 1;
            }
        }
        queue_info.priorities.resize(queue_info.queue_count, 1.0f); // NOTE : all priorities to 1
        queue_infos.push_back(std::move(queue_info));
    }

    vkw::device_main::create(phys_device, queue_infos, DEV_EXTENSIONS, FEATURES);
}

graphics_instance::~graphics_instance() {
    vkw::device_main::destroy();
    _surface = vkw::surface{}; // HACK to get around singleton ugliness
    vkw::instance_main::destroy();
}

}