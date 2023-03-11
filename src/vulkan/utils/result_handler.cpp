//
// Created by August Silva on 5-3-23.
//

#include <sylk/core/utils/log.hpp>
#include <sylk/vulkan/vulkan.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>

#include <magic_enum/magic_enum.hpp>

namespace sylk {
    void handle_result(vk::Result result, const char* error_message, ELogLvl severity) {
        if (result != vk::Result::eSuccess) {
            log(severity, "{}: {}", error_message, magic_enum::enum_name(result));
        }
    }
}
