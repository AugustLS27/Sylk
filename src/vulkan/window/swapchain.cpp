//
// Created by August Silva on 7-3-23.
//

#include <sylk/vulkan/window/swapchain.hpp>
#include <sylk/vulkan/utils/queue_family_indices.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/core/utils/all.hpp>

#include <GLFW/glfw3.h>

#include <limits>
#include <algorithm>

constexpr sylk::u32 U32_LIMIT = std::numeric_limits<sylk::u32>::max();

namespace sylk {
    void Swapchain::create(vk::PhysicalDevice physical_device, GLFWwindow* window, vk::SurfaceKHR surface) {
        log(TRACE, "Creating swapchain...");

        const auto queue_family_indices = QueueFamilyIndices::find(physical_device, surface);
        graphics_queue_family_index_ = queue_family_indices.graphics.value();
        std::vector queue_families {graphics_queue_family_index_, queue_family_indices.presentation.value()};
        const bool queues_equal = queue_family_indices.graphics == queue_family_indices.presentation;

        // exclusive mode does not require specified queue families
        if (!queues_equal) {
            queue_families.clear();
        }
        const auto sharing_mode = (queues_equal ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent);

        const auto support_details = query_device_support_details(physical_device, surface);
        const auto surface_format = select_surface_format(support_details.surface_formats);
        const auto present_mode = select_present_mode(support_details.present_modes);

        extent_ = select_extent_2d(support_details.surface_capabilities, window);
        format_ = surface_format.format;

        const auto max_image_count = support_details.surface_capabilities.maxImageCount;
        const auto min_image_count = support_details.surface_capabilities.minImageCount + 1;
        const bool has_max_images = max_image_count > 0;

        auto swapchain_img_count = (has_max_images && min_image_count > max_image_count ?
                                    max_image_count : min_image_count);

        auto create_info = vk::SwapchainCreateInfoKHR()
                .setSurface(surface)
                .setMinImageCount(swapchain_img_count)
                .setImageFormat(format_)
                .setImageColorSpace(surface_format.colorSpace)
                .setImageExtent(extent_)
                .setImageArrayLayers(1)
                .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                .setImageSharingMode(sharing_mode)
                .setQueueFamilyIndices(queue_families)
                .setPreTransform(support_details.surface_capabilities.currentTransform)
                .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                .setPresentMode(present_mode)
                .setClipped(true)
                .setOldSwapchain(nullptr);

        swapchain_ = device_.createSwapchainKHR(create_info, nullptr);
        images_ = device_.getSwapchainImagesKHR(swapchain_);

        create_image_views();
        create_renderpass();
        graphics_pipeline_.create(extent_, renderpass_);
        create_framebuffers();
        create_command_pool();
        create_command_buffer();
        create_synchronizers();

        log(DEBUG, "Created swapchain");
    }

    void Swapchain::destroy() {
        device_.destroyCommandPool(command_pool_);
        log(TRACE, "Destroyed command pool");

        graphics_pipeline_.destroy();

        device_.destroyRenderPass(renderpass_);
        log(TRACE, "Destroyed renderpass");

        device_.destroySemaphore(sema_img_available_);
        device_.destroySemaphore(sema_render_finished_);
        device_.destroyFence(fence_in_flight_);
        log(TRACE, "Destroyed synchronization objects");

        for (auto framebuffer : frame_buffers_) {
            device_.destroyFramebuffer(framebuffer);
        }
        log(TRACE, "Destroyed framebuffers");

        for (auto view : image_views_) {
            device_.destroyImageView(view);
        }
        log(TRACE, "Destroyed image views");

        device_.destroySwapchainKHR(swapchain_);
        log(TRACE, "Destroyed swapchain");

    }

    Swapchain::SupportDetails Swapchain::query_device_support_details(vk::PhysicalDevice device, vk::SurfaceKHR surface) const {
        log(TRACE, "Querying swapchain support details...");

        SupportDetails details;

        details.surface_capabilities = device.getSurfaceCapabilitiesKHR(surface);
        details.surface_formats = device.getSurfaceFormatsKHR(surface);
        details.present_modes = device.getSurfacePresentModesKHR(surface);

        return details;
    }

    vk::SurfaceFormatKHR Swapchain::select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const {
        log(TRACE, "Selecting swapchain surface format...");

        for (const auto& format : available_formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }

        return available_formats.front();
    }

    vk::PresentModeKHR Swapchain::select_present_mode(const std::vector<vk::PresentModeKHR>& available_modes) const {
        log(TRACE, "Selecting swapchain present mode...");

        for (const auto& mode : available_modes) {
            if (mode == vk::PresentModeKHR::eMailbox) {
                return mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D Swapchain::select_extent_2d(const vk::SurfaceCapabilitiesKHR capabilities, GLFWwindow* window) const {
        log(TRACE, "Selecting swapchain extent...");

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

        log(TRACE, "Created swapchain image views");
    }

    void Swapchain::create_framebuffers() {
        frame_buffers_.resize(image_views_.size());

        for (u64 i = 0; i < image_views_.size(); ++i) {
            const auto framebuffer_info = vk::FramebufferCreateInfo()
                    .setRenderPass(renderpass_)
                    .setAttachments(image_views_[i])
                    .setWidth(extent_.width)
                    .setHeight(extent_.height)
                    .setLayers(1);

            frame_buffers_[i] = device_.createFramebuffer(framebuffer_info);
        }

        log(TRACE, "Created framebuffers");
    }

    void Swapchain::create_synchronizers() {
        sema_img_available_ = device_.createSemaphore(vk::SemaphoreCreateInfo());
        sema_render_finished_ = device_.createSemaphore(vk::SemaphoreCreateInfo());
        fence_in_flight_ = device_.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));

        log(TRACE, "Created synchronizer objects");
    }

    void Swapchain::draw_next() {
        handle_result(device_.waitForFences(fence_in_flight_, true, UINT64_MAX), "Vulkan Fence error");
        device_.resetFences(fence_in_flight_);

        const auto [result, img_index] = device_.acquireNextImageKHR(swapchain_, UINT64_MAX, sema_img_available_);
        handle_result(result, "Image acquisition failed");

        command_buffers_[0].reset();
        record_command_buffer(command_buffers_[0], img_index);

        const vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        const auto submit_info = vk::SubmitInfo()
                .setWaitSemaphores(sema_img_available_)
                .setWaitDstStageMask(wait_stage)
                .setSignalSemaphores(sema_render_finished_)
                .setCommandBuffers(command_buffers_[0]);

        graphics_queue_.submit(submit_info, fence_in_flight_);

        const auto present_info = vk::PresentInfoKHR()
                .setWaitSemaphores(sema_render_finished_)
                .setSwapchains(swapchain_)
                .setImageIndices(img_index);

        handle_result(presentation_queue_.presentKHR(present_info), "Failed to present image");
    }

    void Swapchain::record_command_buffer(const vk::CommandBuffer buffer, const u32 image_index) {
        const auto buffer_begin_info = vk::CommandBufferBeginInfo();
        buffer.begin(buffer_begin_info);

        const auto render_area = vk::Rect2D().setExtent(extent_);
        const std::array clear_value = {
                0.0f, 0.0f, 0.0f, 1.0f
        };
        const auto clear_color = vk::ClearValue(clear_value);

        const auto renderpass_begin_info = vk::RenderPassBeginInfo()
                .setRenderPass(renderpass_)
                .setFramebuffer(frame_buffers_[image_index])
                .setRenderArea(render_area)
                .setClearValues(clear_color);

        const auto pipeline = graphics_pipeline_.get();
        buffer.beginRenderPass(renderpass_begin_info, vk::SubpassContents::eInline);
        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

        const auto viewport = vk::Viewport()
                .setHeight(cast<f32>(extent_.height))
                .setWidth(cast<f32>(extent_.width))
                .setMaxDepth(1.0f);
        buffer.setViewport(0, viewport);

        const auto scissor = vk::Rect2D().setExtent(extent_);
        buffer.setScissor(0, scissor);

        buffer.draw(3, 1, 0, 0);

        buffer.endRenderPass();
        buffer.end();
    }

    Swapchain::Swapchain(const vk::Device& device)
        : device_(device)
        , graphics_pipeline_(device)
        {}

    void Swapchain::create_command_pool() {
        const auto pool_info = vk::CommandPoolCreateInfo()
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(graphics_queue_family_index_);

        command_pool_ = device_.createCommandPool(pool_info);
        log(TRACE, "Created command pool");
    }

    void Swapchain::create_command_buffer() {
        const auto buffer_alloc_info = vk::CommandBufferAllocateInfo()
                .setCommandPool(command_pool_)
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(1);

        command_buffers_ = device_.allocateCommandBuffers(buffer_alloc_info);

        log(TRACE, "Created command buffer");
    }

    void Swapchain::create_renderpass() {
        const auto color_attachment = vk::AttachmentDescription()
                .setFormat(format_)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(vk::AttachmentLoadOp::eClear)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

        const auto color_attachment_ref = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);

        const auto subpass = vk::SubpassDescription()
                .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                .setColorAttachments(color_attachment_ref);

        const auto subpass_dependency = vk::SubpassDependency()
                .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                .setSrcAccessMask(vk::AccessFlagBits::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        const auto render_pass_info = vk::RenderPassCreateInfo()
                .setAttachments(color_attachment)
                .setSubpasses(subpass)
                .setDependencies(subpass_dependency);

        renderpass_ = device_.createRenderPass(render_pass_info);

        log(TRACE, "Created render pass");
    }

    void Swapchain::set_queues(vk::Queue graphics, vk::Queue present) {
        graphics_queue_ = graphics;
        presentation_queue_ = present;
    }
}