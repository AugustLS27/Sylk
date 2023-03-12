//
// Created by August Silva on 12-3-23.
//

#ifndef SYLK_VULKAN_MEMORY_BUFFER_HPP
#define SYLK_VULKAN_MEMORY_BUFFER_HPP

#include <sylk/vulkan/vulkan.hpp>
#include <sylk/core/utils/rust_style_types.hpp>

namespace sylk {

    class Buffer {
    public:
        struct CreateData {
            const void* data_to_map;
            vk::Device device;
            vk::PhysicalDevice physical_device;
            vk::DeviceSize buffer_size;
            vk::BufferUsageFlags buffer_usage_flags;
            vk::MemoryPropertyFlags property_flags;
        };
    public:
        void create(CreateData data);
        void destroy();

        ~Buffer();

        auto get() const -> vk::Buffer;

    private:
        auto find_memtype(vk::PhysicalDevice physical_device, u32 type_filter, vk::MemoryPropertyFlags properties) -> u32;

    private:
        vk::Device device_;
        vk::Buffer buffer_;
        vk::DeviceMemory buffer_memory_;
    };
}

#endif //SYLK_VULKAN_MEMORY_BUFFER_HPP
