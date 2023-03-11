//
// Created by August Silva on 10-3-23.
//

#pragma once

#include <sylk/core/utils/rust_style_types.hpp>

#include <sylk/vulkan/vulkan.hpp>

#include <optional>

namespace sylk {
    struct QueueFamilyIndices {
        std::optional<u32> graphics;
        std::optional<u32> presentation;

        SYLK_NODISCARD auto has_required() const -> bool {
            return graphics.has_value() && presentation.has_value();
        }

        static auto find(vk::PhysicalDevice device, vk::SurfaceKHR surface) -> QueueFamilyIndices;
    };
}