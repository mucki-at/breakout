//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "texture.h"
#include <glm/glm.hpp>

namespace detail
{

struct Empty {};

class ParticleSystemBase
{
public:
    struct ParticlePushData
    {
        glm::vec4 color=glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::mat2 transform=glm::mat2(1.0f);
        glm::vec2 position=glm::vec2(0.0f, 0.0f);
    };

public:
    ParticleSystemBase(const filesystem::path& texture);

protected:
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline pipeline;
    vk::raii::DescriptorSetLayout descriptorLayout;
    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSets descriptors;
    DeviceImage image;
    vk::raii::Sampler sampler;
    glm::mat4 transformation;
};

}

template<typename UserData=detail::Empty>
class ParticleSystem : private detail::ParticleSystemBase
{
public:

    struct Particle : public ParticlePushData, public UserData
    {
        glm::f32 life=0.0f;

        inline void move(const glm::vec2& d) noexcept { position+=d; }
        inline void scale(const glm::vec2& s) noexcept { transform[0][0]*=s[0]; transform[1][1]*=s[1]; }
        inline void rotate(float angle)
        {
            float c=cosf(angle);
            float s=sinf(angle);
            transform=glm::mat2(c, s, -s, c)*transform;
        }
    };

public:
    ParticleSystem(size_t maxParticles, const filesystem::path& texture) :
        ParticleSystemBase(texture),
        particles(maxParticles),
        head(particles.end()-1)
    {
    }

    void spawnParticle(const Particle& data)
    {
        // step 1: find an empty slot
        auto& p=getParticle();
        p=data;
    }

    template<typename... U>
    void spawnParticleP(
        float lifetime,
        const glm::vec4& color,
        const glm::vec2& position,
        const glm::vec2& size,
        glm::f32 rotationInRadians,
        U&&... u
    )
    {
        float c=cosf(rotationInRadians);
        float s=sinf(rotationInRadians);
        spawnParticle({
            color,
            { c*size.x, s*size.x, -s*size.y, c*size.y },
            position,
            std::forward<U>(u)...,
            lifetime
        });
    }


    void update(float dt, const auto& update)
    {
        for (auto& p : particles)
        {
            p.life-=dt;
            if (p.life>0.0f) update(p);
        }
    }

    void update(float dt)
    {
        for (auto& p : particles)
        {
            p.life-=dt;
        }
    }

    void draw(const vk::CommandBuffer& buffer) const
    {
        buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        buffer.pushConstants<glm::mat4>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, transformation);
        buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, *descriptors[0], {});
        for (const auto& p : particles)
        {
            if (p.life>0.0f)
            {
                buffer.pushConstants<ParticlePushData>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(glm::mat4), p);
                buffer.draw(4,1,0,0);
            }
        }
    }

    inline void setTransformation(const glm::mat4& mat) noexcept { transformation=mat; }

private: 
    vector<Particle> particles;
    vector<Particle>::iterator head;

    inline void advanceRoundRobin(vector<Particle>::iterator& iter)
    {
        ++iter;
        if (iter==particles.end()) iter=particles.begin();
    }

    Particle& getParticle()
    {
        // we expect to find empty slots just ahead of the most recently allocated particles
        // we search linerly from there while also keeping track of the best slot found so far
        // in case we don't find any slot.
        auto finder=head;
        advanceRoundRobin(finder);
        auto lowest=finder;
        float lowestLife=lowest->life;
        while ((finder!=head) && (finder->life>0.0f))
        {
            if (finder->life<lowestLife)
            {
                lowestLife=finder->life;
                lowest=finder;
            }
            advanceRoundRobin(finder);
        }
        if (finder->life>0.0f) finder=lowest;
        head=finder;
        return *head;
    }
};
