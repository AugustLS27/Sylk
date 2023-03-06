//
// Created by August Silva on 5-3-23.
//

#include <sylk/vulkan/utils/result_handler.hpp>
#include <magic_enum/magic_enum.hpp>

#include <vulkan/vulkan.hpp>

#define SYLK_EXPOSE_LOG_CONSTANTS
#include <sylk/core/utils/log.hpp>

namespace sylk {
    void handle_result(vk::Result result, const char* error_message, bool terminate) {
        if (result != vk::Result::eSuccess) {
            log((terminate ? CRITICAL : ERROR), "{} | {}", error_message, magic_enum::enum_name(result));
        }
    }
}
