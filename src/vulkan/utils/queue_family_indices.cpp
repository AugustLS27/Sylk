//
// Created by August Silva on 10-3-23.
//

#include <sylk/vulkan/utils/queue_family_indices.hpp>
#include <sylk/core/utils/log.hpp>

auto sylk::QueueFamilyIndices::find(vk::PhysicalDevice device, vk::SurfaceKHR surface) -> sylk::QueueFamilyIndices {
    log(TRACE, "Querying available device queue families...");

    QueueFamilyIndices indices;
    const auto families = device.getQueueFamilyProperties();

    for (i32 i = 0; i < families.size(); ++i) {
        if(families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics = i;
        }

        if (device.getSurfaceSupportKHR(i, surface)) {
            indices.presentation = i;
        }

        if (indices.has_required()) {
            break;
        }
    }
    return indices;
}
