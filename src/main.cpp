//
// Created by August Silva on 4-3-23.
//

#include <sylk/vulkan/window/vulkan_window.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

int main() {
    sylk::VulkanWindow window;

    while (window.is_open()) {
        window.poll_events();
    }

}
