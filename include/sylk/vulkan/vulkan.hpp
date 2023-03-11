//
// Created by August Silva on 11-3-23.
//

#ifndef SYLK_VULKAN_HPP
#define SYLK_VULKAN_HPP

template<typename... Args>
void discard(Args... args) {}

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT discard
#include <vulkan/vulkan.hpp>

#endif
