#include "vkutils.hpp"

auto VkUtil::waitFenceAndReset(
    const vk::raii::Device& device,
    const vk::raii::Fence& fence,
    uint64_t timeout
) -> void {
    auto result = device.waitForFences(*fence, true, timeout);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("failed to wait for fence!");
    device.resetFences(*fence);
}