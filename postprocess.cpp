//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "postprocess.h"
#include "pipelinebuilder.h"
#include "vulkan.h"
#include "vkutils.h"

PostProcess::PostProcess() :
    pipelineLayout(nullptr),
    pipeline(nullptr),
    sampler(nullptr),
    descriptorLayout(nullptr),
    descriptorPool(nullptr),
    descriptors(nullptr),
    currentDescriptor(0),
    state(0.0f, 0.0f, 0.0f)
{
    DescriptorSetBuilder descBuilder;
    descBuilder.bindings.emplace_back(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
    tie(descriptorLayout, descriptorPool, descriptors)=descBuilder.buildLayoutAndSets(vulkan.getDevice(), 2);

    PipelineLayoutBuilder layoutBuilder;
    layoutBuilder.descriptorSets.push_back(descriptorLayout);
    layoutBuilder.pushConstants.push_back(vk::PushConstantRange{.stageFlags=vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(PushData)});
    pipelineLayout=layoutBuilder.build(vulkan.getDevice());

    PipelineBuilder builder;
    auto shaderModule=loadShaderModule(vulkan.getDevice(), "shaders/postprocess.spv");        
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eVertex, .module=shaderModule, .pName="vertMain"});
    builder.inputAssembly.topology = vk::PrimitiveTopology::eTriangleStrip;
    builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eFragment, .module=shaderModule, .pName="fragMain"});

    builder.addColorAttachment(
        vulkan.getSwapChainFormat().format
    );
    pipeline = builder.build(vulkan.getDevice(),pipelineLayout);

    sampler = createSampler(vulkan.getPhysicalDevice(), vulkan.getDevice());
}

void PostProcess::update(float dt)
{
    state.chaos -= dt;
    state.confuse -= dt;
    state.shake -= dt;
}

void PostProcess::draw(const vk::CommandBuffer& commandBuffer, const vk::ImageView image)
{
    auto imageInfo = vk::DescriptorImageInfo
    {
        .sampler = sampler,
        .imageView = image,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    std::array descriptorWrites{
        vk::WriteDescriptorSet{
            .dstSet=descriptors[currentDescriptor],
            .dstBinding=0,
            .descriptorCount=1,
            .descriptorType=vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo=&imageInfo
        }
    };
    vulkan.getDevice().updateDescriptorSets(descriptorWrites, {});
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptors[currentDescriptor], {});

    commandBuffer.pushConstants<PushData>(pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, state);

    commandBuffer.draw(4,1,0,0);

    currentDescriptor = (currentDescriptor+1) % descriptors.size();
}
