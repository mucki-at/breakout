//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "buffermanager.h"

class RenderTarget
{
public:
    RenderTarget();

public:
    void beginRenderTo(const vk::CommandBuffer& commandBuffer, const vk::ClearValue& clear);
    void endRenderTo(const vk::CommandBuffer& commandBuffer);

    inline const auto& getDescription() const noexcept { return description; }

protected:
    ImageDescription description;
    vk::ImageUsageFlags usage;
    vk::SampleCountFlagBits samples;
    vector<DeviceImage> images;
    vector<DeviceImage>::iterator current;
};