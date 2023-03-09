//
// Created by August Silva on 7-3-23.
//

#include <sylk/vulkan/window/swapchain.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/core/utils/rust_style_types.hpp>

#include <GLFW/glfw3.h>

#include <limits>
#include <algorithm>
#include <sylk/core/utils/cast.hpp>

constexpr sylk::u32 U32_LIMIT = std::numeric_limits<sylk::u32>::max();

namespace sylk {
    void Swapchain::create(CreateParams params) {
        device_ = params.device;

        const auto surface_format = select_surface_format(params.support_details.surface_formats);
        const auto present_mode = select_present_mode(params.support_details.present_modes);

        extent_ = select_extent_2d(params.support_details.surface_capabilities, params.window);
        format_ = surface_format.format;

        const auto max_image_count = params.support_details.surface_capabilities.maxImageCount;
        const auto min_image_count = params.support_details.surface_capabilities.minImageCount + 1;
        const bool has_max_images = max_image_count > 0;

        auto swapchain_img_count = (has_max_images && min_image_count > max_image_count ?
                                    max_image_count : min_image_count);

        auto create_info = vk::SwapchainCreateInfoKHR()
                .setSurface(params.surface)
                .setMinImageCount(swapchain_img_count)
                .setImageFormat(format_)
                .setImageColorSpace(surface_format.colorSpace)
                .setImageExtent(extent_)
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                .setImageSharingMode(params.sharing_mode)
                .setQueueFamilyIndices(params.queue_family_indices)
                .setPreTransform(params.support_details.surface_capabilities.currentTransform)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setPresentMode(present_mode)
                .setClipped(true)
                .setOldSwapchain(nullptr);

        swapchain_ = device_.createSwapchainKHR(create_info, nullptr);
        images_ = device_.getSwapchainImagesKHR(swapchain_);
    }

    void Swapchain::destroy() {
        for (auto view : image_views_) {
            device_.destroyImageView(view);
        }
        device_.destroySwapchainKHR(swapchain_);
    }

    Swapchain::SupportDetails Swapchain::query_device_support_details(vk::PhysicalDevice device, vk::SurfaceKHR surface) const {
        SupportDetails details;

        details.surface_capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.surface_formats = device.getSurfaceFormatsKHR(surface);
        details.present_modes = device.getSurfacePresentModesKHR(surface);

        return details;
    }

    vk::SurfaceFormatKHR Swapchain::select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const {
        for (const auto& format : available_formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }

        return available_formats.front();
    }

    vk::PresentModeKHR Swapchain::select_present_mode(const std::vector<vk::PresentModeKHR>& available_modes) const {
        for (const auto& mode : available_modes) {
            if (mode == vk::PresentModeKHR::eMailbox) {
                return mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D Swapchain::select_extent_2d(const vk::SurfaceCapabilitiesKHR capabilities, GLFWwindow* window) const {
        if (capabilities.currentExtent.width != U32_LIMIT) {
            return capabilities.currentExtent;
        }

        i32 window_width, window_height;
        glfwGetFramebufferSize(window, &window_width, &window_height);

        vk::Extent2D actual_extent{
                cast<u32>(window_width),
                cast<u32>(window_height)
        };

        actual_extent.width = std::clamp(actual_extent.width,
                                         capabilities.minImageExtent.width,
                                         capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height,
                                          capabilities.minImageExtent.height,
                                          capabilities.maxImageExtent.height);

        return actual_extent;
    }

    const std::vector<vk::Image>& Swapchain::retrieve_images() const {
        return images_;
    }

    void Swapchain::create_image_views() {
        image_views_.resize(images_.size());

        const auto component_mappings = vk::ComponentMapping()
                .setR(vk::ComponentSwizzle::eIdentity)
                .setG(vk::ComponentSwizzle::eIdentity)
                .setB(vk::ComponentSwizzle::eIdentity)
                .setA(vk::ComponentSwizzle::eIdentity);

        const auto subresource_range = vk::ImageSubresourceRange()
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseMipLevel(0)
                .setLevelCount(1)
                .setBaseArrayLayer(0)
                .setLayerCount(1);

        for (u64 i = 0; i < images_.size(); ++i) {
            const auto create_info = vk::ImageViewCreateInfo()
                    .setImage(images_[i])
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(format_)
                    .setComponents(component_mappings)
                    .setSubresourceRange(subresource_range);

            image_views_[i] = device_.createImageView(create_info);
        }
    }

    vk::Format Swapchain::get_format() const {
        return format_;
    }
}