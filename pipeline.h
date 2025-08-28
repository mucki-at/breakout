//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "swapchainmanager.h"
#include "dynamicresource.h"
#include "pipelinebuilder.h"

template<typename Vertex_, typename Uniforms_, size_t MaxFramesInFlight_ = 2>
class Pipeline
{
public:
    static inline constexpr auto MaxFramesInFlight=MaxFramesInFlight_;
    using Vertex=Vertex_;
    using Uniforms=Uniforms_;

    struct PerFrameData
    {
        Uniforms* uniforms;
        vk::DescriptorSet descriptors;
    };

public:
    Pipeline(
        const vk::raii::Device& device,
        const SwapChainManager& swapper,
        const BufferManager& bufferManager,
        const DeviceImage& texture,
        const vk::raii::Sampler& sampler
    ) :
        pipeline(nullptr),
        layout(nullptr),
        descriptorPool(nullptr),    
        descriptorSets(nullptr),
        descriptorSetLayout(nullptr),
        uniformBuffers(),
        perFrame()
    {
        // descriptor sets
        DescriptorSetBuilder setBuilder;
        setBuilder.bindings.emplace_back(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex);
        setBuilder.bindings.emplace_back(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);

        tie(descriptorSetLayout, descriptorPool, descriptorSets) = setBuilder.buildLayoutAndSets(device, MaxFramesInFlight);

        // set up per frame data
        auto imageInfo = vk::DescriptorImageInfo
        {
            .sampler = sampler,
            .imageView = texture,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };

        for (size_t i=0; i<MaxFramesInFlight; ++i)
        {
            uniformBuffers.emplace_back(bufferManager.createBuffer(
                sizeof(Uniforms),
                vk::BufferUsageFlagBits::eUniformBuffer,
                true
            ));
            auto& buf=uniformBuffers.back();

            auto bufferInfo = vk::DescriptorBufferInfo
            {
                .buffer = buf,
                .offset = 0,
                .range = sizeof(Uniforms)
            };

            std::array descriptorWrites{
                vk::WriteDescriptorSet{
                    .dstSet=descriptorSets[i],
                    .descriptorCount=1,
                    .descriptorType=vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo=&bufferInfo
                },
                vk::WriteDescriptorSet{
                    .dstSet=descriptorSets[i],
                    .dstBinding=1,
                    .descriptorCount=1,
                    .descriptorType=vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo=&imageInfo
                }
            };
            device.updateDescriptorSets(descriptorWrites, {});

            perFrame.current().uniforms = static_cast<Uniforms*>(buf.offset(0));
            perFrame.current().descriptors = descriptorSets[i];
            perFrame.cycle();
        }
        

        PipelineLayoutBuilder layoutBuilder;
        layoutBuilder.descriptorSets.push_back(descriptorSetLayout);
        layout = layoutBuilder.build(device);

        PipelineBuilder builder;

        auto shaderModule=loadShaderModule(device, "shaders/slang.spv");
        
        builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eVertex, .module=shaderModule, .pName="vertMain"});
        builder.vertexInputBindings.push_back(Vertex::getBindingDescription());
        builder.vertexInputAttributes=Vertex::getAttributeDescriptions();
        builder.inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

        builder.shaders.push_back({ .stage=vk::ShaderStageFlagBits::eFragment, .module=shaderModule, .pName="fragMain"});

        builder.addColorAttachment(swapper.getFormat());
        pipeline = builder.build(device,layout);


    }

    inline const vk::raii::Pipeline& getPipeline() const noexcept { return pipeline; }
    inline const vk::raii::PipelineLayout& getLayout() const noexcept { return layout; }

    inline const PerFrameData& getNextFrame() const noexcept { perFrame.cycle(); return perFrame.current(); }
    inline const PerFrameData& getCurrentFrame() const noexcept { return perFrame.current(); }

private:
    vk::raii::Pipeline pipeline;
    vk::raii::PipelineLayout layout;

    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSetLayout descriptorSetLayout;
    vk::raii::DescriptorSets descriptorSets;
    vector<DeviceBuffer> uniformBuffers;

    DynamicResource<PerFrameData, MaxFramesInFlight> perFrame;
};

