#pragma once

#ifdef _DEBUG
  #ifndef VKW_ENABLE_VAL_LAYERS
    #define VKW_ENABLE_VAL_LAYERS
  #endif
#endif

#ifdef _WIN32
 #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <array>

#include "vkw/device/surface.hpp"
#include "vkw/queue/queue_graphics.hpp"
#include "vkw/queue/queue_transfer.hpp"

namespace dry::gr {

struct queue_ind_info {
    uint32_t family_ind;
    uint32_t queue_ind;
};

class graphics_instance {
public:
    constexpr static VkFormat         IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
    constexpr static VkColorSpaceKHR  IMAGE_COLORSPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    constexpr static VkPresentModeKHR IMAGE_PRESENT_MODE = VK_PRESENT_MODE_MAILBOX_KHR;

    // NOTE : forcing default config, can't support fully custom features yet
    graphics_instance(wsi::native_handle window);
    ~graphics_instance();

    const vkw::surface& surface() const {
        return _surface;
    }
    const queue_ind_info& present_queue() const {
        return _present_queue;
    }
    const queue_ind_info& graphics_queue() const {
        return _graphics_worker_queue;
    }
    const queue_ind_info& transfer_queue() const {
        return _transfer_worker_queue;
    }

private:
    constexpr static VkQueueFlags QUEUES = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT;
    constexpr static std::array<const char*, 3> EXTENSIONS{
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    constexpr static std::array<const char*, 1> VAL_LAYERS{
        "VK_LAYER_KHRONOS_validation"
    };
    constexpr static std::array<const char*, 1> DEV_EXTENSIONS{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    constexpr static VkPhysicalDeviceFeatures   FEATURES{
        .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* user_data);

    vkw::instance _instance;
    vkw::surface _surface;

    // family and index
    queue_ind_info _present_queue;
    queue_ind_info _graphics_worker_queue;
    queue_ind_info _transfer_worker_queue;
};

}