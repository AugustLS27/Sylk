//
// Created by August Silva on 14-3-23.
//

#ifndef SYLK_VULKAN_SHADER_UNIFORMBUFFER_HPP
#define SYLK_VULKAN_SHADER_UNIFORMBUFFER_HPP

#include <glm/mat4x4.hpp>

namespace sylk {

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
    };

}  // namespace sylk

#endif  // SYLK_VULKAN_SHADER_UNIFORMBUFFER_HPP
