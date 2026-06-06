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



void ChopinLogger::l(const std::string& msg) {
    std::cout << msg;
}

void ChopinLogger::ln(const std::string& msg) {
    std::cout << msg << std::endl;
}

void ChopinLogger::l(const std::exception& exception) {
    std::cout << exception.what();
}

void ChopinLogger::lerr(const std::string& msg) {
    std::cerr << msg;
}

std::string AssetsUtil::res(const std::string& path) {
    return "assets/" + path;
}

auto ListUtil::unwrapCStr(std::vector<std::string_view> target) -> std::vector<const char*> {
    return target 
        | std::views::transform([](const auto& sv) { return sv.data(); })
        | RangesUtil::toList();     
}