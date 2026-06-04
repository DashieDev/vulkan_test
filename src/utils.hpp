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
#include <algorithm>
#include <stdexcept>

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
    void ln(const std::string& msg);
    void lerr(const std::string& msg);
}

namespace RangesUtil {

    enum class MatchType { Any, All, None };
    template <typename Pred, MatchType type>
    struct AnyMatchArgs { Pred pred; };

    template <typename Pred>
    auto anyMatch(Pred pred) -> AnyMatchArgs<Pred, MatchType::Any> {
        return AnyMatchArgs<Pred, MatchType::Any>{std::move(pred)};
    }

    template <typename Pred>
    auto noMatch(Pred pred) -> AnyMatchArgs<Pred, MatchType::None> {
        return AnyMatchArgs<Pred, MatchType::None>{std::move(pred)};
    }

    template <typename Pred>
    auto allMatch(Pred pred) -> AnyMatchArgs<Pred, MatchType::All> {
        return AnyMatchArgs<Pred, MatchType::All>{std::move(pred)};
    }

    template <std::ranges::input_range R, typename Pred, MatchType Type>
    requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>
    auto operator| (R&& target, AnyMatchArgs<Pred, Type> args) -> bool {
        if constexpr (Type == MatchType::All) {
            return std::ranges::all_of(target, std::move(args.pred));
        } else if constexpr (Type == MatchType::None) {
            return std::ranges::none_of(target, std::move(args.pred));
        } else {
            return std::ranges::any_of(target, std::move(args.pred));
        }
    }


    struct FindFirstArgs {};

    inline auto findFirst() -> FindFirstArgs {
        return FindFirstArgs{};
    }

    template <std::ranges::input_range R>
    auto operator| (R&& target, FindFirstArgs) {
        return std::forward<R>(target) | std::views::take(1); 
    }


    //====

    template <typename Pred>
    struct FirstMatchAtArgs { Pred pred; };

    template <typename Pred>
    inline auto firstMatchAt(Pred pred) -> FirstMatchAtArgs<Pred> {
        return FirstMatchAtArgs{pred};
    }

    template <std::ranges::input_range R, typename Pred>
    requires std::indirect_unary_predicate<Pred, std::ranges::iterator_t<R>>
    auto operator| (R&& target, FirstMatchAtArgs<Pred> args) -> std::optional<int> {
        auto at = std::ranges::find_if(target, std::move(args.pred));
        if (at == std::ranges::end(target))
            return std::nullopt;

        return std::optional(std::distance(std::begin(target), at));
    }

    //=====


    //So I don't have to create a dedicated named variable for the pipeline
    template <std::ranges::input_range R>
    auto toList(R&& target) -> std::vector<std::ranges::range_value_t<R>> {
        std::vector<std::ranges::range_value_t<R>> vec;

        if constexpr (std::ranges::sized_range<R>) {
            vec.reserve(std::ranges::size(target));
        }
        
        if constexpr (!std::is_reference_v<R>) {
            std::ranges::move(target, std::back_inserter(vec));
        } else {
            std::ranges::copy(target, std::back_inserter(vec));
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


    template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, const char*>
    inline auto strView(R&& target) {
        return std::forward<R>(target) | std::views::transform([](const char* str) 
            { return std::string_view(str); });
    }

    struct ToOptionalArgs {};
    inline auto asOptional() { return ToOptionalArgs{}; }

    template <std::ranges::input_range R>
    auto operator| (R&& target, ToOptionalArgs) -> std::optional<std::ranges::range_value_t<R>> {
        if (std::ranges::empty(target)) {
            return std::nullopt;
        }
        return *std::ranges::begin(target); // or std::optional(std::ranges::iter_move(std::ranges::begin(target)))
    }

    struct PresentArgs { bool inverted; };
    inline auto isPresent() { return PresentArgs{false}; }
    inline auto isEmpty() { return PresentArgs{true}; }

    template <std::ranges::input_range R>
    auto operator| (R&& target, PresentArgs) -> bool {
        return !std::ranges::empty(target);
    }
}

namespace OptionalUtil {

    struct OrElseThrowArgs { std::string_view str; };

    inline auto orElseThrow(std::string_view str) -> OrElseThrowArgs {
        return OrElseThrowArgs{str};
    }

    template<typename T>
    auto operator| (const std::optional<T>& target, OrElseThrowArgs args) -> T {
        if (!target.has_value())
            throw std::runtime_error(std::string(args.str));
        return target.value();
    } 

    template<typename T>
    auto operator| (std::optional<T>&& target, OrElseThrowArgs args) -> T {
        if (!target.has_value()) {
            throw std::runtime_error(std::string(args.str));
        }
        return std::move(target.value()); 
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
        uint32_t gflw_required_count = 0;
        auto glfw_required_ptr = glfwGetRequiredInstanceExtensions(&gflw_required_count);
        
        return std::views::counted(glfw_required_ptr, gflw_required_count);
    }

}

#endif