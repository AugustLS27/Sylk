//
// Created by August Silva on 11-3-23.
//

#ifndef SYLK_VULKAN_SHADER_VERTEX_HPP
#define SYLK_VULKAN_SHADER_VERTEX_HPP

#include <sylk/vulkan/vulkan.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>

namespace sylk {
    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

        static auto binding_description() -> vk::VertexInputBindingDescription;
        static auto attribute_descriptions() -> std::array<vk::VertexInputAttributeDescription, 2>;
    };
}

#endif  // SYLK_VULKAN_SHADER_VERTEX_HPP