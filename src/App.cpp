#include "App.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include "utils.hpp"
#include <ranges>

const std::vector<std::string_view> APP_VALIDATIONS_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATIONS_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATIONS_LAYERS = true;
#endif

void App::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->window = glfwCreateWindow(WINDOW_W, WINDOW_H, "Vulkan Test", NULL, NULL);
}

void App::initVulkan() {
    this->vulkanContext.init();
}

void App::mainLoop() {
    while (!glfwWindowShouldClose(this->window)) {
        processWindowInput();
        glfwPollEvents();
    }
}

void App::processWindowInput() {
    if(glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(this->window, true);
}

void App::cleanup() {
    glfwDestroyWindow(window);

    glfwTerminate();
}

void VulkanContext::initInstance() {
    auto appInfo = vk::ApplicationInfo()
        .setPApplicationName("Rotating Cubes")
        .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
        .setPEngineName("No Engine")
        .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
        .setApiVersion(vk::ApiVersion14);

    auto glfw_required_extensions = GLFWUtil::getRequiredInstanceExtensions();
    
    auto provided_extensions = this->vkContext.enumerateInstanceExtensionProperties();
    auto missing_extension = glfw_required_extensions 
        | std::views::transform([](const char* ptr) { return std::string_view(ptr); })
        | RangesUtil::findIf([&](std::string_view sv) {
            auto found = provided_extensions 
                | std::views::transform([](const auto& props) 
                    { return std::string_view(props.extensionName); })
                | RangesUtil::find(sv);
            return !found;
        });

    if (missing_extension.has_value()) {
        throw std::runtime_error("Required GLFW extension not supported: " 
            + std::string(missing_extension.value()));
    }

    std::vector<std::string_view> required_layers;
    std::vector<const char*> required_layers_raw;

    if (ENABLE_VALIDATIONS_LAYERS) {
        required_layers.assign(APP_VALIDATIONS_LAYERS.begin(), APP_VALIDATIONS_LAYERS.end());
        required_layers_raw = required_layers 
            | std::views::transform([](const auto& sv) 
                    { return sv.data(); })
            | RangesUtil::toList();

        auto provided_layers = this->vkContext.enumerateInstanceLayerProperties();
        auto missing_provider = required_layers | RangesUtil::findIf([&](std::string_view sv) {
            auto found = provided_layers 
                | std::views::transform([](const auto& props) 
                    { return std::string_view(props.layerName); })
                | RangesUtil::find(sv);
            return !found;
        });
        if (missing_provider.has_value()) {
            throw std::runtime_error("Required layer not supported: " 
                + std::string(missing_provider.value()));
        }
    }

    
    auto createInfo = vk::InstanceCreateInfo()
        .setPApplicationInfo(&appInfo)
        .setPEnabledExtensionNames({static_cast<uint32_t>(glfw_required_extensions.size()), glfw_required_extensions.data()})
        .setPEnabledLayerNames(required_layers_raw);
        
    this->vkInstance = vk::raii::Instance(this->vkContext, createInfo);
}



