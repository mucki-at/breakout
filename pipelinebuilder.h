//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"

struct DescriptorSetBuilder
{
    vk::DescriptorSetLayoutCreateFlags layoutFlags={};
    vector<vk::DescriptorSetLayoutBinding> bindings = {};

    vk::DescriptorPoolCreateFlags poolFlags=vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    vk::raii::DescriptorSetLayout buildLayout(const vk::raii::Device& device);
    vk::raii::DescriptorPool buildPool(const vk::raii::Device& device, size_t setCount);

    tuple<vk::raii::DescriptorSetLayout, vk::raii::DescriptorPool, vk::raii::DescriptorSets>
    buildLayoutAndSets(const vk::raii::Device& device, size_t setCount);
};


struct PipelineLayoutBuilder
{
    vector<vk::DescriptorSetLayout> descriptorSets = {};
    vector<vk::PushConstantRange> pushConstants = {};

    vk::raii::PipelineLayout build(const vk::raii::Device& device);
};

struct PipelineBuilder
{
public:
    vk::PipelineCreateFlags                       flags = {};
    vector<vk::PipelineShaderStageCreateInfo>     shaders = {};
    vector<vk::VertexInputBindingDescription>     vertexInputBindings = {};
    vector<vk::VertexInputAttributeDescription>   vertexInputAttributes = {};
    vk::PipelineInputAssemblyStateCreateInfo      inputAssembly = {};
    vk::PipelineTessellationStateCreateInfo       tessellation = { .patchControlPoints=1 };
    vector<vk::Viewport>                          viewports = { {} };
    vector<vk::Rect2D>                            scissors = { {} };
    vk::PipelineRasterizationStateCreateInfo      rasterization = { .lineWidth=1.0f };
    vk::PipelineMultisampleStateCreateInfo        multisample = {};
    vk::PipelineDepthStencilStateCreateInfo       depthStencil = { .depthTestEnable=vk::True, .depthWriteEnable=vk::True, .depthCompareOp=vk::CompareOp::eLess, };
    vk::PipelineColorBlendStateCreateInfo         colorBlend = {};
    vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {};
    vector<vk::DynamicState>                      dynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };

    vector<vk::Format> colorFormats = {};
    vk::Format         depthFormat = vk::Format::eUndefined;
    vk::Format         stencilFormat = vk::Format::eUndefined;

    vk::raii::Pipeline build(const vk::raii::Device& device, const vk::PipelineLayout& layout);

    void removeAllAttachments();
    void addColorAttachment(
        vk::Format format,
        const vk::PipelineColorBlendAttachmentState& blendState={
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
        });
};