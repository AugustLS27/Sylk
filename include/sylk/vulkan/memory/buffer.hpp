//
// Created by August Silva on 12-3-23.
//

#ifndef SYLK_VULKAN_MEMORY_BUFFER_HPP
#define SYLK_VULKAN_MEMORY_BUFFER_HPP

#include <sylk/core/utils/short_types.hpp>
#include <sylk/vulkan/vulkan.hpp>

namespace sylk {

    class Buffer {
      public:
        struct CreateData {
            const void*                   data_to_map;
            const bool                    persistent_mapping = false;
            const vk::Device              device;
            const vk::PhysicalDevice      physical_device;
            const vk::DeviceSize          buffer_size;
            const vk::BufferUsageFlags    buffer_usage_flags;
            const vk::MemoryPropertyFlags property_flags;
        };

        struct CopyData {
            vk::Buffer      target;
            vk::DeviceSize  size;
            vk::CommandPool pool;
            vk::Device      device;
            vk::Queue       queue;
        };

      public:
        void create(CreateData data);
        void destroy_with(vk::Device device);
        void pass_data(void* data_to_pass, size_t size_in_bytes);

        void copy_onto(CopyData data) const;

        SYLK_NODISCARD auto vk_buffer() const -> vk::Buffer;
        SYLK_NODISCARD auto memory_handle() const -> vk::DeviceMemory;
        SYLK_NODISCARD auto mapped_memory() const -> void*;

      private:
        SYLK_NODISCARD auto find_memtype(vk::PhysicalDevice physical_device, u32 type_filter, vk::MemoryPropertyFlags properties) -> u32;

      private:
        vk::Buffer       buffer_;
        vk::DeviceMemory buffer_memory_;
        void*            mapped_memory_;
    };
}

#endif  // SYLK_VULKAN_MEMORY_BUFFER_HPP
