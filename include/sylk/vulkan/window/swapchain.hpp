//
// Created by August Silva on 7-3-23.
//

#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace sylk {

    class Swapchain {
    public:
        struct SupportDetails {
            vk::SurfaceCapabilitiesKHR surface_capabilities;
            std::vector<vk::SurfaceFormatKHR> surface_formats;
            std::vector<vk::PresentModeKHR> present_modes;
        };

    private:

    public:
        SupportDetails device_support_details(vk::PhysicalDevice device, vk::SurfaceKHR surface);


    };

}
