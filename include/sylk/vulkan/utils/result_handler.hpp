//
// Created by August Silva on 5-3-23.
//

#ifndef SYLK_VULKAN_UTILS_RESULTHANDLER_HPP
#define SYLK_VULKAN_UTILS_RESULTHANDLER_HPP

#include <sylk/core/utils/log.hpp>

namespace vk {
    enum class Result;
}

namespace sylk {
    void handle_result(vk::Result result, const char* error_message, ELogLvl severity = ELogLvl::WARN);
}

#endif // SYLK_VULKAN_UTILS_RESULTHANDLER_HPP