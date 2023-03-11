//
// Created by August Silva on 25-2-23.
//

#include <sylk/vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <sylk/vulkan/window/vulkan_window.hpp>
#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/vulkan/utils/queue_family_indices.hpp>

#include <set>

namespace sylk {

    VulkanWindow::VulkanWindow(const Settings settings)
        : validation_layers_(instance_)
        , settings_(settings)
        , required_extensions_({})
        , swapchain_(device_)
        {
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

    bool VulkanWindow::is_open() const {
        return !glfwWindowShouldClose(window_);
    }

    void VulkanWindow::poll_events() const {
        glfwPollEvents();
    }

    void VulkanWindow::render() {
        swapchain_.draw_next();
    }

    std::span<const char*> VulkanWindow::fetch_required_extensions(bool force_update) {
        log(ELogLvl::TRACE, "Querying available Vulkan extensions...");

        if (required_extensions_.empty() || force_update) {
            u32 ext_count;
            const char** instance_ext_list = glfwGetRequiredInstanceExtensions(&ext_count);
            required_extensions_ = std::vector(instance_ext_list, instance_ext_list + ext_count);
        }

        return {required_extensions_.data(), required_extensions_.size()};
    }

    void VulkanWindow::create_instance() {
        if (ValidationLayers::enabled() && !validation_layers_.supports_required_layers()) {
            log(ELogLvl::CRITICAL, "Missing validation layers are likely a flaw of an incomplete Vulkan SDK.\n"
                          "Consider installing the LunarG Vulkan SDK, or run Sylk in Release mode to "
                          "disable validation layers altogether.\n"
                          "Sylk will now shut down.");
        }

        const vk::ApplicationInfo app_info = vk::ApplicationInfo()
                .setPApplicationName("Sylk")
                .setApplicationVersion(VK_MAKE_VERSION(0, 1, 0))
                .setPEngineName("None")
                .setEngineVersion(VK_MAKE_VERSION(0, 1, 0))
                .setApiVersion(VK_API_VERSION_1_3);

        fetch_required_extensions();
        required_extensions_available();

        vk::InstanceCreateInfo instance_create_info = vk::InstanceCreateInfo()
                .setPApplicationInfo(&app_info)
                .setPEnabledExtensionNames(required_extensions_)
#ifdef SYLK_DEBUG
                .setEnabledLayerCount(validation_layers_.enabled_layer_count())
                .setPEnabledLayerNames(validation_layers_.enabled_layer_container())
#else
                .setEnabledLayerCount(0)
#endif
                ;

        
        handle_result(vk::createInstance(&instance_create_info, nullptr, &instance_),
                      "Failed to create Vulkan instance", ELogLvl::CRITICAL);

        log(ELogLvl::DEBUG, "Created Vulkan instance");

    }

    bool VulkanWindow::required_extensions_available() {
        const auto [result, ext_props] = vk::enumerateInstanceExtensionProperties();
        handle_result(result, "Failed to enumerate instance extension properties");

        log(ELogLvl::TRACE, "Available extensions:");
        for (const auto& ext: ext_props) {
            available_extensions_.emplace_back(ext.extensionName);
            log(ELogLvl::TRACE, "   -- {}", available_extensions_.back());
        }

        log(ELogLvl::TRACE, "Required extensions:");
        for (const auto& ext: required_extensions_) {
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

        for (const auto dev : physical_devices) {
            log(ELogLvl::TRACE, "  -- Found device: {}", dev.getProperties().deviceName);
            const auto device_type = dev.getProperties().deviceType;
            i16 score = 0;

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

        std::sort(eligible_devices.begin(), eligible_devices.end(),
                  [=] (std::pair<vk::PhysicalDevice, i16> a, std::pair<vk::PhysicalDevice, i16> b) {
                      return a.second > b.second;
                  });

        for (const auto [dev, score] : eligible_devices) {
            if (device_is_suitable(dev)) {
                physical_device_ = dev;
                break;
            }
        }

        log(ELogLvl::INFO, "Selected device: {}", physical_device_.getProperties().deviceName);
    }

    bool VulkanWindow::device_is_suitable(vk::PhysicalDevice device) const {
        log(ELogLvl::TRACE, "Verifying device suitability...");

        Swapchain::SupportDetails swapchain_support = swapchain_.query_device_support_details(device, surface_);
        bool swapchain_supported = !swapchain_support.surface_formats.empty()
                                && !swapchain_support.present_modes.empty();

        return QueueFamilyIndices::find(device, surface_).has_required()
               && device_supports_required_extensions(device)
               && swapchain_supported;
    }

    void VulkanWindow::create_logical_device() {
        const auto queue_indices = QueueFamilyIndices::find(physical_device_, surface_);

        f32 queue_prio = 1.f;
        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        std::set<u32> unique_queue_families {queue_indices.graphics.value(), queue_indices.presentation.value()};
        for (const auto family : unique_queue_families) {
            const auto dev_queue_create_info = vk::DeviceQueueCreateInfo()
                    .setQueueFamilyIndex(family)
                    .setQueueCount(1)
                    .setPQueuePriorities(&queue_prio);
            queue_create_infos.push_back(dev_queue_create_info);
        }

        const auto dev_features = vk::PhysicalDeviceFeatures();

        const auto dev_create_info = vk::DeviceCreateInfo()
                .setPQueueCreateInfos(queue_create_infos.data())
                .setQueueCreateInfoCount(cast<u32>(queue_create_infos.size()))
                .setPEnabledFeatures(&dev_features)
                .setEnabledExtensionCount(cast<u32>(required_device_extensions_.size()))
                .setPEnabledExtensionNames(required_device_extensions_)
#ifdef SYLK_DEBUG
                // For backward compatibility with older VK implementations
                .setEnabledLayerCount(validation_layers_.enabled_layer_count())
                .setPEnabledLayerNames(validation_layers_.enabled_layer_container())
#else
                .setEnabledLayerCount(0)
#endif
                ;

        const auto [result, dev] = physical_device_.createDevice(dev_create_info);
        handle_result(result, "Failed to create logical Vulkan device", ELogLvl::CRITICAL);
        device_ = dev;

        swapchain_.set_queues(
                device_.getQueue(queue_indices.graphics.value(), 0),
                device_.getQueue(queue_indices.presentation.value(), 0)
                );

        log(ELogLvl::DEBUG, "Created Vulkan logical device");
    }

    void VulkanWindow::create_surface() {
        auto tmp_surface = VkSurfaceKHR(surface_);
        handle_result(cast<vk::Result>(glfwCreateWindowSurface(instance_, window_, nullptr, &tmp_surface)),
                      "Failed to create window surface");

        surface_ = tmp_surface;

        log(ELogLvl::TRACE, "Created window surface");
    }

    bool VulkanWindow::device_supports_required_extensions(vk::PhysicalDevice device) const {
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
        window_ = glfwCreateWindow(settings_.width, settings_.height, settings_.title, monitor, nullptr);

        log(ELogLvl::DEBUG, "Launched window");
    }
}





// There is a GCC/Clang bug which causes an erroneous compiler error
// unless we default this constructor in the cpp file.
// The cause appears to be having nested class/struct declarations.
// The error wasn't my fault for once.
// For more details, see https://bugs.llvm.org/show_bug.cgi?id=36684 (Clang)
// and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88165 (GCC)
sylk::VulkanWindow::Settings::Settings() = default;