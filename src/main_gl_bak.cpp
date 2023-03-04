#include <sylk/coreutils/all.hpp>
#include <sylk/window/window.hpp>
#include <sylk/shader/shader.hpp>
#include <sylk/shader/program.hpp>

#include <glad/glad.h>

#include <img/stb_image.h>

#include <array>

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

    std::array vertices = {
           // pos               // color            // tex coords
           0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   4.0f, 4.0f, // top right
           0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   4.0f, 0.0f, // bottom right
          -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
          -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 4.0f, // top left
    };

    GLuint VAO {};
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    GLuint VBO {};
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

    std::array indices = {
            0, 1, 3,
            1, 2, 3,
    };

    GLuint EBO {};
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), get_stride<f32>(3));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(f32), get_stride<f32>(6));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_set_flip_vertically_on_load(true);

    i32 width, height, num_channels;
    unsigned char* img_data = stbi_load("../../assets/container.jpg", &width, &height, &num_channels, 0);

    if (!img_data) {
        log(CRITICAL, "Failed to load texture.");
    }

    u32 texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    i32 width2, height2, num_chan2;
    unsigned char* img2_data = stbi_load("../../assets/awesomeface.png", &width2, &height2, &num_chan2, 0);

    if (!img2_data) {
        log(CRITICAL, "Failed to load texture.");
    }

    u32 tex2;
    glGenTextures(1, &tex2);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width2, height2, 0, GL_RGBA, GL_UNSIGNED_BYTE, img2_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(img_data);
    stbi_image_free(img2_data);

    shader.use();
    glUniform1i(glGetUniformLocation(shader.get_handle(), "tex1"), 0);
    glUniform1i(glGetUniformLocation(shader.get_handle(), "tex2"), 1);

    f32 mix_val = 0.5f;
    f32 add_mix = 0.015f;

    while (window.is_open()) {

        window.clear();

        if (mix_val > 0.97f || mix_val < 0.03f) {
            add_mix *= -1.f;
        }

        mix_val += add_mix;

        glUniform1f(glGetUniformLocation(shader.get_handle(), "mix_val"), mix_val);

        shader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tex2);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        window.poll_events();
        window.render();
    }
}
