//
// Created by August Silva on 5-3-23.
//

#include <sylk/vulkan/vulkan.hpp>
#include <magic_enum/magic_enum.hpp>

#include <sylk/core/utils/log.hpp>
#include <sylk/vulkan/utils/result_handler.hpp>
#include <sylk/vulkan/utils/validation_layers.hpp>

namespace sylk {
    ValidationLayers::ValidationLayers(vk::Instance& instance)
        : vk_instance_(instance) {}

    // i know this is nearly a duplicate of required_extensions_available()
    // [window.cpp] however, minor differences between what these functions
    // actually are trying to do makes homogenizing this a bigger pita than is
    // worth right now perhaps in the future i'll figure out something cleaner
    bool ValidationLayers::supports_required_layers() {
        fetch_available_validation_layers();

        bool all_available = true;
        for (const auto& req_layer : required_layers_) {
            bool found = false;
            for (const auto& avail_layer : available_layers_) {
                if (!strcmp(req_layer, avail_layer.c_str())) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                log(ELogLvl::ERROR,
                    "Required validation layer \"{}\" was not found on this "
                    "device",
                    req_layer);
                all_available = false;
            }
        }

        if (all_available) {
            log(ELogLvl::DEBUG, "All required validation layers were located");
        }

        return all_available;
    }

    void ValidationLayers::fetch_available_validation_layers() {
        // this function only needs to be ran once per instance
        if (!available_layers_.empty()) {
            return;
        }

        log(ELogLvl::TRACE, "Querying available validation layers...");
        const auto [result, available_layers] = vk::enumerateInstanceLayerProperties();
        handle_result(result, "Failed to fetch validation layers");

        if (available_layers.empty()) {
            log(ELogLvl::ERROR, "No validation layers were detected");
            return;
        }

        log(ELogLvl::TRACE, "Layers found:");
        for (const auto& layer : available_layers) {
            available_layers_.emplace_back(layer.layerName);
            log(ELogLvl::TRACE, "  -- {}", available_layers_.back());
        }
    }

    u32 ValidationLayers::enabled_layer_count() { return static_cast<u32>(required_layers_.size()); }

    const std::vector<const char*>& ValidationLayers::enabled_layer_container() { return required_layers_; }
}  // namespace sylk