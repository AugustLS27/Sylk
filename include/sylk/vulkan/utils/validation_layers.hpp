//
// Created by August Silva on 5-3-23.
//

#ifndef SYLK_VULKAN_UTILS_VALIDATIONLAYERS_HPP
#define SYLK_VULKAN_UTILS_VALIDATIONLAYERS_HPP

#include <sylk/core/utils/short_types.hpp>
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
        static constexpr auto enabled() -> bool {
#ifdef SYLK_DEBUG
            return true;
#else
            return false;
#endif
        }

        explicit ValidationLayers(vk::Instance& instance);

        auto supports_required_layers() -> bool;

        SYLK_NODISCARD auto enabled_layer_count() -> u32;
        SYLK_NODISCARD auto enabled_layer_container() -> const std::vector<const char*>&;

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

#endif // SYLK_VULKAN_UTILS_VALIDATIONLAYERS_HPP