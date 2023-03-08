//
// Created by August Silva on 5-3-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>
#include <sylk/vulkan/utils/constants.hpp>

#include <array>
#include <vector>
#include <string>

namespace vk {
    class Instance;
}

namespace sylk {
    class ValidationLayers {
    public:
        static constexpr bool enabled() {
#ifdef SYLK_DEBUG
            return true;
#else
            return false;
#endif
        }

        explicit ValidationLayers(vk::Instance& instance);

        bool supports_required_layers();

        [[nodiscard]] u32 enabled_layer_count();
        [[nodiscard]] const std::vector<const char*>& enabled_layer_container();

    private:
        void fetch_available_validation_layers();

    private:
        const std::vector<const char*> required_layers_ {
                VK_LAYER_KHRONOS_NAME
        };

        std::vector<std::string> available_layers_;
        vk::Instance& vk_instance_;
    };
}
