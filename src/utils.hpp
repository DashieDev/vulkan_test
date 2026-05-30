#ifndef colorutils_included
#define colorutils_included

#include <array>
#include <string>
#include <functional>
#include <concepts>
#include <optional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ranges>
#include <GLFW/glfw3.h>

namespace ColorUtils {
    
    namespace RGB {
        using color_vec3 = std::array<float, 3>;
        color_vec3 toArray(int color);
        int toInt(color_vec3 color);
        color_vec3 lerpColor(float progress, color_vec3 color1, color_vec3 color2);
        int lerpColor(float progress, int color1, int color2);
    }

}

namespace Mth {
    float clamp(float x, float min, float max);
    float normNDC(float x);
}

namespace FileUtils {
    std::string readFileToString(const std::string& filename);
}

namespace ChopinLogger {
    void l(const std::string& msg);
    void l(const std::exception& exception);
    void lerr(const std::string& msg);
}

namespace RangesUtil {

    template <std::ranges::input_range R, typename Pred>
    requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>
    auto findIf(R&& target, Pred pred) -> std::optional<std::ranges::range_value_t<R>> {
        auto found_at = std::ranges::find_if(target, std::move(pred));
        if (found_at != std::ranges::end(target)) {
            return std::optional(*found_at);
        }
        return std::nullopt;
    }

    template <typename Pred>
    struct FindIfArgs { Pred pred; };

    template <typename Pred>
    auto findIf(Pred pred) -> FindIfArgs<Pred> {
        return FindIfArgs<Pred>{std::move(pred)};
    }

    template <std::ranges::input_range R, typename Pred>
    requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>
    auto operator| (R&& target, FindIfArgs<Pred> args) -> std::optional<std::ranges::range_value_t<R>> {
        auto found_at = std::ranges::find_if(target, std::move(args.pred));
        if (found_at != std::ranges::end(target)) {
            return std::optional(*found_at);
        }
        return std::nullopt;
    }

    
    template <typename T, std::ranges::input_range R>
    requires std::equality_comparable_with<std::ranges::range_reference_t<R>, const T&>
    auto find(R&& target, const T& toCompare) -> std::optional<T> {
        auto found_at = std::ranges::find(target, toCompare);
        if (found_at != std::ranges::end(target)) {
            return std::optional<T>(*found_at);
        }
        return std::nullopt;
    }

    template <typename T>
    struct FindArgs { T value; };

    template <typename T>
    auto find(T value) -> FindArgs<T> {
        return FindArgs<T>{std::move(value)};
    }

    template <typename T, std::ranges::input_range R>
    requires std::equality_comparable_with<std::ranges::range_reference_t<R>, const T&>
    auto operator| (R&& target, FindArgs<T> args) -> std::optional<T> {
        auto found_at = std::ranges::find(target, std::move(args.value));
        if (found_at != std::ranges::end(target)) {
            return std::optional<T>(*found_at);
        }
        return std::nullopt;
    }


    //So I don't have to create a dedicated named variable for the pipeline
    template <std::ranges::input_range R>
    auto toList(R&& target) -> std::vector<std::ranges::range_value_t<R>> {
        std::vector<std::ranges::range_value_t<R>> vec;

        if constexpr (std::ranges::sized_range<R>) {
            vec.reserve(std::ranges::size(target));
        }
        
        for (auto&& val : target) {
            vec.push_back(std::forward<decltype(val)>(val));
        }
        return vec;
    }

    struct ToListArgs {};

    inline auto toList() -> ToListArgs {
        return ToListArgs{};
    }

    template <std::ranges::input_range R>
    auto operator| (R&& target, ToListArgs args) -> std::vector<std::ranges::range_value_t<R>> {
        return toList(std::forward<R>(target));
    }
}

namespace AssetsUtil {

    std::string res(const std::string& path);

}

namespace ListUtil {

    auto unwrapCStr(std::vector<std::string_view> target) -> std::vector<const char*>;

}

namespace GLFWUtil {

    inline auto getRequiredInstanceExtensions() {
        uint32_t glfw_required_extensions_count = 0;
        auto glfw_required_extensions_ptr = glfwGetRequiredInstanceExtensions(&glfw_required_extensions_count);
        
        return std::views::counted(glfw_required_extensions_ptr, glfw_required_extensions_count);
    }

}

#endif