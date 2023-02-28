#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <array>

#include <sylk/coreutils/all.hpp>
#include <sylk/window/window.hpp>
#include <sylk/shader/shader.hpp>
#include <sylk/shader/program.hpp>

template<typename T>
void* get_stride(u32 stride) {
    return (void*)(stride * sizeof(T));
}

int main() {
    using namespace sylk;

    Window window;

    Shader vertex(Shader::Type::Vertex, "shader.vert");

    Program shader;
    shader.add_shader(vertex);
    shader.add_shader(Shader::Type::Fragment, "shader.frag");
    shader.link();

    f32 r = 0.9f, g = 0.5f, b = 0.33f;
    f32 rinc = 0.002873f, ginc = 0.003123f, binc = 0.004984f;

    constexpr f32 cmax = 0.98f;
    constexpr f32 cmin = 0.32f;

    while (window.is_open()) {
        std::array vertices = {
                0.5f, -0.5f, 0.0f,   r, g, b,
               -0.5f, -0.5f, 0.0f,   g, b, r,
                0.0f,  0.5f, 0.0f,   b, r, g,
        };

        r += rinc;
        g += ginc;
        b += binc;

        if (r > cmax || r < cmin) rinc *= -1.f;
        if (g > cmax || g < cmin) ginc *= -1.f;
        if (b > cmax || b < cmin) binc *= -1.f;

        GLuint VAO {};
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        GLuint VBO {};
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), nullptr);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(f32), get_stride<f32>(3));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        window.clear();

        shader.use();
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

//        red_shader.use();
//        glBindVertexArray(VAO);
//        glDrawArrays(GL_TRIANGLES, 0, 3);
//
//        blue_shader.use();
//        glBindVertexArray(VAO_Right);
//        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.poll_events();
        window.render();
    }
}
