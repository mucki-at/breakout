//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include <glm/glm.hpp>

class PostProcess
{
public:
    PostProcess();

    void update(float dt);
    void draw(const vk::CommandBuffer& commandBuffer, const vk::ImageView image);

    inline void shake(float length) { state.shake = length; }
    inline void confuse(float length) { state.confuse = length; }
    inline void chaos(float length) { state.chaos = length; }

private:
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline pipeline;
    vk::raii::Sampler sampler;

    vk::raii::DescriptorSetLayout descriptorLayout;
    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSets descriptors;
    uint32_t currentDescriptor;

    struct PushData
    {
        glm::f32 chaos;
        glm::f32 confuse;
        glm::f32 shake;
    };

    PushData state;
};