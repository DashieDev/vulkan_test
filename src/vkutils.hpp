#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <cstdint>

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

    class SwapChain {
    public:
        vk::raii::SwapchainKHR swapChain = nullptr;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::Extent2D extent;
        std::vector<vk::Image> images;
        std::vector<vk::raii::ImageView> imageViews;

        SwapChain() = delete;
        SwapChain(std::nullptr_t nptr) {};
    };
}