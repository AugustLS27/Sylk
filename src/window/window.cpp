//
// Created by August Silva on 25-2-23.
//

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <sylk/window/window.hpp>
#include <sylk/coreutils/all.hpp>
#include <magic_enum/magic_enum.hpp>

namespace sylk {

    Window::Window(const Settings settings)
        : window_(nullptr)
        , settings_(settings)
        , required_extensions_({}) {
        if (!glfwInit()) {
            log(CRITICAL, "GLFW initialization failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window_ = glfwCreateWindow(settings_.width, settings_.height, settings_.title, nullptr, nullptr);

        create_vk_instance();
    }

    Window::~Window() {
        if (window_) {
            glfwDestroyWindow(window_);
            glfwTerminate();
        }
    }

    bool Window::is_open() const {
        return !glfwWindowShouldClose(window_);
    }

    void Window::poll_events() const {
        glfwPollEvents();
    }

    void Window::render() const {
    }

    std::span<const char*> Window::fetch_vk_required_extensions(bool force_update) {
        if (required_extensions_.empty() || force_update) {
            u32 ext_count;
            const char** instance_ext_list = glfwGetRequiredInstanceExtensions(&ext_count);
            required_extensions_ = std::vector(instance_ext_list, instance_ext_list + ext_count);
        }

        return {required_extensions_.data(), required_extensions_.size()};
    }

    void validate(vk::Result result, const char* error_message, bool terminate = false) {
        if (result != vk::Result::eSuccess) {
            log((terminate ? CRITICAL : ERROR), "{} | {}", error_message, magic_enum::enum_name(result));
        }
    }

    void Window::create_vk_instance() {
        const vk::ApplicationInfo app_info = vk::ApplicationInfo()
                .setPApplicationName("Sylk")
                .setApplicationVersion(VK_MAKE_VERSION(0,1,0))
                .setPEngineName("None")
                .setEngineVersion(VK_MAKE_VERSION(0,1,0))
                .setApiVersion(VK_API_VERSION_1_0);

        fetch_vk_required_extensions();
        const vk::InstanceCreateInfo instance_create_info = vk::InstanceCreateInfo()
                .setPApplicationInfo(&app_info)
                .setEnabledExtensionCount(required_extensions_.size())
                .setPEnabledExtensionNames(*required_extensions_.data())
                .setEnabledLayerCount(0);

        validate(vk::createInstance(&instance_create_info, nullptr, &vk_instance_),
                 "Failed to create Vulkan instance", true);

        log(INFO, "Created Vulkan instance.");

    }

    // There is a GCC/Clang bug which causes an erroneous compiler error
    // unless we default this constructor in the cpp file.
    // The cause appears to be having nested class/struct declarations.
    // This is not a skill issue.
    // For more details, see https://bugs.llvm.org/show_bug.cgi?id=36684 (Clang)
    // and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88165 (GCC)
    Window::Settings::Settings() = default;
}