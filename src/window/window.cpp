//
// Created by August Silva on 25-2-23.
//

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <magic_enum/magic_enum.hpp>

#include <sylk/window/window.hpp>
#include <sylk/coreutils/all.hpp>
#include <sylk/vkutils/validation_layers.hpp>
#include <sylk/vkutils/result_handler.hpp>

namespace sylk {

    Window::Window(const Settings settings)
        : validation_layers_(vk_instance_)
        , window_(nullptr)
        , settings_(settings)
        , vk_required_extensions_({}) {
        if (!glfwInit()) {
            log(CRITICAL, "GLFW initialization failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        auto monitor = (settings.fullscreen ? glfwGetPrimaryMonitor() : nullptr);
        window_ = glfwCreateWindow(settings_.width, settings_.height, settings_.title, monitor, nullptr);

        create_vk_instance();
    }

    Window::~Window() {
        if (window_) {
            vk_instance_.destroy();
            
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
        log(DEBUG, "Querying available Vulkan extensions...");

        if (vk_required_extensions_.empty() || force_update) {
            u32 ext_count;
            const char** instance_ext_list = glfwGetRequiredInstanceExtensions(&ext_count);
            vk_required_extensions_ = std::vector(instance_ext_list, instance_ext_list + ext_count);
        }

        return {vk_required_extensions_.data(), vk_required_extensions_.size()};
    }

    void Window::create_vk_instance() {
        if (ValidationLayers::enabled() && !validation_layers_.supports_required_layers()) {
            log(CRITICAL, "Missing validation layers are likely a flaw of an incomplete Vulkan SDK.\n"
                          "Consider installing the LunarG Vulkan SDK, or run Sylk in Release mode to "
                          "disable validation layers altogether.\n"
                          "Sylk will now shut down.");
        }

        const vk::ApplicationInfo app_info = vk::ApplicationInfo()
                .setPApplicationName("Sylk")
                .setApplicationVersion(VK_MAKE_VERSION(0, 1, 0))
                .setPEngineName("None")
                .setEngineVersion(VK_MAKE_VERSION(0, 1, 0))
                .setApiVersion(VK_API_VERSION_1_0);

        fetch_vk_required_extensions();
        check_vk_required_extensions_available();

        const vk::InstanceCreateInfo instance_create_info = vk::InstanceCreateInfo()
                .setPApplicationInfo(&app_info)
                .setEnabledExtensionCount(vk_required_extensions_.size())
                .setPEnabledExtensionNames(*vk_required_extensions_.data())
#ifdef SYLK_DEBUG
                .setEnabledLayerCount(ValidationLayers::enabled_layer_count())
                .setPEnabledLayerNames(*ValidationLayers::enabled_layer_names())
#else
                .setEnabledLayerCount(0)
#endif
;

        handle_vkresult(vk::createInstance(&instance_create_info, nullptr, &vk_instance_),
                        "Failed to create Vulkan instance", true);

        log(INFO, "Created Vulkan instance.");

    }

    bool Window::check_vk_required_extensions_available() {
        const std::vector<vk::ExtensionProperties> ext_props = vk::enumerateInstanceExtensionProperties();

        log(TRACE, "Available extensions:");
        for (const auto& ext: ext_props) {
            vk_available_extensions_.emplace_back(ext.extensionName);
            log(TRACE, "   -- {}", vk_available_extensions_.back());
        }

        log(TRACE, "Required extensions:");
        for (const auto& ext: vk_required_extensions_) {
            log(TRACE, "   -- {}", ext);
        }

        bool all_available = true;
        for (const auto& req_ext : vk_required_extensions_) {
            bool ext_found = false;
            for (const auto& available_ext : vk_available_extensions_) {
                if (!std::strcmp(req_ext, available_ext)) {
                    ext_found = true;
                    break;
                }
            }

            if (!ext_found) {
                log(ERROR, "Required Vulkan extension \"{}\" was not found on this device", req_ext);
                all_available = false;
            }
        }

        if (all_available) {
            log(DEBUG, "All required Vulkan extensions are available");
        }

        return all_available;
    }

    // There is a GCC/Clang bug which causes an erroneous compiler error
    // unless we default this constructor in the cpp file.
    // The cause appears to be having nested class/struct declarations.
    // This is not a skill issue.
    // For more details, see https://bugs.llvm.org/show_bug.cgi?id=36684 (Clang)
    // and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88165 (GCC)
    Window::Settings::Settings() = default;
}