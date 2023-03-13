//
// Created by August Silva on 10-3-23.
//

#ifndef SYLK_VULKAN_UTILS_QUEUEFAMILYINDICES_HPP
#define SYLK_VULKAN_UTILS_QUEUEFAMILYINDICES_HPP

#include <sylk/core/utils/short_types.hpp>

#include <sylk/vulkan/vulkan.hpp>

#include <optional>

namespace sylk {
    struct QueueFamilyIndices {
        std::optional<u32> graphics;
        std::optional<u32> presentation;

        SYLK_NODISCARD auto has_required() const -> bool { return graphics.has_value() && presentation.has_value(); }

        static auto find(vk::PhysicalDevice device, vk::SurfaceKHR surface) -> QueueFamilyIndices;
    };
}

#endif  // SYLK_VULKAN_UTILS_QUEUEFAMILYINDICES_HPP