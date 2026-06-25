#include "App.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include "utils.hpp"
#include <ranges>
#include <span>
#include <cstdint>
#include <limits>
#include <algorithm>

const std::vector<const char*> APP_VALIDATIONS_LAYERS = {
    "VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
constexpr bool ENABLE_VALIDATIONS_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATIONS_LAYERS = true;
#endif

App* App::SHARED_INSTANCE = nullptr;

App& App::get() {
    assert(SHARED_INSTANCE != nullptr && "App instance is not initialized!");
    return *SHARED_INSTANCE;
}

App::App() {
    assert(App::SHARED_INSTANCE == nullptr && "App instance already exists!");
    SHARED_INSTANCE = this;
}

App::~App() {
    SHARED_INSTANCE = nullptr;
}

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
        this->vulkanContext.renderFrame();
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


namespace {
    std::vector<const char*> getRequiredInstanceExtensions() {
        auto required_extensions = GLFWUtil::getRequiredInstanceExtensions()
            | RangesUtil::toList();

        if (ENABLE_VALIDATIONS_LAYERS) {
            required_extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }
        return required_extensions;
    }
}

void VulkanContext::initInstance() {
    auto appInfo = vk::ApplicationInfo()
        .setPApplicationName("Rotating Cubes")
        .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
        .setPEngineName("No Engine")
        .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
        .setApiVersion(vk::ApiVersion14);


    auto required_extensions = getRequiredInstanceExtensions();
    
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

void VulkanContext::initSurface() {
    App& app = App::get();
    auto window = app.getWindow();
    
    VkSurfaceKHR surface_raw;
    if (glfwCreateWindowSurface(*(this->vkInstance), window, nullptr, &surface_raw) != 0) {
        throw std::runtime_error("Failed to create Window Surface!");
    }

    this->surface = vk::raii::SurfaceKHR(this->vkInstance, surface_raw);
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
    auto physical_devices = this->vkInstance.enumeratePhysicalDevices();
    if (physical_devices.empty())
        throw std::runtime_error("Can't find GPU with Vulkan Support");

    ChopinLogger::l("Vulkan devices:");
    for (const auto& device : physical_devices) {
        auto props = device.getProperties();
        ChopinLogger::l(props.deviceName);
    }

    auto selected_device = physical_devices 
        | std::views::filter(isDeviceSuitable)
        | RangesUtil::findFirst() | RangesUtil::asOptional();

    if (!selected_device.has_value())
        throw std::runtime_error("Can't find any suitable Vulkan devices");
    
    ChopinLogger::l(std::string("Selecting Vulkan device: ") 
        + std::string(selected_device.value().getProperties().deviceName.data()));

    this->physicalDevice = selected_device.value();
}

void VulkanContext::createLogicalDevice() {
    auto queue_family_props = this->physicalDevice.getQueueFamilyProperties();

    auto queue_family_index = 
        std::views::iota(0u, static_cast<uint32_t>(queue_family_props.size()))
        | std::views::filter([&](uint32_t indx) {
            auto queue_flags = queue_family_props[indx].queueFlags;
            return (queue_flags & vk::QueueFlagBits::eGraphics)
                && this->physicalDevice.getSurfaceSupportKHR(indx, *(this->surface));
        }) | RangesUtil::findFirst() | RangesUtil::asOptional()
        | OptionalUtil::orElseThrow("Can't find a sutable queue that support both graphics and presenting");

    const float queue_priority = 0.5f;
    const auto device_queue_create_args = vk::DeviceQueueCreateInfo()
        .setQueueFamilyIndex(queue_family_index)
        .setQueueCount(1)
        .setPQueuePriorities(&queue_priority);
    
    
    auto using_device_feature = vk::StructureChain{
        vk::PhysicalDeviceFeatures2(),
        vk::PhysicalDeviceVulkan11Features()
            .setShaderDrawParameters(true),
        vk::PhysicalDeviceVulkan13Features()
            .setDynamicRendering(true)
            .setSynchronization2(true),
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT()
            .setExtendedDynamicState(true)
    };


    std::vector<const char*> required_device_ext = {vk::KHRSwapchainExtensionName};

    auto device_create_args = vk::DeviceCreateInfo()
        .setPNext(&using_device_feature.get<vk::PhysicalDeviceFeatures2>())
        .setQueueCreateInfoCount(1)
        .setPQueueCreateInfos(&device_queue_create_args)
        .setPEnabledExtensionNames(required_device_ext);

    this->device = vk::raii::Device(this->physicalDevice, device_create_args);
    this->queue = vk::raii::Queue(this->device, queue_family_index, 0);
    this->queueFamilyIndex = queue_family_index;
}


namespace {
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& providedFormats) {
        
        assert(!providedFormats.empty());

        auto ideal_format = providedFormats
            | std::views::filter([](const auto &format) { 
                return format.format == vk::Format::eB8G8R8A8Srgb 
                && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; 
            })
            | RangesUtil::findFirst();

        if (!ideal_format.empty())
            return ideal_format.front();

        return providedFormats[0];
    }

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& providedPresentMode) {
        const bool has_efifo = providedPresentMode 
            | RangesUtil::anyMatch([](auto presentMode) { 
                return presentMode == vk::PresentModeKHR::eFifo; 
            });

        assert(has_efifo);

        const bool has_mailbox = providedPresentMode 
            | RangesUtil::anyMatch([](auto presentMode) { 
                return presentMode == vk::PresentModeKHR::eMailbox; 
            });
        
        return has_mailbox ? vk::PresentModeKHR::eMailbox 
            : vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
        const bool do_choose_extent = 
            capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max();
        if (!do_choose_extent) {
            return capabilities.currentExtent;
        }

        App& app = App::get();
        int width, height;
        glfwGetFramebufferSize(app.getWindow(), &width, &height);

        return {
            Mth::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            Mth::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    uint32_t chooseSwapMinImageCount(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
        const uint32_t ideal_value = 3;

        const uint32_t surface_min = surfaceCapabilities.minImageCount;
        const uint32_t surface_max = surfaceCapabilities.maxImageCount;

        const bool has_max = surfaceCapabilities.maxImageCount > 0;
        
        return has_max ?
            Mth::clamp<uint32_t>(ideal_value, surface_min, surface_max)
            : std::max(surface_min, ideal_value);
    }
}

void VulkanContext::createSwapChain() {
    auto surface_cap = this->physicalDevice.getSurfaceCapabilitiesKHR( *(this->surface) );
    this->swapChainExtent = chooseSwapExtent(surface_cap);
    uint32_t min_image_count = chooseSwapMinImageCount(surface_cap);
    
    auto provided_formats = this->physicalDevice.getSurfaceFormatsKHR( *(this->surface) );
    this->swapChainSurfaceFormat = chooseSwapSurfaceFormat(provided_formats);

    auto provided_present_modes = this->physicalDevice.getSurfacePresentModesKHR( surface );
    auto present_mode = chooseSwapPresentMode(provided_present_modes);

    auto swap_chain_create_args = vk::SwapchainCreateInfoKHR()
        .setSurface(*surface)
        .setMinImageCount(min_image_count)
        .setImageFormat(this->swapChainSurfaceFormat.format)
        .setImageColorSpace(this->swapChainSurfaceFormat.colorSpace)
        .setImageExtent(this->swapChainExtent)
        .setPresentMode(present_mode)
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setPreTransform(surface_cap.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setClipped(true);

    this->swapChain = vk::raii::SwapchainKHR(this->device, swap_chain_create_args);
    this->swapChainImages = this->swapChain.getImages();
    ChopinLogger::l("Accquired swap chain with:");
    ChopinLogger::l(std::format("Swap Extent : {} x {}", 
        this->swapChainExtent.width, this->swapChainExtent.height));
    ChopinLogger::l(std::format("Min Images Count : {}", min_image_count));
    ChopinLogger::l(std::format("Format - Color Space : {}", vk::to_string(this->swapChainSurfaceFormat.colorSpace)));
    ChopinLogger::l(std::format("Format - Format : {}", vk::to_string(this->swapChainSurfaceFormat.format)));
    ChopinLogger::l(std::format("Present Mode : {}", vk::to_string(present_mode)));
    ChopinLogger::l(std::format("Images Count : {}", this->swapChainImages.size()));

    ChopinLogger::l("========");
}

void VulkanContext::createImageViews() {
    assert(this->swapChainImageViews.empty());
    
    auto image_create_args = vk::ImageViewCreateInfo()
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(this->swapChainSurfaceFormat.format)
        .setSubresourceRange(
            vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseMipLevel(0)
                .setLevelCount(1)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
        );

    for (auto &image : swapChainImages){
        this->swapChainImageViews
            .emplace_back( device, image_create_args.setImage(image) );
    }
}

void VulkanContext::setupRenderPipeline() {
    
    using VkShaderStages = vk::ShaderStageFlagBits;

    const std::string shader_path = "shaders/shader.spv";
    auto shader_result = SpirvUtil::shaderModuleAndStagesFromFile(
        this->device, shader_path,
        VkShaderStages::eVertex | VkShaderStages::eFragment
    );

    ChopinLogger::l(std::format("Loaded SPIR-V Binary from : {}", shader_path));
    for (const auto& entry : shader_result.pipelineStages) {
        const auto msg = std::format(
            "Discovered Entry Point for [ {} ] : Name = [ {} ] Stage = [ {} ]",
            shader_path,
            std::string_view(entry.pName), vk::to_string(entry.stage)
        );
        ChopinLogger::l(msg);
    }

    auto dyanmic_states_list = std::vector<vk::DynamicState>{
        vk::DynamicState::eViewport, vk::DynamicState::eScissor
    };
    auto dynamic_state = vk::PipelineDynamicStateCreateInfo()
        .setDynamicStates(dyanmic_states_list);
    
    auto vertex_layout = vk::PipelineVertexInputStateCreateInfo();

    auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList);

    auto viewport_state = vk::PipelineViewportStateCreateInfo()
        .setViewportCount(1).setScissorCount(1);

    auto rasterizer_state = vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(vk::False)
        .setRasterizerDiscardEnable(vk::False)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setDepthBiasEnable(vk::False)
        .setLineWidth(1.f);

    auto multisampling_state = vk::PipelineMultisampleStateCreateInfo()
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setSampleShadingEnable(vk::False);

    auto blend_attachments = std::vector{
        vk::PipelineColorBlendAttachmentState()
            .setBlendEnable(true)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
            .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd)
            .setColorWriteMask(
                vk::ColorComponentFlagBits::eR 
                | vk::ColorComponentFlagBits::eG 
                | vk::ColorComponentFlagBits::eB 
                | vk::ColorComponentFlagBits::eA
            )
    };

    auto blend_state = vk::PipelineColorBlendStateCreateInfo()
        .setLogicOpEnable(vk::False)
        .setLogicOp(vk::LogicOp::eCopy)
        .setAttachments(blend_attachments);

    auto pipeline_layout = vk::PipelineLayoutCreateInfo()
        .setSetLayouts(nullptr)
        .setPushConstantRanges(nullptr);

    this->pipelineLayout = vk::raii::PipelineLayout(this->device, pipeline_layout);

    auto pipeline_create_chain = vk::StructureChain{
        vk::GraphicsPipelineCreateInfo()
            .setPDynamicState(&dynamic_state)
            .setStages(shader_result.pipelineStages)
            .setPVertexInputState(&vertex_layout)
            .setPInputAssemblyState(&input_assembly)
            .setPViewportState(&viewport_state)
            .setPRasterizationState(&rasterizer_state)
            .setPMultisampleState(&multisampling_state)
            .setPColorBlendState(&blend_state)
            .setLayout(this->pipelineLayout)
            .setRenderPass(nullptr),
        vk::PipelineRenderingCreateInfo()
            .setColorAttachmentFormats(this->swapChainSurfaceFormat.format)
    };
    
    this->renderPipeline = vk::raii::Pipeline(this->device, nullptr, 
        pipeline_create_chain.get<vk::GraphicsPipelineCreateInfo>());

    ChopinLogger::l("Created Render Pipeline");
}

void VulkanContext::createCommandPool() {
    auto pool_args = vk::CommandPoolCreateInfo()
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        .setQueueFamilyIndex(this->queueFamilyIndex);

    this->commandPool = vk::raii::CommandPool(this->device, pool_args);
}

void VulkanContext::createCommandBuffer() {
    auto alloc_args = vk::CommandBufferAllocateInfo()
        .setCommandPool(this->commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);
    
    this->commandBuffer = 
        std::move(vk::raii::CommandBuffers(this->device, alloc_args).front());
}

void VulkanContext::recordCommandBuffer(uint32_t imageIndex) {
    this->commandBuffer.begin({});

    this->transitionImageLayout(
        imageIndex,
        
        vk::ImageLayout::eUndefined, 
        vk::ImageLayout::eColorAttachmentOptimal,
        
        {}, 
        vk::AccessFlagBits2::eColorAttachmentWrite,
        
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    auto clear_color = vk::ClearColorValue(0.f, 0.f, 0.f, 1.f);
    auto attachment_info = vk::RenderingAttachmentInfo()
        .setImageView(this->swapChainImageViews[imageIndex])
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(clear_color);

    auto rendering_info = vk::RenderingInfo()
        .setRenderArea(vk::Rect2D({0, 0}, this->swapChainExtent))
        .setLayerCount(1)
        .setColorAttachments(attachment_info);


    //Rendering
    this->commandBuffer.beginRendering(rendering_info);

    this->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, this->renderPipeline);
    
    auto viewport = vk::Viewport()
        .setX(0.f).setY(0.f)
        .setWidth(this->swapChainExtent.width)
        .setHeight(this->swapChainExtent.height)
        .setMinDepth(0.f).setMaxDepth(1.f);
    
    auto scrissor = vk::Rect2D()
        .setOffset({0, 0}).setExtent(this->swapChainExtent);

    this->commandBuffer.setViewport(0, viewport);
    this->commandBuffer.setScissor(0, scrissor);

    this->commandBuffer.draw(3, 1, 0, 0);

    this->commandBuffer.endRendering();
    //End Rendering

    this->transitionImageLayout(
        imageIndex,

        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,

        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},

        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffer.end();
}

void VulkanContext::createAsyncHelper() {
    this->whenImageAccquired = vk::raii::Semaphore(this->device, vk::SemaphoreCreateInfo());
    this->whenRenderedToImage = vk::raii::Semaphore(this->device, vk::SemaphoreCreateInfo());
    this->whenRenderedToImageJoin = vk::raii::Fence(this->device, 
        vk::FenceCreateInfo()
            .setFlags(vk::FenceCreateFlagBits::eSignaled)
    );

}

void VulkanContext::renderFrame() {
    
    VkUtil::waitFenceAndReset(this->device, this->whenRenderedToImageJoin);

    auto [r1, image_index] = swapChain.acquireNextImage(UINT64_MAX, *(this->whenImageAccquired), nullptr);

    this->recordCommandBuffer(image_index);

    auto wait_dst_stage_mask = vk::PipelineStageFlags(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);
    auto submit_args = vk::SubmitInfo()
        .setWaitSemaphores(*(this->whenImageAccquired))
        .setWaitDstStageMask(wait_dst_stage_mask)
        .setCommandBuffers(*(this->commandBuffer))
        .setSignalSemaphores(*(this->whenRenderedToImage));

    this->queue.submit(submit_args, *(this->whenRenderedToImageJoin));

    
    auto present_args = vk::PresentInfoKHR()
        .setWaitSemaphores(*(this->whenRenderedToImage))
        .setSwapchains(*(this->swapChain))
        .setImageIndices(image_index);

    auto present_result = this->queue.presentKHR(present_args);
}