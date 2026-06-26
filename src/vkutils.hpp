#include <vulkan/vulkan_raii.hpp>

namespace VkUtil {
    struct RenderingFrame {
        vk::raii::CommandBuffer commands = nullptr;
        vk::raii::Semaphore whenImageAccquired = nullptr;
        vk::raii::Fence whenRenderedToFrameJoin = nullptr;
    };

    auto waitFenceAndReset(
        const vk::raii::Device& device, 
        const vk::raii::Fence& fence,
        uint64_t timeout = UINT64_MAX
    ) -> void;
}