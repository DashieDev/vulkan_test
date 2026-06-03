#include "App.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include "utils.hpp"
#include <ranges>
#include <span>

const std::vector<const char*> APP_VALIDATIONS_LAYERS = {
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


    auto required_extensions = GLFWUtil::getRequiredInstanceExtensions()
        | RangesUtil::toList();

    if (ENABLE_VALIDATIONS_LAYERS) {
        required_extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    
    auto provided_extensions = this->vkContext.enumerateInstanceExtensionProperties();
    auto missing_extension = RangesUtil::strView(required_extensions) 
        | std::views::filter([&](std::string_view sv) {
            return provided_extensions 
                | RangesUtil::noMatch([&](const auto& props) 
                    { return std::string_view(props.extensionName) == sv; });
        }) | RangesUtil::findFirst() | RangesUtil::asOptional();

    if (missing_extension.has_value()) {
        throw std::runtime_error("Required GLFW extension not supported: " 
            + std::string(missing_extension.value()));
    }


    std::span<const char* const> required_layers;

    if (ENABLE_VALIDATIONS_LAYERS) {
        required_layers = APP_VALIDATIONS_LAYERS;

        auto provided_layers = this->vkContext.enumerateInstanceLayerProperties();
        auto missing_provider = RangesUtil::strView(required_layers) 
            | std::views::filter([&](std::string_view sv) {
                return provided_layers 
                    | RangesUtil::noMatch(([&](const auto& props) 
                        { return std::string_view(props.layerName) == sv; }));
            }) 
            | RangesUtil::findFirst() | RangesUtil::asOptional(); //renamed toOptional to asOptional
        if (missing_provider.has_value()) {
            throw std::runtime_error("Required layer not supported: " 
                + std::string(missing_provider.value()));
        }
    }

    auto createInfo = vk::InstanceCreateInfo()
        .setPApplicationInfo(&appInfo)
        .setPEnabledExtensionNames(required_extensions)
        .setPEnabledLayerNames(required_layers);
        
    this->vkInstance = vk::raii::Instance(this->vkContext, createInfo);
}

namespace {
    VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
        vk::DebugUtilsMessageTypeFlagsEXT type,
        const vk::DebugUtilsMessengerCallbackDataEXT * pCallbackData,
        void * pUserData
    ) {
        
        ChopinLogger::lerr("validation layer: type " + to_string(type) + " msg: " + pCallbackData->pMessage + "\n");

        return vk::False;
    }
}

void VulkanContext::initDebugMsgr() {
    if (!ENABLE_VALIDATIONS_LAYERS)
        return;

    vk::DebugUtilsMessageSeverityFlagsEXT severity_flags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
    );
    vk::DebugUtilsMessageTypeFlagsEXT message_type_flags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance 
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
    );


    auto debug_msgr_args = vk::DebugUtilsMessengerCreateInfoEXT()
        .setMessageSeverity(severity_flags)
        .setMessageType(message_type_flags)
        .setPfnUserCallback(&debugCallback);
    
    this->debugMessenger = this->vkInstance.createDebugUtilsMessengerEXT(debug_msgr_args);
}


namespace {
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& device) {
        auto props = device.getProperties();

        bool base_check = 
            props.apiVersion >= vk::ApiVersion14
            && props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;

        bool queue_families_check = device.getQueueFamilyProperties() 
            | RangesUtil::anyMatch([] (auto const& val) {
                return static_cast<bool>(val.queueFlags & vk::QueueFlagBits::eGraphics);
            });

        const std::vector<const char*> required_extensions = 
            {vk::KHRSwapchainExtensionName};
        auto provided_extensions = device.enumerateDeviceExtensionProperties();
        bool ext_support_check = RangesUtil::strView(required_extensions)
            | RangesUtil::allMatch([&](const auto& required){
                return provided_extensions 
                    | RangesUtil::anyMatch([&](const auto& provided) {
                        return required == std::string_view(provided.extensionName); 
                    });
            });
        
        auto features = device.getFeatures2<
            vk::PhysicalDeviceFeatures2, 
            vk::PhysicalDeviceVulkan13Features, 
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        >();

        bool feature_support_check = 
            features.get<vk::PhysicalDeviceFeatures2>().features.geometryShader
            && features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering 
            && features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

        return base_check && queue_families_check 
            && ext_support_check && feature_support_check;
    }
}

void VulkanContext::pickPhysicalDevice() {
    bool isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice);

    auto physical_devices = this->vkInstance.enumeratePhysicalDevices();
    if (physical_devices.empty())
        throw std::runtime_error("Can't find GPU with Vulkan Support");

    ChopinLogger::ln("Vulkan devices:");
    for (const auto& device : physical_devices) {
        auto props = device.getProperties();
        ChopinLogger::ln(props.deviceName);
    }

    auto selected_device = physical_devices 
        | std::views::filter(isDeviceSuitable)
        | RangesUtil::findFirst() | RangesUtil::asOptional();

    if (!selected_device.has_value())
        throw std::runtime_error("Can't find any suitable Vulkan devices");
    
    ChopinLogger::ln(std::string("Selecting Vulkan device: ") 
        + std::string(selected_device.value().getProperties().deviceName.data()));

    this->physicalDevice = selected_device.value();
}



