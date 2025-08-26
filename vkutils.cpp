//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "vkutils.h"
#include <string.h> // we need strcmp...
#include <fstream>
#include "stb_image.h"

[[nodiscard]] tuple<vk::raii::PhysicalDevice, uint32_t, uint32_t>
findAppropriateDeviceAndQueueFamily(
    const vk::raii::Instance& instance,
    const DeviceRequirements& requirements,
    DeviceScore score
)
{
    auto physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.empty()) throw std::runtime_error("No physical vulkan devices found");

    vk::raii::PhysicalDevice chosenPhysicalDevice=nullptr;
    uint32_t graphicsQueue=0;
    uint32_t presentQueue=0;
    float bestScore=0.0f;
    for (const auto& device : physicalDevices)
    {
        auto properties=device.getProperties();
        auto features=device.getFeatures();

        // check capabilities to make sure device is usable
        if (properties.apiVersion < requirements.apiVersion) continue;
        if (features < requirements.features) continue;

        // because extension names use const char* instead of string and are also
        // unsorted this code is uglier than it has to be. But it checks whether all
        // required device extensions are supported by the physical device so
        // that we don't get any surprises during device creation.
        auto extensions = device.enumerateDeviceExtensionProperties();
        if (ranges::any_of(requirements.deviceExtensions, [&extensions](auto reqName) {
            return ranges::none_of(extensions, [reqName](auto&& extProp) { return strcmp(extProp.extensionName, reqName)==0; });
        })) continue;

        auto queueFamilies = device.getQueueFamilyProperties();
        uint32_t queueIndex=0;
        for (queueIndex=0; queueIndex<queueFamilies.size(); ++queueIndex)
        {
            if (queueFamilies[queueIndex].queueCount<1) continue;
            if ((queueFamilies[queueIndex].queueFlags & requirements.queueFlags) != requirements.queueFlags) continue;
            break;
        }
        if (queueIndex>=queueFamilies.size()) continue;

        uint32_t presentIndex=0;
        if (requirements.surface)
        {
            if (device.getSurfaceSupportKHR(queueIndex,*requirements.surface))
            {
                presentIndex=queueIndex;
            }
            else
            {
                for (presentIndex=0; presentIndex<queueFamilies.size(); ++presentIndex)
                {
                    if (device.getSurfaceSupportKHR(presentIndex,*requirements.surface)) break;
                }
            }
        }
        else
        {
            presentIndex=queueIndex;
        }
        if (presentIndex>=queueFamilies.size()) continue;

        auto deviceScore=score(properties,features);
        if ((chosenPhysicalDevice==nullptr) || (deviceScore>bestScore))
        {
            chosenPhysicalDevice=std::move(device);
            graphicsQueue=queueIndex;
            presentQueue=presentIndex;
            bestScore=deviceScore;
        }
    }

    if (chosenPhysicalDevice==nullptr) throw("No physical vulkan device meets the minimum requirements");
    return tie(chosenPhysicalDevice, graphicsQueue, presentQueue);
}

[[nodiscard]] tuple<vk::raii::SwapchainKHR, vk::Format, vk::Extent2D>
createSwapChain(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device,
    const vk::raii::SurfaceKHR& surface,
    const SwapChainRequirements& requirements
)
{
    //! pick a color format for our swap chain
    auto availableFormats=physicalDevice.getSurfaceFormatsKHR(surface);
    vk::SurfaceFormatKHR format={};
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            format=availableFormat;
            break;
        }
    }
    if (format.format==vk::Format::eUndefined)
    {
        format=availableFormats[0];
    }

    //! pick a presentation mode
    vk::PresentModeKHR presentMode=vk::PresentModeKHR::eFifo;
    if (requirements.preferredPresentMode!=vk::PresentModeKHR::eFifo)
    {
        auto availableModes = physicalDevice.getSurfacePresentModesKHR(surface);
        if (ranges::find(availableModes,requirements.preferredPresentMode)!=availableModes.end())
        {
            presentMode=requirements.preferredPresentMode;
        }
    }

    //! pick a swap chain size
    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR( surface );
    vk::Extent2D extent={};
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        extent=surfaceCapabilities.currentExtent;
    }
    else
    {
        extent.width=clamp<uint32_t>(requirements.fallbackSwapchainSize.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
        extent.height=clamp<uint32_t>(requirements.fallbackSwapchainSize.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
    }

    auto minImageCount = max(requirements.minImageCount, surfaceCapabilities.minImageCount);
    if (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
        minImageCount =  surfaceCapabilities.maxImageCount;

    std::pair<uint32_t,uint32_t> vals;
    auto swapChainCreateInfo = vk::SwapchainCreateInfoKHR
    {
        .surface=surface,
        .minImageCount = minImageCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers =1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true,
    };
    
    if (requirements.queueIndices)
    {
        swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = &(requirements.queueIndices->first);
    }

    auto chain=vk::raii::SwapchainKHR(device, swapChainCreateInfo);
    // std::tie doesn't work here for some reason
    return make_tuple(std::move(chain), format.format, extent);
}

[[nodiscard]] vk::raii::ShaderModule loadShaderModule(const vk::raii::Device& device, const string& filename)
{
    auto file=ifstream(filename, ios::in | ios::ate | ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open "+filename);
    auto buffer=vector<char>(file.tellg());
    file.seekg(0, ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    vk::ShaderModuleCreateInfo createInfo{ };

    return vk::raii::ShaderModule
    {
        device,
        {
            .codeSize = buffer.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(buffer.data()) 
        }
    };
}

[[nodiscard]] vk::raii::DeviceMemory allocateDeviceMemory(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device,
    const vk::MemoryRequirements& requirements,
    vk::MemoryPropertyFlags properties
    )
{
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((requirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return vk::raii::DeviceMemory( device, vk::MemoryAllocateInfo
            {
                .allocationSize=requirements.size,
                .memoryTypeIndex=i
            });
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

[[nodiscard]] vk::raii::Sampler createSampler(
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device
)
{
    auto properties = physicalDevice.getProperties();
    return vk::raii::Sampler(device, vk::SamplerCreateInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
    });
}