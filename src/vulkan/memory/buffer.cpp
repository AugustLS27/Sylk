//
// Created by August Silva on 12-3-23.
//

#include <sylk/vulkan/memory/buffer.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/core/utils/cast.hpp>

namespace sylk {
    void Buffer::create(const CreateData data) {
        const auto buffer_info = vk::BufferCreateInfo {
            .size = data.buffer_size,
            .usage = data.buffer_usage_flags,
            .sharingMode = vk::SharingMode::eExclusive,
        };

        const auto [buffer_result, buffer] = data.device.createBuffer(buffer_info);
        handle_result(buffer_result, "Failed to create buffer");
        buffer_ = buffer;

        const auto mem_reqs = data.device.getBufferMemoryRequirements(buffer_);

        const auto alloc_info = vk::MemoryAllocateInfo {
            .allocationSize = mem_reqs.size,
            .memoryTypeIndex = find_memtype(data.physical_device, mem_reqs.memoryTypeBits, data.property_flags),
        };

        const auto [alloc_result, mem] = data.device.allocateMemory(alloc_info);
        handle_result(alloc_result, "Failed to allocate device memory");
        buffer_memory_ = mem;

        handle_result(data.device.bindBufferMemory(buffer_, buffer_memory_, 0),
                      "Failed to bind buffer memory");

        // only map memory if there is data to map
        if (data.data_to_map) {
            const auto [mmap_result, mapped_data] = data.device.mapMemory(buffer_memory_, 0, data.buffer_size);
            handle_result(mmap_result, "Failed to map buffer to device memory");
            std::memcpy(mapped_data, data.data_to_map, cast<size_t>(data.buffer_size));

            data.device.unmapMemory(buffer_memory_);
        }
    }

    auto Buffer::find_memtype(vk::PhysicalDevice physical_device, u32 type_filter, vk::MemoryPropertyFlags properties) -> u32 {
        const auto mem_properties = physical_device.getMemoryProperties();

        for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i) {
            if ((type_filter & (1 << i))
                && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable vertex buffer memory type on this device");
    }

    auto Buffer::get_vkbuffer() const -> vk::Buffer {
        return buffer_;
    }

    auto Buffer::get_memory_handle() const -> vk::DeviceMemory {
        return buffer_memory_;
    }

    void Buffer::copy_onto(Buffer::CopyData data) const {
        const auto cmd_buffer_alloc_info = vk::CommandBufferAllocateInfo {
                .commandPool        = data.pool,
                .level              = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1,
        };

        const auto [cmd_buffer_result, cmd_buffers] = data.device.allocateCommandBuffers(cmd_buffer_alloc_info);
        handle_result(cmd_buffer_result, "Failed to allocate command buffer");

        const auto cmd_buffer_begin_info = vk::CommandBufferBeginInfo {
                .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
        };

        const auto cmd_buffer = cmd_buffers[0];
        handle_result(cmd_buffer.begin(cmd_buffer_begin_info), "Failed to begin recording command buffer");

        const auto copy_region = vk::BufferCopy {
                .size = data.size,
        };

        cmd_buffer.copyBuffer(buffer_, data.target, copy_region);

        handle_result(cmd_buffer.end(), "Failed to stop recording command buffer");

        const auto submit_info = vk::SubmitInfo().setCommandBuffers(cmd_buffer);

        handle_result(data.queue.submit(submit_info), "Failed to submit to graphics queue");
        handle_result(data.queue.waitIdle(), "Device error");

        data.device.freeCommandBuffers(data.pool, cmd_buffer);
    }

    void Buffer::destroy_with(vk::Device device) {
        device.destroyBuffer(buffer_);
        device.freeMemory(buffer_memory_);
    }


} // sylk