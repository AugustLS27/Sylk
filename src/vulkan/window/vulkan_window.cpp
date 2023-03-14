//
// Created by August Silva on 25-2-23.
//

#include <sylk/vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/utils/queue_family_indices.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>
#include <sylk/vulkan/window/vulkan_window.hpp>

#include <set>

namespace sylk {

    VulkanWindow::VulkanWindow(const Settings settings)
        : validation_layers_(instance_)
        , settings_(settings)
        , swapchain_(device_) {
        create_window();
        create_instance();
        create_surface();
        select_physical_device();
        create_logical_device();
        swapchain_.create(physical_device_, window_, surface_);
    }

    VulkanWindow::~VulkanWindow() {
        handle_result(device_.waitIdle(), "Device error");

        swapchain_.destroy();

        device_.destroy();
        log(ELogLvl::TRACE, "Destroyed logical device object");

        instance_.destroySurfaceKHR(surface_);
        log(ELogLvl::TRACE, "Destroyed window surface");

        instance_.destroy();
        log(ELogLvl::TRACE, "Destroyed Vulkan instance");

        glfwDestroyWindow(window_);
        glfwTerminate();
        log(ELogLvl::TRACE, "Destroyed window");
    }

    bool VulkanWindow::is_open() const { return !glfwWindowShouldClose(window_); }

    void VulkanWindow::poll_events() const { glfwPollEvents(); }

    void VulkanWindow::render() { swapchain_.draw_next(); }

    std::span<const char*> VulkanWindow::fetch_required_extensions(const bool force_update) {
        log(ELogLvl::TRACE, "Querying available Vulkan extensions...");

        if (required_extensions_.empty() || force_update) {
            u32          ext_count;
            const char** instance_ext_list = glfwGetRequiredInstanceExtensions(&ext_count);

            required_extensions_ = std::vector(instance_ext_list, instance_ext_list + ext_count);
        }

        return {required_extensions_.data(), required_extensions_.size()};
    }

    void VulkanWindow::create_instance() {
        if (ValidationLayers::enabled() && !validation_layers_.supports_required_layers()) {
            log(ELogLvl::CRITICAL,
                "Missing validation layers are likely a flaw of an incomplete Vulkan SDK.\n"
                "Consider installing the LunarG Vulkan SDK, or run Sylk in Release mode to "
                "disable validation layers altogether.\n"
                "Sylk will now shut down.");
        }

        const vk::ApplicationInfo app_info = vk::ApplicationInfo {
            .pApplicationName   = settings_.title,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "Sylk",
            .engineVersion      = VK_MAKE_VERSION(SYLK_VERSION_MAJOR, SYLK_VERSION_MINOR, SYLK_VERSION_PATCH),
            .apiVersion         = VK_API_VERSION_1_3,
        };

        fetch_required_extensions();
        required_extensions_available();

        vk::InstanceCreateInfo instance_info = vk::InstanceCreateInfo {
            .pApplicationInfo = &app_info,
#ifdef SYLK_DEBUG
            .enabledLayerCount   = validation_layers_.enabled_layer_count(),
            .ppEnabledLayerNames = validation_layers_.enabled_layer_container().data(),
#else
            .enabledLayerCount     = 0,
#endif
            .enabledExtensionCount   = cast<u32>(required_extensions_.size()),
            .ppEnabledExtensionNames = required_extensions_.data(),
        };

        const auto [result, instance] = vk::createInstance(instance_info);
        handle_result(result, "Failed to create Vulkan instance", ELogLvl::CRITICAL);
        instance_ = instance;

        log(ELogLvl::DEBUG, "Created Vulkan instance");
    }

    auto VulkanWindow::required_extensions_available() -> bool {
        const auto [result, ext_props] = vk::enumerateInstanceExtensionProperties();
        handle_result(result, "Failed to enumerate instance extension properties");

        log(ELogLvl::TRACE, "Available extensions:");
        for (const auto& ext : ext_props) {
            available_extensions_.emplace_back(ext.extensionName);
            log(ELogLvl::TRACE, "   -- {}", available_extensions_.back());
        }

        log(ELogLvl::TRACE, "Required extensions:");
        for (const auto& ext : required_extensions_) {
            log(ELogLvl::TRACE, "   -- {}", ext);
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
                log(ELogLvl::ERROR, "Required Vulkan extension \"{}\" was not found on this device", req_ext);
                all_available = false;
            }
        }

        if (all_available) {
            log(ELogLvl::DEBUG, "All required Vulkan extensions are available");
        }

        return all_available;
    }

    void VulkanWindow::select_physical_device() {
        log(ELogLvl::TRACE, "Querying available physical devices...");

        const auto [result, physical_devices] = instance_.enumeratePhysicalDevices();
        handle_result(result, "Failed to enumerate physical devices");

        if (physical_devices.empty()) {
            log(ELogLvl::CRITICAL, "No graphics cards were located on this device.");
            return;
        }

        std::vector<std::pair<vk::PhysicalDevice, i16>> eligible_devices;

        // to later be expanded with more intricate functionality
        // list of devices should also be stored to allow the user to switch manually
        for (const auto dev : physical_devices) {
            log(ELogLvl::TRACE, "  -- Found device: {}", dev.getProperties().deviceName);
            const auto device_type = dev.getProperties().deviceType;
            i16        score       = 0;

            if (device_type == vk::PhysicalDeviceType::eDiscreteGpu) {
                score += 1000;
                eligible_devices.emplace_back(dev, score);
            } else if (device_type == vk::PhysicalDeviceType::eIntegratedGpu) {
                score += 200;
                eligible_devices.emplace_back(dev, score);
            }
        }

        if (eligible_devices.empty()) {
            log(ELogLvl::ERROR, "No suitable GPU detected");
            return;
        }

        std::sort(eligible_devices.begin(),
                  eligible_devices.end(),
                  [=](std::pair<vk::PhysicalDevice, i16> a, std::pair<vk::PhysicalDevice, i16> b) { return a.second > b.second; });

        for (const auto [dev, score] : eligible_devices) {
            if (device_is_suitable(dev)) {
                physical_device_ = dev;
                break;
            }
        }

        log(ELogLvl::INFO, "Selected device: {}", physical_device_.getProperties().deviceName);
    }

    auto VulkanWindow::device_is_suitable(const vk::PhysicalDevice device) const -> bool {
        log(ELogLvl::TRACE, "Verifying device suitability...");

        const Swapchain::SupportDetails swapchain_support = swapchain_.query_device_support_details(device, surface_);

        const bool swapchain_supported = !swapchain_support.surface_formats.empty() && !swapchain_support.present_modes.empty();

        return QueueFamilyIndices::find(device, surface_).has_required() && device_supports_required_extensions(device) &&
               swapchain_supported;
    }

    void VulkanWindow::create_logical_device() {
        const auto queue_indices = QueueFamilyIndices::find(physical_device_, surface_);

        const f32                              queue_prio = 1.f;
        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        const std::set<u32>                    unique_queue_families {queue_indices.graphics.value(), queue_indices.presentation.value()};
        for (const auto family : unique_queue_families) {
            const auto dev_queue_create_info =
                vk::DeviceQueueCreateInfo {
                    .queueFamilyIndex = family,
                }
                    .setQueuePriorities(queue_prio);

            queue_create_infos.push_back(dev_queue_create_info);
        }

        const auto dev_features = vk::PhysicalDeviceFeatures();

        const auto dev_create_info =
            vk::DeviceCreateInfo {
#ifdef SYLK_DEBUG
                .enabledLayerCount   = validation_layers_.enabled_layer_count(),
                .ppEnabledLayerNames = validation_layers_.enabled_layer_container().data(),
#else
                .enabledLayerCount = 0,
#endif

                .enabledExtensionCount   = cast<u32>(required_device_extensions_.size()),
                .ppEnabledExtensionNames = required_device_extensions_.data(),
                .pEnabledFeatures        = &dev_features,
            }
                .setQueueCreateInfos(queue_create_infos);

        const auto [result, dev] = physical_device_.createDevice(dev_create_info);
        handle_result(result, "Failed to create logical Vulkan device", ELogLvl::CRITICAL);
        device_ = dev;

        swapchain_.set_queues(device_.getQueue(queue_indices.graphics.value(), 0), device_.getQueue(queue_indices.presentation.value(), 0));

        log(ELogLvl::DEBUG, "Created Vulkan logical device");
    }

    void VulkanWindow::create_surface() {
        auto tmp_surface = VkSurfaceKHR(surface_);
        handle_result(cast<vk::Result>(glfwCreateWindowSurface(instance_, window_, nullptr, &tmp_surface)),
                      "Failed to create window surface");

        surface_ = tmp_surface;

        log(ELogLvl::TRACE, "Created window surface");
    }

    auto VulkanWindow::device_supports_required_extensions(const vk::PhysicalDevice device) const -> bool {
        log(ELogLvl::TRACE, "Querying supported device extensions...");

        const auto [result, dev_ext_props] = device.enumerateDeviceExtensionProperties();
        handle_result(result, "Failed to enumerate device's extension properties");

        for (const auto& req_ext : required_device_extensions_) {
            bool found = false;
            for (const auto& dev_ext : dev_ext_props) {
                if (strcmp(dev_ext.extensionName, req_ext) == 0) {
                    found = true;
                }
            }

            if (!found) {
                log(ELogLvl::ERROR, "Missing required device extension: {}", req_ext);
                return false;
            }
        }

        log(ELogLvl::DEBUG, "All required device extensions were found");
        return true;
    }

    void VulkanWindow::create_window() {
        if (!glfwInit()) {
            log(ELogLvl::CRITICAL, "GLFW initialization failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        auto monitor = (settings_.fullscreen ? glfwGetPrimaryMonitor() : nullptr);
        window_      = glfwCreateWindow(settings_.width, settings_.height, settings_.title, monitor, nullptr);

        log(ELogLvl::DEBUG, "Launched window");
    }
}  // namespace sylk

// There is a GCC/Clang bug which causes an erroneous compiler error
// unless we default this constructor in the cpp file.
// The cause appears to be having nested class/struct declarations.
// The error wasn't my fault for once.
// For more details, see https://bugs.llvm.org/show_bug.cgi?id=36684 (Clang)
// and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88165 (GCC)
sylk::VulkanWindow::Settings::Settings() = default;