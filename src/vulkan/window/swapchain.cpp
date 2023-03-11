//
// Created by August Silva on 7-3-23.
//

#include <sylk/vulkan/window/swapchain.hpp>
#include <sylk/vulkan/utils/queue_family_indices.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>

#include <limits>
#include <algorithm>

constexpr sylk::u32 U32_LIMIT = std::numeric_limits<sylk::u32>::max();
constexpr sylk::i32 MAX_FRAMES_IN_FLIGHT = 2;

namespace sylk {
    Swapchain::Swapchain(const vk::Device& device)
            : device_(device)
            , graphics_pipeline_(device)
            , command_buffers_(MAX_FRAMES_IN_FLIGHT)
            , semaphores_img_available_(MAX_FRAMES_IN_FLIGHT)
            , semaphores_render_finished_(MAX_FRAMES_IN_FLIGHT)
            , fences_in_flight_(MAX_FRAMES_IN_FLIGHT)
            , current_frame_(0)
    {}

    void Swapchain::create(const vk::PhysicalDevice physical_device, GLFWwindow* window, const vk::SurfaceKHR surface) {
        log(ELogLvl::TRACE, "Creating swapchain...");

        physical_device_ = physical_device;
        window_ = window;
        surface_ = surface;

        setup_swapchain();
        create_image_views();
        create_renderpass();
        graphics_pipeline_.create(extent_, renderpass_);
        create_framebuffers();
        create_command_pool();
        create_command_buffer();
        create_synchronizers();

        log(ELogLvl::DEBUG, "Created swapchain");
    }

    void Swapchain::destroy() {
        device_.destroyCommandPool(command_pool_);
        log(ELogLvl::TRACE, "Destroyed command pool");

        graphics_pipeline_.destroy();

        device_.destroyRenderPass(renderpass_);
        log(ELogLvl::TRACE, "Destroyed renderpass");

        for (u64 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            device_.destroySemaphore(semaphores_img_available_[i]);
            device_.destroySemaphore(semaphores_render_finished_[i]);
            device_.destroyFence(fences_in_flight_[i]);
        }
        log(ELogLvl::TRACE, "Destroyed synchronization objects");

        destroy_partial();
    }

    auto Swapchain::query_device_support_details(const vk::PhysicalDevice device, const vk::SurfaceKHR surface) const
    -> Swapchain::SupportDetails {
        log(ELogLvl::TRACE, "Querying swapchain support details...");

        SupportDetails details;

        const auto [cap_result, capabilities] = device.getSurfaceCapabilitiesKHR(surface);
        handle_result(cap_result, "Failed to acquire surface capabilities");
        details.surface_capabilities = capabilities;

        const auto [format_result, formats] = device.getSurfaceFormatsKHR(surface);
        handle_result(format_result, "Failed to acquire surface formats");
        details.surface_formats = formats;

        const auto [mode_result, modes] = device.getSurfacePresentModesKHR(surface);
        handle_result(mode_result, "Failed to acquire surface present modes");
        details.present_modes = modes;

        return details;
    }

    auto Swapchain::select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const -> vk::SurfaceFormatKHR {
        log(ELogLvl::TRACE, "Selecting swapchain surface format...");

        for (const auto& format : available_formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }

        return available_formats.front();
    }

    auto Swapchain::select_present_mode(const std::vector<vk::PresentModeKHR>& available_modes) const -> vk::PresentModeKHR {
        log(ELogLvl::TRACE, "Selecting swapchain present mode...");

        for (const auto& mode : available_modes) {
            if (mode == vk::PresentModeKHR::eImmediate) {
                return mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    auto Swapchain::select_extent_2d(const vk::SurfaceCapabilitiesKHR capabilities, GLFWwindow* window) const -> vk::Extent2D {
        log(ELogLvl::TRACE, "Selecting swapchain extent...");

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

            const auto [result, view] = device_.createImageView(create_info);
            handle_result(result, "Failed to create image view");
            image_views_[i] = view;
        }

        log(ELogLvl::TRACE, "Created swapchain image views");
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

            const auto [result, buffer] = device_.createFramebuffer(framebuffer_info);
            handle_result(result, "Failed to create framebuffer");
            frame_buffers_[i] = buffer;
        }

        log(ELogLvl::TRACE, "Created framebuffers");
    }

    void Swapchain::create_synchronizers() {
        for (u64 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            const auto [sema_result_a, sema_a] = device_.createSemaphore(vk::SemaphoreCreateInfo());
            handle_result(sema_result_a, "Failed to create semaphore");

            const auto [sema_result_b, sema_b] = device_.createSemaphore(vk::SemaphoreCreateInfo());
            handle_result(sema_result_b, "Failed to create semaphore");

            const auto [fence_result, fence] = device_.createFence(vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled));
            handle_result(fence_result, "Failed to create fence");

            semaphores_img_available_[i] = sema_a;
            semaphores_render_finished_[i] = sema_b;
            fences_in_flight_[i] = fence;
        }

        log(ELogLvl::TRACE, "Created synchronizer objects");
    }

    void Swapchain::draw_next() {
        handle_result(device_.waitForFences(fences_in_flight_[current_frame_], true, UINT64_MAX), "Vulkan fence error");

        const auto [result, img_index] = device_.acquireNextImageKHR(swapchain_, UINT64_MAX, semaphores_img_available_[current_frame_]);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreate();
            return;
        } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            handle_result(result, "Image acquisition failed");
        }

        device_.resetFences(fences_in_flight_[current_frame_]);
        command_buffers_[current_frame_].reset();
        record_command_buffer(command_buffers_[current_frame_], img_index);

        const vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        const auto submit_info = vk::SubmitInfo()
                .setWaitSemaphores(semaphores_img_available_[current_frame_])
                .setWaitDstStageMask(wait_stage)
                .setSignalSemaphores(semaphores_render_finished_[current_frame_])
                .setCommandBuffers(command_buffers_[current_frame_]);

        handle_result(graphics_queue_.submit(submit_info, fences_in_flight_[current_frame_]),
                      "Failed to submit to graphics queue");

        const auto present_info = vk::PresentInfoKHR()
                .setWaitSemaphores(semaphores_render_finished_[current_frame_])
                .setSwapchains(swapchain_)
                .setImageIndices(img_index);

        vk::Result present_result = presentation_queue_.presentKHR(present_info);
        if (present_result == vk::Result::eErrorOutOfDateKHR || present_result == vk::Result::eSuboptimalKHR) {
            recreate();
        } else {
            handle_result(present_result, "Failed to present image");
        }

        current_frame_ = ++current_frame_ % MAX_FRAMES_IN_FLIGHT;
    }

    void Swapchain::record_command_buffer(const vk::CommandBuffer buffer, const u32 image_index) {
        const auto buffer_begin_info = vk::CommandBufferBeginInfo();
        handle_result(buffer.begin(buffer_begin_info), "Failed to start recording command buffer");

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

        const auto pipeline = graphics_pipeline_.get_handle();
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
        handle_result(buffer.end(), "Failed to finish recording command buffer");
    }

    void Swapchain::create_command_pool() {
        const auto pool_info = vk::CommandPoolCreateInfo()
                .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
                .setQueueFamilyIndex(graphics_queue_family_index_);

        const auto [result, pool] = device_.createCommandPool(pool_info);
        handle_result(result, "Failed to create command pool");
        command_pool_ = pool;

        log(ELogLvl::TRACE, "Created command pool");
    }

    void Swapchain::create_command_buffer() {
        const auto buffer_alloc_info = vk::CommandBufferAllocateInfo()
                .setCommandPool(command_pool_)
                .setLevel(vk::CommandBufferLevel::ePrimary)
                .setCommandBufferCount(cast<u32>(command_buffers_.size()));

        const auto [result, buffer] = device_.allocateCommandBuffers(buffer_alloc_info);
        handle_result(result, "Failed to allocate command buffer");
        command_buffers_ = buffer;

        log(ELogLvl::TRACE, "Created command buffer");
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

        const auto [result, renderpass] = device_.createRenderPass(render_pass_info);
        handle_result(result, "Failed to create renderpass");
        renderpass_ = renderpass;

        log(ELogLvl::TRACE, "Created render pass");
    }

    void Swapchain::set_queues(const vk::Queue graphics, const vk::Queue present) {
        graphics_queue_ = graphics;
        presentation_queue_ = present;
    }

    void Swapchain::destroy_partial() {
        for (auto framebuffer : frame_buffers_) {
            device_.destroyFramebuffer(framebuffer);
        }
        log(ELogLvl::TRACE, "Destroyed framebuffers");

        for (auto view : image_views_) {
            device_.destroyImageView(view);
        }
        log(ELogLvl::TRACE, "Destroyed image views");

        device_.destroySwapchainKHR(swapchain_);
        log(ELogLvl::TRACE, "Destroyed swapchain");
    }

    void Swapchain::recreate() {
        handle_result(device_.waitIdle(), "Device error");

        destroy_partial();

        setup_swapchain();
        create_image_views();
        create_framebuffers();

        log(ELogLvl::TRACE, "Re-created swapchain");
    }

    void Swapchain::setup_swapchain() {
        const auto queue_family_indices = QueueFamilyIndices::find(physical_device_, surface_);
        graphics_queue_family_index_ = queue_family_indices.graphics.value();
        std::vector queue_families {graphics_queue_family_index_, queue_family_indices.presentation.value()};
        const bool queues_equal = queue_family_indices.graphics == queue_family_indices.presentation;

        // exclusive mode does not require specified queue families
        if (!queues_equal) {
            queue_families.clear();
        }
        const auto sharing_mode = (queues_equal ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent);

        const auto support_details = query_device_support_details(physical_device_, surface_);
        const auto surface_format = select_surface_format(support_details.surface_formats);
        const auto present_mode = select_present_mode(support_details.present_modes);

        extent_ = select_extent_2d(support_details.surface_capabilities, window_);
        format_ = surface_format.format;

        const auto max_image_count = support_details.surface_capabilities.maxImageCount;
        const auto min_image_count = support_details.surface_capabilities.minImageCount + 1;
        const bool has_max_images = max_image_count > 0;

        const auto swapchain_img_count = (has_max_images && min_image_count > max_image_count ?
                                    max_image_count : min_image_count);

        auto create_info = vk::SwapchainCreateInfoKHR()
                .setSurface(surface_)
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

        const auto [swapc_result, swapchain] = device_.createSwapchainKHR(create_info, nullptr);
        handle_result(swapc_result, "Failed to create swapchain");
        swapchain_ = swapchain;

        const auto [img_result, images] = device_.getSwapchainImagesKHR(swapchain_);
        handle_result(img_result, "Failed to acquire swapchain images");
        images_ = images;
    }
}