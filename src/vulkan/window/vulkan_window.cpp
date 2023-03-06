//
// Created by August Silva on 25-2-23.
//

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <sylk/vulkan/window/vulkan_window.hpp>
#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>

namespace sylk {

    VulkanWindow::VulkanWindow(const Settings settings)
        : validation_layers_(instance_)
        , window_(nullptr)
        , settings_(settings)
        , required_extensions_({}) {
        if (!glfwInit()) {
            log(CRITICAL, "GLFW initialization failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        auto monitor = (settings.fullscreen ? glfwGetPrimaryMonitor() : nullptr);
        window_ = glfwCreateWindow(settings_.width, settings_.height, settings_.title, monitor, nullptr);

        create_instance();
        select_physical_device();
    }

    VulkanWindow::~VulkanWindow() {
        if (window_) {
            instance_.destroy();
            device_.destroy();
            
            glfwDestroyWindow(window_);
            glfwTerminate();
        }
    }

    bool VulkanWindow::is_open() const {
        return !glfwWindowShouldClose(window_);
    }

    void VulkanWindow::poll_events() const {
        glfwPollEvents();
    }

    void VulkanWindow::render() const {
    }

    std::span<const char*> VulkanWindow::fetch_required_extensions(bool force_update) {
        log(DEBUG, "Querying available Vulkan extensions...");

        if (required_extensions_.empty() || force_update) {
            u32 ext_count;
            const char** instance_ext_list = glfwGetRequiredInstanceExtensions(&ext_count);
            required_extensions_ = std::vector(instance_ext_list, instance_ext_list + ext_count);
        }

        return {required_extensions_.data(), required_extensions_.size()};
    }

    void VulkanWindow::create_instance() {
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

        fetch_required_extensions();
        required_extensions_available();

        const vk::InstanceCreateInfo instance_create_info = vk::InstanceCreateInfo()
                .setPApplicationInfo(&app_info)
                .setEnabledExtensionCount(required_extensions_.size())
                .setPEnabledExtensionNames(*required_extensions_.data())
#ifdef SYLK_DEBUG
                .setEnabledLayerCount(ValidationLayers::enabled_layer_count())
                .setPEnabledLayerNames(*ValidationLayers::enabled_layer_names())
#else
                .setEnabledLayerCount(0)
#endif
                ;

        handle_result(vk::createInstance(&instance_create_info, nullptr, &instance_),
                      "Failed to create Vulkan instance", true);

        log(INFO, "Created Vulkan instance.");

    }

    bool VulkanWindow::required_extensions_available() {
        const std::vector<vk::ExtensionProperties> ext_props = vk::enumerateInstanceExtensionProperties();

        log(TRACE, "Available extensions:");
        for (const auto& ext: ext_props) {
            available_extensions_.emplace_back(ext.extensionName);
            log(TRACE, "   -- {}", available_extensions_.back());
        }

        log(TRACE, "Required extensions:");
        for (const auto& ext: required_extensions_) {
            log(TRACE, "   -- {}", ext);
        }

        bool all_available = true;
        for (const auto& req_ext : required_extensions_) {
            bool ext_found = false;
            for (const auto& available_ext : available_extensions_) {
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

    void VulkanWindow::select_physical_device() {
        const auto physical_devices = instance_.enumeratePhysicalDevices();

        if (physical_devices.empty()) {
            log(CRITICAL, "No graphics cards were located on this device.");
            return;
        }

        for (const auto& dev : physical_devices) {
            log(TRACE, "Found device: {}", dev.getProperties().deviceName);
            if (device_is_suitable(dev)) {
                physical_device_ = dev;
                break;
            }
        }

        // If no discrete GPU is found, opt for integrated card instead
        if (physical_device_ == vk::PhysicalDevice()) {
            bool found = false;
            for (const auto& dev : physical_devices) {
                if (device_is_suitable(dev, vk::PhysicalDeviceType::eIntegratedGpu)) {
                    physical_device_ = dev;
                    found = true;
                    break;
                }
            }

            if (!found) {
                log(ERROR, "No suitable GPU found");
                return;
            }
        }

        log(DEBUG, "Selected device: {}", physical_device_.getProperties().deviceName);
    }

    VulkanWindow::QueueFamilyIndices VulkanWindow::find_queue_families(const vk::PhysicalDevice& device) const {
        QueueFamilyIndices indices;
        auto families = device.getQueueFamilyProperties();

        for (i32 i = 0; i < families.size(); ++i) {
            if(families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphics = i;
            }

            if (indices.has_all()) {
                break;
            }
        }
        return indices;
    }

    bool VulkanWindow::device_is_suitable(const vk::PhysicalDevice& device,
                                          vk::PhysicalDeviceType required_device_type) const {
        return device.getProperties().deviceType == required_device_type
                && find_queue_families(device).has_all();
    }

    void VulkanWindow::create_logical_device() {
        const auto queue_indices = find_queue_families(physical_device_);

        f32 queue_prio = 1.f;
        const auto dev_queue_create_info = vk::DeviceQueueCreateInfo()
                .setQueueFamilyIndex(queue_indices.graphics.value())
                .setQueueCount(1)
                .setPQueuePriorities(&queue_prio);

        const auto dev_features = vk::PhysicalDeviceFeatures();

        const auto dev_create_info = vk::DeviceCreateInfo()
                .setPQueueCreateInfos(&dev_queue_create_info)
                .setQueueCreateInfoCount(1)
                .setPEnabledFeatures(&dev_features)
                .setEnabledExtensionCount(0)
#ifdef SYLK_DEBUG
                .setEnabledLayerCount(ValidationLayers::enabled_layer_count())
                .setPEnabledLayerNames(*ValidationLayers::enabled_layer_names()) // For backward compatibility with older VK implementations
#else
                .setEnabledLayerCount(0)
#endif
                ;

        device_ = physical_device_.createDevice(dev_create_info);

        device_.getQueue(queue_indices.graphics.value(), 0, &graphics_queue_);

    }

    // There is a GCC/Clang bug which causes an erroneous compiler error
    // unless we default this constructor in the cpp file.
    // The cause appears to be having nested class/struct declarations.
    // This is not a skill issue.
    // For more details, see https://bugs.llvm.org/show_bug.cgi?id=36684 (Clang)
    // and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88165 (GCC)
    VulkanWindow::Settings::Settings() = default;
}