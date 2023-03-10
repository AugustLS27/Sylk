//
// Created by August Silva on 4-3-23.
//

#include <sylk/vulkan/window/vulkan_window.hpp>

int main() {
    sylk::VulkanWindow window;

    while (window.is_open()) {
        window.poll_events();
        //window.render();
    }
}
