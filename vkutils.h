//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once
#include "common.h"

struct DeviceRequirements
{
    uint32_t apiVersion = vk::ApiVersion10;
    vk::QueueFlags queueFlags = vk::QueueFlagBits::eGraphics;
    std::vector<const char*> deviceExtensions = { vk::KHRSwapchainExtensionName };
    vk::SurfaceKHR surface = nullptr;
};

using DeviceScore=function<float(const vk::PhysicalDeviceProperties&)>;

[[nodiscard]] tuple<vk::raii::PhysicalDevice, uint32_t, uint32_t>
findAppropriateDeviceAndQueueFamily(
    const vk::raii::Instance& instance,
    const DeviceRequirements& requirements={},
    DeviceScore score=[](const vk::PhysicalDeviceProperties& dp){ return dp.deviceType==vk::PhysicalDeviceType::eDiscreteGpu ? 1.0 : 0.0; }
);

struct SwapChainRequirements
{
    vk::PresentModeKHR preferredPresentMode = vk::PresentModeKHR::eFifo;
    vk::Extent2D fallbackSwapchainSize = {};
    uint32_t minImageCount = 2;
    optional<pair<uint32_t,uint32_t>> queueIndices={};
};

[[nodiscard]] tuple<vk::raii::SwapchainKHR, vk::Format, vk::Extent2D>
createSwapChain(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device,
    const vk::raii::SurfaceKHR& surface,
    const vk::SurfaceFormatKHR& format,
    const SwapChainRequirements& requirements={}
);

[[nodiscard]] vk::raii::ShaderModule loadShaderModule(
    const vk::raii::Device& device,
    const std::string& filename
);

[[nodiscard]] vk::raii::DeviceMemory allocateDeviceMemory(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device,
    const vk::MemoryRequirements& requirements,
    vk::MemoryPropertyFlags properties
);

[[nodiscard]] vk::raii::Sampler createSampler(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device
);

