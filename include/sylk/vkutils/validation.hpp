//
// Created by August Silva on 5-3-23.
//

#pragma once

#include <array>
#include <vector>

namespace vk {
    enum class Result;
    class Instance;
}

namespace sylk {
    constexpr bool validation_layers_enabled() {
#ifdef SYLK_DEBUG
        return true;
#else
        return false;
#endif
    }

    void vk_validate_result(vk::Result result, const char* error_message, bool terminate = false);

    class ValidationLayers {
        static constexpr std::array required_layers_ {
            "VK_LAYER_KHRONOS_validation"
        };

        std::vector<const char*> available_layers_;
        vk::Instance& vk_instance_;

    public:
        explicit ValidationLayers(vk::Instance& instance);

    private:
        bool check_layer_support() const; // NOLINT(modernize-use-nodiscard)

    };
}
