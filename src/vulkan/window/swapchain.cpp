//
// Created by August Silva on 7-3-23.
//

#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/shader/uniformbuffer.hpp>
#include <sylk/vulkan/utils/queue_family_indices.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/vulkan/vulkan.hpp>
#include <sylk/vulkan/window/swapchain.hpp>

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <limits>

constexpr sylk::u32 U32_LIMIT            = std::numeric_limits<sylk::u32>::max();
constexpr sylk::i32 MAX_FRAMES_IN_FLIGHT = 3;

namespace sylk {
    Swapchain::Swapchain(const vk::Device& device)
        : current_frame_(0)
        , device_(device)
        , graphics_pipeline_(device)
        , command_buffers_(MAX_FRAMES_IN_FLIGHT)
        , semaphores_img_available_(MAX_FRAMES_IN_FLIGHT)
        , semaphores_render_finished_(MAX_FRAMES_IN_FLIGHT)
        , fences_in_flight_(MAX_FRAMES_IN_FLIGHT)
        , uniform_buffers_(MAX_FRAMES_IN_FLIGHT)
        , descriptor_sets_(MAX_FRAMES_IN_FLIGHT) {}

    void Swapchain::create(const vk::PhysicalDevice physical_device, GLFWwindow* window, const vk::SurfaceKHR surface) {
        log(ELogLvl::TRACE, "Creating swapchain...");

        physical_device_ = physical_device;
        window_          = window;
        surface_         = surface;

        setup_swapchain();
        create_image_views();
        create_renderpass();
        graphics_pipeline_.create(extent_, renderpass_);
        create_framebuffers();
        create_command_pool();
        create_staged_buffer(vertex_buffer_, vk::BufferUsageFlagBits::eVertexBuffer, vertices_);
        create_staged_buffer(index_buffer_, vk::BufferUsageFlagBits::eIndexBuffer, indices_);
        create_uniform_buffers();
        create_descriptor_pool();
        create_descriptor_sets();
        create_command_buffer();
        create_synchronizers();

        log(ELogLvl::DEBUG, "Created swapchain");
    }

    void Swapchain::destroy() {
        device_.destroyCommandPool(command_pool_);
        log(ELogLvl::TRACE, "Destroyed command pool");

        vertex_buffer_.destroy_with(device_);
        log(ELogLvl::TRACE, "Destroyed vertex buffer");

        index_buffer_.destroy_with(device_);
        log(ELogLvl::TRACE, "Destroyed index buffer");

        for (auto& buffer : uniform_buffers_) {
            buffer.destroy_with(device_);
        }
        log(ELogLvl::TRACE, "Destroyed uniform buffers");

        device_.destroyDescriptorPool(descriptor_pool_);
        log(ELogLvl::TRACE, "Destroyed descriptor pool");

        graphics_pipeline_.destroy();
        graphics_pipeline_.destroy_descriptorset_layouts();

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

    auto
    Swapchain::select_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats) const -> vk::SurfaceFormatKHR {
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

        vk::Extent2D actual_extent {cast<u32>(window_width), cast<u32>(window_height)};

        actual_extent.width  = std::clamp(actual_extent.width,
                                         capabilities.minImageExtent.width,
                                         capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height,
                                          capabilities.minImageExtent.height,
                                          capabilities.maxImageExtent.height);

        return actual_extent;
    }

    void Swapchain::create_image_views() {
        image_views_.resize(images_.size());

        const auto component_mappings = vk::ComponentMapping {.r = vk::ComponentSwizzle::eIdentity,
                                                              .g = vk::ComponentSwizzle::eIdentity,
                                                              .b = vk::ComponentSwizzle::eIdentity,
                                                              .a = vk::ComponentSwizzle::eIdentity};

        const auto subresource_range = vk::ImageSubresourceRange {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };

        for (u64 i = 0; i < images_.size(); ++i) {
            const auto create_info = vk::ImageViewCreateInfo {
                .image            = images_[i],
                .viewType         = vk::ImageViewType::e2D,
                .format           = format_,
                .components       = component_mappings,
                .subresourceRange = subresource_range,
            };

            const auto [result, view] = device_.createImageView(create_info);
            handle_result(result, "Failed to create image view");
            image_views_[i] = view;
        }

        log(ELogLvl::TRACE, "Created swapchain image views");
    }

    void Swapchain::create_framebuffers() {
        frame_buffers_.resize(image_views_.size());

        for (u64 i = 0; i < image_views_.size(); ++i) {
            const auto framebuffer_info =
                vk::FramebufferCreateInfo {
                    .renderPass = renderpass_,
                    .width      = extent_.width,
                    .height     = extent_.height,
                    .layers     = 1,
                }
                    .setAttachments(image_views_[i]);

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

            const auto [fence_result,
                        fence] = device_.createFence(vk::FenceCreateInfo {.flags = vk::FenceCreateFlagBits::eSignaled});
            handle_result(fence_result, "Failed to create fence");

            semaphores_img_available_[i]   = sema_a;
            semaphores_render_finished_[i] = sema_b;
            fences_in_flight_[i]           = fence;
        }

        log(ELogLvl::TRACE, "Created synchronizer objects");
    }

    void Swapchain::draw_next() {
        handle_result(device_.waitForFences(fences_in_flight_[current_frame_], true, UINT64_MAX), "Vulkan fence error");

        const auto [result,
                    img_index] = device_.acquireNextImageKHR(swapchain_, UINT64_MAX, semaphores_img_available_[current_frame_]);
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

        update_uniform_buffers();

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

        const auto clear_color = vk::ClearValue {.color = {std::array {0.0f, 0.0f, 0.0f, 1.0f}}};
        const auto renderpass_begin_info =
            vk::RenderPassBeginInfo {
                .renderPass  = renderpass_,
                .framebuffer = frame_buffers_[image_index],
                .renderArea  = vk::Rect2D {.extent = extent_},

            }
                .setClearValues(clear_color);

        buffer.beginRenderPass(renderpass_begin_info, vk::SubpassContents::eInline);
        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline_.get_handle());

        buffer.bindVertexBuffers(0, vertex_buffer_.vk_buffer(), vk::DeviceSize {0});
        buffer.bindIndexBuffer(index_buffer_.vk_buffer(), vk::DeviceSize {0}, vk::IndexType::eUint16);

        buffer.setViewport(0,
                           vk::Viewport {
                               .width    = cast<f32>(extent_.width),
                               .height   = cast<f32>(extent_.height),
                               .maxDepth = 1.0f,
                           });

        buffer.setScissor(0, vk::Rect2D {.extent = extent_});

        buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                  graphics_pipeline_.get_layout(),
                                  0,
                                  descriptor_sets_[current_frame_],
                                  nullptr);

        buffer.drawIndexed(cast<u32>(indices_.size()), 1, 0, 0, 0);

        buffer.endRenderPass();
        handle_result(buffer.end(), "Failed to finish recording command buffer");
    }

    void Swapchain::create_command_pool() {
        const auto pool_info = vk::CommandPoolCreateInfo {
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = graphics_queue_family_index_,
        };

        const auto [result, pool] = device_.createCommandPool(pool_info);
        handle_result(result, "Failed to create command pool");
        command_pool_ = pool;

        log(ELogLvl::TRACE, "Created command pool");
    }

    void Swapchain::create_command_buffer() {
        const auto buffer_alloc_info = vk::CommandBufferAllocateInfo {
            .commandPool        = command_pool_,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = cast<u32>(command_buffers_.size()),
        };

        const auto [result, buffer] = device_.allocateCommandBuffers(buffer_alloc_info);
        handle_result(result, "Failed to allocate command buffer");
        command_buffers_ = buffer;

        log(ELogLvl::TRACE, "Created command buffer");
    }

    void Swapchain::create_renderpass() {
        const auto color_attachment = vk::AttachmentDescription {
            .format         = format_,
            .samples        = vk::SampleCountFlagBits::e1,
            .loadOp         = vk::AttachmentLoadOp::eClear,
            .storeOp        = vk::AttachmentStoreOp::eStore,
            .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
            .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
            .initialLayout  = vk::ImageLayout::eUndefined,
            .finalLayout    = vk::ImageLayout::ePresentSrcKHR,
        };

        const auto color_attachment_ref = vk::AttachmentReference {
            .attachment = 0,
            .layout     = vk::ImageLayout::eColorAttachmentOptimal,
        };

        const auto subpass =
            vk::SubpassDescription {
                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
            }
                .setColorAttachments(color_attachment_ref);

        const auto subpass_dependency = vk::SubpassDependency {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        };

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
        graphics_queue_     = graphics;
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
        const auto support_details = query_device_support_details(physical_device_, surface_);
        const auto surface_format  = select_surface_format(support_details.surface_formats);

        format_ = surface_format.format;
        extent_ = select_extent_2d(support_details.surface_capabilities, window_);

        const auto max_image_count = support_details.surface_capabilities.maxImageCount;
        const auto min_image_count = support_details.surface_capabilities.minImageCount + 1;

        const auto swapchain_img_count = (max_image_count > 0 && min_image_count > max_image_count ? max_image_count
                                                                                                   : min_image_count);

        const auto queue_indices     = QueueFamilyIndices::find(physical_device_, surface_);
        graphics_queue_family_index_ = queue_indices.graphics.value();
        std::vector active_queues {graphics_queue_family_index_, queue_indices.presentation.value()};
        const bool  queues_equal = queue_indices.graphics == queue_indices.presentation;

        // exclusive mode does not require specified queue families
        if (!queues_equal) {
            active_queues.clear();
        }

        auto create_info =
            vk::SwapchainCreateInfoKHR {
                .surface          = surface_,
                .minImageCount    = swapchain_img_count,
                .imageFormat      = format_,
                .imageColorSpace  = surface_format.colorSpace,
                .imageExtent      = extent_,
                .imageArrayLayers = 1,
                .imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
                .imageSharingMode = (queues_equal ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent),
                .preTransform     = support_details.surface_capabilities.currentTransform,
                .compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
                .presentMode      = select_present_mode(support_details.present_modes),
                .clipped          = true,
                .oldSwapchain     = nullptr,
            }
                .setQueueFamilyIndices(active_queues);

        const auto [swapc_result, swapchain] = device_.createSwapchainKHR(create_info, nullptr);
        handle_result(swapc_result, "Failed to create swapchain");
        swapchain_ = swapchain;

        const auto [img_result, images] = device_.getSwapchainImagesKHR(swapchain_);
        handle_result(img_result, "Failed to acquire swapchain images");
        images_ = images;
    }

    void Swapchain::create_uniform_buffers() {
        vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

        const auto buffer_data = Buffer::CreateData {
            .data_to_map        = nullptr,
            .persistent_mapping = true,
            .device             = device_,
            .physical_device    = physical_device_,
            .buffer_size        = buffer_size,
            .buffer_usage_flags = vk::BufferUsageFlagBits::eUniformBuffer,
            .property_flags     = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        };

        for (auto& buffer : uniform_buffers_) {
            buffer.create(buffer_data);
        }
    }

    void Swapchain::update_uniform_buffers() {
        namespace clock = std::chrono;

        static const auto start_time = clock::high_resolution_clock::now();
        static f32        inc        = 0.0f;
        static i32        seconds    = 0;

        const auto current_time = clock::high_resolution_clock::now();
        const f32  elapsed_time = clock::duration<f32, clock::seconds::period>(current_time - start_time).count();

        auto ubo = UniformBufferObject {
            .model = glm::rotate(glm::mat4(1.0f), elapsed_time * glm::radians(inc), glm::vec3(0.0f, 0.0f, 1.0f)),
            //            .view       = glm::lookAt(glm::vec3(2.0f, inc, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f,
            //            0.0f)), .projection = glm::perspective(glm::radians(45.0f), cast<f32>(extent_.width / extent_.height),
            //            0.1f, 10.0f),
        };

        static f32 incval = 0.1f;
        if (inc > 360.f || inc < -360.0f) {
            incval *= -1.f;
        }
        inc += incval;
        if (std::trunc(elapsed_time) > seconds) {
            seconds = cast<i32>(elapsed_time);
            log(ELogLvl::DEBUG, "inc: {}", inc);
            log(ELogLvl::INFO, "elapsed: {}", elapsed_time);
        }

        // invert y axis since glm was designed for OGL and VK isn't weird
        ubo.projection[1][1] *= -1;

        uniform_buffers_[current_frame_].pass_data(&ubo, sizeof(ubo));
    }

    void Swapchain::create_descriptor_pool() {
        const auto num_buffers = cast<u32>(uniform_buffers_.size());
        const auto pool_size   = vk::DescriptorPoolSize {
              .descriptorCount = num_buffers,
        };

        const auto pool_info =
            vk::DescriptorPoolCreateInfo {
                .maxSets = num_buffers,
            }
                .setPoolSizes(pool_size);

        const auto [result, pool] = device_.createDescriptorPool(pool_info);
        handle_result(result, "Failed to create descriptor pool");
        descriptor_pool_ = pool;
    }

    void Swapchain::create_descriptor_sets() {
        std::vector<vk::DescriptorSetLayout> set_layouts(MAX_FRAMES_IN_FLIGHT, graphics_pipeline_.get_descriptor_set_layout());

        const auto alloc_info = vk::DescriptorSetAllocateInfo {.descriptorPool = descriptor_pool_}.setSetLayouts(set_layouts);

        const auto [result, sets] = device_.allocateDescriptorSets(alloc_info);
        handle_result(result, "Failed to allocate descriptor sets");
        descriptor_sets_ = sets;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            const auto buffer_info = vk::DescriptorBufferInfo {
                .buffer = uniform_buffers_[i].vk_buffer(),
                .offset = 0,
                .range  = sizeof(UniformBufferObject),
            };

            const auto write_descript_set = vk::WriteDescriptorSet {
                .dstSet          = descriptor_sets_[i],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType  = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo     = &buffer_info,
            };

            device_.updateDescriptorSets(write_descript_set, nullptr);
        }
    }

    template<typename T>
    void Swapchain::create_staged_buffer(Buffer& buffer, vk::BufferUsageFlags buffer_type, const std::vector<T>& data) {
        const vk::DeviceSize buffer_size = sizeof(data[0]) * data.size();

        Buffer     staging_buffer;
        const auto staging_buffer_data = Buffer::CreateData {
            .data_to_map        = data.data(),
            .device             = device_,
            .physical_device    = physical_device_,
            .buffer_size        = buffer_size,
            .buffer_usage_flags = vk::BufferUsageFlagBits::eTransferSrc,
            .property_flags     = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible,
        };

        staging_buffer.create(staging_buffer_data);

        const auto buffer_data = Buffer::CreateData {
            .data_to_map        = nullptr,
            .device             = device_,
            .physical_device    = physical_device_,
            .buffer_size        = buffer_size,
            .buffer_usage_flags = buffer_type | vk::BufferUsageFlagBits::eTransferDst,
            .property_flags     = vk::MemoryPropertyFlagBits::eDeviceLocal,
        };

        buffer.create(buffer_data);

        staging_buffer.copy_onto({
            .target = buffer.vk_buffer(),
            .size   = buffer_size,
            .pool   = command_pool_,
            .device = device_,
            .queue  = graphics_queue_,
        });

        staging_buffer.destroy_with(device_);
    }
}  // namespace sylk