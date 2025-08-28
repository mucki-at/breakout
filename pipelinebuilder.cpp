//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "pipelinebuilder.h"

vk::raii::DescriptorSetLayout DescriptorSetBuilder::buildLayout(const vk::raii::Device& device)
{
    auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{ .flags=layoutFlags };
    layoutInfo.setBindings(bindings);
    return vk::raii::DescriptorSetLayout(device, layoutInfo);
}

vk::raii::DescriptorPool DescriptorSetBuilder::buildPool(const vk::raii::Device& device, size_t setCount)
{
    auto poolInfo = vk::DescriptorPoolCreateInfo{ .flags=poolFlags };
    poolInfo.setMaxSets(setCount);
    vector<vk::DescriptorPoolSize> poolSizes;
    for (auto&& b : bindings) poolSizes.emplace_back(b.descriptorType, setCount);
    poolInfo.setPoolSizes(poolSizes);
    return vk::raii::DescriptorPool(device, poolInfo);
}

tuple<vk::raii::DescriptorSetLayout, vk::raii::DescriptorPool, vk::raii::DescriptorSets>
DescriptorSetBuilder::buildLayoutAndSets(const vk::raii::Device& device, size_t setCount)
{
    auto layout=buildLayout(device);
    auto pool=buildPool(device, setCount);
    auto layouts=vector<vk::DescriptorSetLayout>(setCount, *layout);
    auto allocInfo=vk::DescriptorSetAllocateInfo{.descriptorPool=pool};
    allocInfo.setSetLayouts(layouts);
    auto descriptorSets=vk::raii::DescriptorSets(device, allocInfo);
    return make_tuple(std::move(layout), std::move(pool), std::move(descriptorSets));

}

vk::raii::PipelineLayout PipelineLayoutBuilder::build(const vk::raii::Device& device)
{
    auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{};
    pipelineLayoutInfo.setSetLayouts(descriptorSets);
    pipelineLayoutInfo.setPushConstantRanges(pushConstants);

    return vk::raii::PipelineLayout(device, pipelineLayoutInfo);
}


vk::raii::Pipeline PipelineBuilder::build(const vk::raii::Device& device, const vk::PipelineLayout& layout)
{
    colorBlend.setAttachments(colorBlendAttachments);
    auto pipelineRenderingCreateInfo = vk::PipelineRenderingCreateInfo{
        .depthAttachmentFormat = depthFormat,
        .stencilAttachmentFormat = stencilFormat
    };
    pipelineRenderingCreateInfo.setColorAttachmentFormats(colorFormats);
    
    auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo{}
        .setVertexBindingDescriptions(vertexInputBindings)
        .setVertexAttributeDescriptions(vertexInputAttributes);

    auto viewportState = vk::PipelineViewportStateCreateInfo{};
    viewportState.setViewports(viewports);
    viewportState.setScissors(scissors);

    auto dynamicState = vk::PipelineDynamicStateCreateInfo{};
    dynamicState.setDynamicStates(dynamicStates);

    auto pipelineInfo = vk::GraphicsPipelineCreateInfo
    {
        .pNext = &pipelineRenderingCreateInfo,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterization,
        .pMultisampleState = &multisample,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlend,
        .pDynamicState = &dynamicState,
        .layout = layout
    };
    pipelineInfo.setStages(shaders);

    return vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

void PipelineBuilder::removeAllAttachments()
{
    colorFormats.clear();
    depthFormat = vk::Format::eUndefined;
    stencilFormat = vk::Format::eUndefined;
}

void PipelineBuilder::addColorAttachment(
    vk::Format format,
    const vk::PipelineColorBlendAttachmentState& blendState)
{
    colorFormats.push_back(format);
    colorBlendAttachments.push_back(blendState);
}