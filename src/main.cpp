//
// Created by August Silva on 4-3-23.
//

#include <sylk/window/window.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

int main() {
    sylk::Window window;

    while (window.is_open()) {
        window.poll_events();
    }

}
