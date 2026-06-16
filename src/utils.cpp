#include "./utils.hpp"
#include <stdexcept>
#include <fstream>
#include <string>
#include <iostream>
#include <functional>
#include <ranges>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spirv-reflect/spirv_reflect.h>
#include <vulkan/vulkan_raii.hpp>
#include <format>

std::array<float, 3> ColorUtils::RGB::toArray(int color) {
    int r = (color >> 16) & 255;
    int g = (color >> 8) & 255;
    int b = (color >> 0) & 255;
    return {r / 255.0f, g / 255.0f, b / 255.0f};
}

int ColorUtils::RGB::toInt(std::array<float, 3> color) {
    std::array<int, 3> color_rgb;
    for (int i = 0; i < color_rgb.size(); ++i) {
        color_rgb[i] = (int) (color[i] * 255);
    }
    return (color_rgb[0] * 255) << 16
        | color_rgb[1] << 8
        | color_rgb[2];
}

std::array<float, 3> ColorUtils::RGB::lerpColor(float progress,
    std::array<float, 3> color1, std::array<float, 3> color2) {
    progress = Mth::clamp(progress, 0, 1);
    return {
        color1[0] + progress * (color2[0] - color1[0]),
        color1[1] + progress * (color2[1] - color1[1]),
        color1[2] + progress * (color2[2] - color1[2]),
    };
}

int ColorUtils::RGB::lerpColor(float progress, int color1, int color2) {
    auto color_array = lerpColor(progress, toArray(color1), toArray(color2));
    return toInt(color_array);
}

float Mth::normNDC(float ndc) {
    return clamp((ndc + 1)/2, 0.0f, 1.0f);
}

std::string FileUtils::readFileToString(const std::string& filename) {
    std::ifstream input(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::string ret;
    std::string temp;
    while (std::getline(input, temp)) {
        ret.append(temp + "\n" );
    }
    return ret;
}

std::vector<char> FileUtils::readFileAsByte(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("failed to open file : " + filename);

    std::vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    return buffer;
}

void ChopinLogger::l(const std::string& msg) {
    std::cout << msg << std::endl;
}

void ChopinLogger::l(const std::exception& exception) {
    std::cout << exception.what() << std::endl;;
}

void ChopinLogger::lerr(const std::string& msg) {
    std::cerr << msg << std::endl;
}

std::string AssetsUtil::res(const std::string& path) {
    return "assets/" + path;
}

auto ListUtil::unwrapCStr(std::vector<std::string_view> target) -> std::vector<const char*> {
    return target 
        | std::views::transform([](const auto& sv) { return sv.data(); })
        | RangesUtil::toList();     
}

auto SpirvUtil::readFromFile(const std::string& filename) -> std::vector<uint32_t> {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("failed to open file: " + filename);

    size_t file_size_bytes = static_cast<size_t>(file.tellg());
    size_t buffer_size = file_size_bytes / sizeof(uint32_t);
    size_t remainder_bytes = file_size_bytes % sizeof(uint32_t); 

    if (remainder_bytes != 0)
        throw std::runtime_error("Trying to read corrupted SPIR-V file (size is not a multiple of 4): " + filename);

    std::vector<uint32_t> buffer(buffer_size);

    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(file_size_bytes)))
        throw std::runtime_error("Failed to read the entire SPIR-V file: " + filename);;

    return buffer;
}

auto SpirvReflectUtil::reflectEntryPointsFromBuf(const std::vector<uint32_t>& buffer) 
    -> std::vector<SpirvReflectUtil::SpvEntryPoint> {

    SpirvReflectUtil::RaiiShaderModule reflect_module_raii(buffer);
    const auto& reflect_module = reflect_module_raii.get();

    uint32_t entry_count = reflect_module.entry_point_count;
    
    std::vector<SpirvReflectUtil::SpvEntryPoint> ret;
    ret.reserve(entry_count);

    for (uint32_t i = 0; i < entry_count; ++i) {
        const auto& entry_point = reflect_module.entry_points[i];

        ret.emplace_back(
            static_cast<vk::ShaderStageFlagBits>(entry_point.shader_stage),
            entry_point.name
        );
    }
    return ret;
}

auto SpirvUtil::readfromFileWithReflect(const std::string& filename) 
    -> SpirvUtil::SpvWithReflect {

    SpirvUtil::SpvWithReflect ret;

    ret.buffer = SpirvUtil::readFromFile(filename);
    std::vector<SpirvReflectUtil::SpvEntryPoint> entry_points;
    try {
        ret.entryPoints = SpirvReflectUtil::reflectEntryPointsFromBuf(ret.buffer);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::format(
            "Exception thrown when reflecting SPV file [ {} ]: {}",
            filename, e.what()
        ));
    }

    return ret;
}

[[nodiscard]]
auto SpirvUtil::shaderModuleAndStagesFromFile(
        const vk::raii::Device& device, 
        const std::string& filename,
        vk::ShaderStageFlags requiredStages
    ) -> SpirvUtil::SpvShaderModuleAndStage {

    SpirvUtil::SpvShaderModuleAndStage ret;

    auto read_result = SpirvUtil::readfromFileWithReflect(filename);
    ret.shaderModule = vk::raii::ShaderModule(
        device, 
        vk::ShaderModuleCreateInfo().setCode(read_result.buffer)
    );

    vk::ShaderStageFlags found_stages;

    ret.stringPool.reserve(read_result.entryPoints.size());
    ret.pipelineStages.reserve(read_result.entryPoints.size());
    for (auto& entry : read_result.entryPoints) {
        found_stages |= entry.stage;

        ret.stringPool.push_back(std::make_unique<std::string>(std::move(entry.name)));
        
        const auto& name = *(ret.stringPool.back());
        ret.pipelineStages.push_back(
            vk::PipelineShaderStageCreateInfo()
                .setModule(ret.shaderModule)
                .setStage(entry.stage)
                .setPName(name.c_str())
        );
    }

    const bool has_required_stages =
        (found_stages & requiredStages) == requiredStages; 

    if (!has_required_stages) {
        auto missing_stages = vk::ShaderStageFlags(requiredStages & ~found_stages);
        
        throw std::runtime_error(std::format(
            "SPIR-V file [ {} ] is missing required shader stages: {}",
            filename, vk::to_string(missing_stages)
        ));
    }
    
    return ret;
}