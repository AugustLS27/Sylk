//
// Created by August Silva on 11-3-23.
//

#pragma once

#include <sylk/vulkan/vulkan.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace sylk {
    struct Vertex {
        glm::vec2 pos;
        glm::vec3 color;

//        static auto binding_description() -> vk::VertexInputBindingDescription {
//            vk::VertexInputBindingDescription description {
//                .binding = 0,
//                .stride = sizeof(Vertex),
//                .inputRate = vk::VertexInputRate::eVertex
//            };
//
//            return description;
//        }
    };
}

