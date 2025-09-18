//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "texture.h"
#include <glm/glm.hpp>

class SpriteManager
{
public:
    static constexpr inline glm::vec2 Automatic = glm::vec2{};

private:
    struct SpritePushData
    {
        glm::vec2 pos;
        glm::vec2 size;
        glm::vec4 color;
    };

public:
    using Texture = glm::u8;
    struct SpriteData : public SpritePushData
    {
        Texture texture = 0;

        SpriteData(
            const glm::vec2& pos,
            const glm::vec2& size,
            const glm::vec4& color,
            Texture texture
        ) :
            SpritePushData(pos, size, color),
            texture(texture)
        {}

    };

private:
    struct SpriteEntry : public SpriteData
    {
    public:
        SpriteEntry(
            const glm::vec2& pos,
            const glm::vec2& size,
            const glm::vec4& color,
            Texture texture
        ) :
            SpriteData(pos, size, color, texture),
            valid(true)
        {}

        inline float top() const noexcept {return pos.y-size.y*0.5f; }
        inline float bottom() const noexcept {return pos.y+size.y*0.5f; }
        inline float left() const noexcept {return pos.x-size.x*0.5f; }
        inline float right() const noexcept {return pos.x+size.x*0.5f; }

        inline glm::vec2 tl() const noexcept {return pos-size*0.5f; }
        inline glm::vec2 tr() const noexcept {return { top(), right() }; }
        inline glm::vec2 bl() const noexcept {return { bottom(), left() }; }
        inline glm::vec2 br() const noexcept {return pos+size*0.5f; }

        inline bool intersects(const SpriteEntry& rhs)
        {
            return  (bottom() >= rhs.top()) &&
                    (top()<=rhs.bottom()) &&
                    (left() <= rhs.right()) &&
                    (right() >= rhs.left());
        }

    private:
        bool valid=false;
        friend class SpriteManager;
    };

    struct TextureEntry
    {
        Texture id;
        DeviceImage image;
        vk::raii::Sampler sampler;
    };

public:
    using Sprite = shared_ptr<SpriteEntry>;

private:
    using Container = vector<SpriteEntry>;
    inline Sprite makeSprite(Container::iterator iter)
    {
        return Sprite(&*iter, [](SpriteEntry* e){e->valid=false;});
    };

public:
    struct Layer
    {
        Container sprites;
        glm::mat4 transformation = glm::mat4(1.0f);
    };

public:
    SpriteManager(size_t layers=1, size_t maxSpritesPerLayer=1024, size_t maxTextures=256);

//    Texture recreateTexture(const string& name, const filesystem::path& filename);
    Texture getOrCreateTexture(const string& name, const filesystem::path& filename);
//    void releaseTexture(const string& name);

    Sprite createSprite(
        size_t layer,
        glm::vec2 pos,
        Texture texture,
        glm::vec2 size = Automatic,
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }
    );

    void drawAllLayers(const vk::CommandBuffer& buffer) const;
    void drawLayer(size_t layer, const vk::CommandBuffer& buffer) const;

    inline void setLayerTransform(size_t layer, const glm::mat4& transform) { layers[layer].transformation = transform; }
    
private:
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline pipeline;
    vk::raii::DescriptorSetLayout descriptorLayout;
    vk::raii::DescriptorPool descriptorPool;
    vk::raii::DescriptorSets descriptors;
    
    vector<Layer> layers;
    map<string, TextureEntry> textures;
    vector<Texture> freeTextureIds;

    Texture createTextureEntry(const string& name, const filesystem::path& filename);
};