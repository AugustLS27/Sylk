//
// Created by August Silva on 25-2-23.
//

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <sylk/vulkan/window/vulkan_window.hpp>
#include <sylk/core/utils/all.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>

#include <set>

namespace sylk {

    VulkanWindow::VulkanWindow(const Settings settings)
        : validation_layers_(instance_)
        , settings_(settings)
        , required_extensions_({}) {
        init_glfw();
        create_instance();
        create_surface();
        select_physical_device();
        create_logical_device();
        create_swapchain();
    }

    VulkanWindow::~VulkanWindow() {
        swapchain_.destroy();
        device_.destroy();
        instance_.destroySurfaceKHR(surface_);
        instance_.destroy();

        glfwDestroyWindow(window_);
        glfwTerminate();
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
        log(TRACE, "Querying available Vulkan extensions...");

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
                      "Failed to create Vulkan instance", true);

        log(DEBUG, "Created Vulkan instance.");

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

        std::vector<std::pair<vk::PhysicalDevice, i16>> eligible_devices;

        for (const auto dev : physical_devices) {
            log(TRACE, "Found device: {}", dev.getProperties().deviceName);
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
            log(ERROR, "No suitable GPU detected");
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

        log(INFO, "Selected device: {}", physical_device_.getProperties().deviceName);
    }

    VulkanWindow::QueueFamilyIndices VulkanWindow::find_queue_families(vk::PhysicalDevice device) const {
        QueueFamilyIndices indices;
        const auto families = device.getQueueFamilyProperties();

        for (i32 i = 0; i < families.size(); ++i) {
            if(families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                indices.graphics = i;
            }

            if (device.getSurfaceSupportKHR(i, surface_)) {
                indices.presentation = i;
            }

            if (indices.has_required()) {
                break;
            }
        }
        return indices;
    }

    bool VulkanWindow::device_is_suitable(vk::PhysicalDevice device) const {
        Swapchain::SupportDetails swapchain_support = swapchain_.query_device_support_details(device, surface_);
        bool swapchain_supported = !swapchain_support.surface_formats.empty()
                                && !swapchain_support.present_modes.empty();

        return find_queue_families(device).has_required()
               && device_supports_required_extensions(device)
               && swapchain_supported;
    }

    void VulkanWindow::create_logical_device() {
        const auto queue_indices = find_queue_families(physical_device_);

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
                .setEnabledLayerCount(validation_layers_.enabled_layer_count())
                .setPEnabledLayerNames(validation_layers_.enabled_layer_container()) // For backward compatibility with older VK implementations
#else
                .setEnabledLayerCount(0)
#endif
                ;

        device_ = physical_device_.createDevice(dev_create_info);

        device_.getQueue(queue_indices.graphics.value(), 0, &graphics_queue_);
        device_.getQueue(queue_indices.presentation.value(), 0, &presentation_queue_);
    }

    void VulkanWindow::create_surface() {
        auto tmp_surface = VkSurfaceKHR(surface_);
        handle_result(cast<vk::Result>(glfwCreateWindowSurface(instance_, window_, nullptr, &tmp_surface)),
                      "Failed to create window surface");

        surface_ = tmp_surface;
    }

    bool VulkanWindow::device_supports_required_extensions(vk::PhysicalDevice device) const {
        const auto dev_ext_props = device.enumerateDeviceExtensionProperties();

        for (const auto& req_ext : required_device_extensions_) {
            bool found = false;
            for (const auto& dev_ext : dev_ext_props) {
                if (strcmp(dev_ext.extensionName, req_ext) == 0) {
                    found = true;
                }
            }

            if (!found) {
                log(ERROR, "Missing required device extension: {}", req_ext);
                return false;
            }
        }

        log(DEBUG, "All required device extensions were found");
        return true;
    }

    void VulkanWindow::init_glfw() {
        if (!glfwInit()) {
            log(CRITICAL, "GLFW initialization failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        auto monitor = (settings_.fullscreen ? glfwGetPrimaryMonitor() : nullptr);
        window_ = glfwCreateWindow(settings_.width, settings_.height, settings_.title, monitor, nullptr);

        log(DEBUG, "Launched window");
    }

    void VulkanWindow::create_swapchain() {
        const auto queue_family_indices = find_queue_families(physical_device_);
        std::vector queue_families {queue_family_indices.graphics.value(), queue_family_indices.presentation.value()};
        const bool queues_equal = queue_family_indices.graphics == queue_family_indices.presentation;

        // exclusive mode does not require specified queue families
        if (!queues_equal) {
            queue_families.clear();
        }

        const Swapchain::CreateParams params {
            .window = window_,
            .support_details = swapchain_.query_device_support_details(physical_device_, surface_),
            .device = device_,
            .surface = surface_,
            .sharing_mode = (queues_equal ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent),
            .queue_family_indices = queue_families,
        };

        swapchain_.create(params);

        log(DEBUG, "Created swapchain");
    }
}





// There is a GCC/Clang bug which causes an erroneous compiler error
// unless we default this constructor in the cpp file.
// The cause appears to be having nested class/struct declarations.
// The error wasn't my fault for once.
// For more details, see https://bugs.llvm.org/show_bug.cgi?id=36684 (Clang)
// and https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88165 (GCC)
sylk::VulkanWindow::Settings::Settings() = default;