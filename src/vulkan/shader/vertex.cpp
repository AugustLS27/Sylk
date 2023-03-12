//
// Created by August Silva on 11-3-23.
//

#include <sylk/vulkan/shader/vertex.hpp>

namespace sylk {
    auto Vertex::binding_description() -> vk::VertexInputBindingDescription {
        const auto description = vk::VertexInputBindingDescription {
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = vk::VertexInputRate::eVertex
        };

        return description;
    }

    auto Vertex::attribute_descriptions() -> std::array<vk::VertexInputAttributeDescription, 2> {
        using VAD = vk::VertexInputAttributeDescription;

        const auto description = std::array {
            VAD {
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = offsetof(Vertex, pos)
            },

            VAD {
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Vertex, color)
            },
        };

        return description;
    }
}
