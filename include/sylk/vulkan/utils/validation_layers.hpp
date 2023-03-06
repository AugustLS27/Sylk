//
// Created by August Silva on 5-3-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>

#include <array>
#include <vector>
#include <string>

namespace vk {
    class Instance;
}

namespace sylk {
    class ValidationLayers {
        static constexpr std::array required_layers_ {
            "VK_LAYER_KHRONOS_validation"
        };

        std::vector<std::string> available_layers_;
        vk::Instance& vk_instance_;

    public:
        static constexpr bool enabled() {
#ifdef SYLK_DEBUG
            return true;
#else
            return false;
#endif
        }

        explicit ValidationLayers(vk::Instance& instance);

        bool supports_required_layers() ; // NOLINT(modernize-use-nodiscard)

        [[nodiscard]] static u32 enabled_layer_count();
        [[nodiscard]] static const char* const* enabled_layer_names();

    private:
        void fetch_available_validation_layers();
    };
}
