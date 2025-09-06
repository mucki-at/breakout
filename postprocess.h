//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"

class PostProcess
{
public:
    PostProcess();

    void draw(const vk::CommandBuffer& commandBuffer, const vk::ImageView image);

private:
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline pipeline;
    vk::raii::Sampler sampler;

    vk::raii::DescriptorSetLayout descriptorLayout;
    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSets descriptors;
    uint32_t currentDescriptor;
};