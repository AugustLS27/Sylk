//
// Created by August Silva on 5-3-23.
//

#include <vulkan/vulkan.hpp>
#include <magic_enum/magic_enum.hpp>

#include <sylk/vkutils/validation.hpp>
#define SYLK_EXPOSE_LOG_CONSTANTS
#include <sylk/coreutils/log.hpp>

namespace sylk {
    void vk_validate_result(vk::Result result, const char* error_message, bool terminate) {
        if (result != vk::Result::eSuccess) {
            log((terminate ? CRITICAL : ERROR), "{} | {}", error_message, magic_enum::enum_name(result));
        }
    }

    // i know this is nearly a duplicate of verify_req_exts_available()
    // however i believe these are the only two usages of this type of container strcmp function
    // making this modular may actually not be worth it since this function actually compares an std::array
    // should there be at least a third case for this type of function, i'll look into doing something with concepts
    bool ValidationLayers::check_layer_support() const {
        bool all_available = true;
        for (const auto& req_layer : required_layers_) {
            bool found = false;
            for (const auto& avail_layer : available_layers_) {
                if (!strcmp(req_layer, avail_layer)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                log(ERROR, "Required validation layer \"{}\" was not found on this device", req_layer);
                all_available = false;
            }
        }

        if (all_available) {
            log(DEBUG, "All required validation layers were located");
        }

        return all_available;
    }

    ValidationLayers::ValidationLayers(vk::Instance& instance)
        : vk_instance_(instance)
    {
        const auto available_layers = vk::enumerateInstanceLayerProperties();

        for (const auto& layer : available_layers) {
            available_layers_.emplace_back(layer.layerName);
        }
    }
}