//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.

#include "level.h"
#include <fstream>
#include <sstream>

static glm::vec4 Colors[]=
{
    { 0.0f, 0.0f, 0.0f, 0.0f },     // empty
    { 0.95f, 0.95f, 0.95f, 1.0f },  // 1 - white - 50 pts
    { 1.0f, 0.56f, 0.0f, 1.0f },    // 2 - orange - 60 pts
    { 0.0f, 1.0f, 1.0f, 1.0f },     // 3 - cyan - 70 pts
    { 0.0f, 1.0f, 0.0f, 1.0f },     // 4 - green - 80 pts
    { 1.0f, 0.0f, 0.0f, 1.0f },     // 5 - red - 90 pts
    { 0.0f, 0.43f, 1.0f, 1.0f},     // 6 - blue - 100 pts
    { 1.0f, 0.0f, 1.0f, 1.0f},      // 7 - purple - 110 pts
    { 1.0f, 1.0f, 0.0f, 1.0f},      // 8 - yellow - 120 pts
    { 0.62f, 0.62f, 0.62f, 1.0f},   // S - silver - 50*level pts
    { 0.74f, 0.69f, 0.0f, 1.0f}     // X - solid
};

Level::Level(const filesystem::path& path, glm::vec2 topLeft, glm::vec2 blockSize, SpriteManager& sprites, size_t layer)
{
    block = sprites.getOrCreateTexture("block", "textures/block.png");
    solid = sprites.getOrCreateTexture("solid", "textures/solid.png");

    // load from file
    string line;
    auto file=ifstream(path);
    float y=topLeft.y + blockSize.y*0.5f;
    while (getline(file, line)) // read each line from level file
    {
        float x=topLeft.x - blockSize.x*0.5f;
        for (char c : line)
        {
            x+=blockSize.x;
            if (c==32) continue;

            auto color = c-48;
            size_t hp = 1;
            if (c=='S')
            {
                color=9;
                hp=2;
            }
            else if (c=='X')
            {
                color=10;
                hp=size_t(-1);
            }

            bricks.emplace_back(
                sprites.createSprite(
                    layer, 
                    {x,y},
                    hp>1 ? solid:block,
                    blockSize,
                    Colors[color]
                ),
                40+color*10,
                hp
            );
            
        }
        y+=blockSize.y;
    }
}

tuple<SpriteManager::Sprite, glm::vec2, size_t, size_t> Level::getBallCollision(const glm::vec2& pos, float radius)
{
    for (auto&& b : bricks)
    {
        if (b.sprite)
        {
            glm::vec2 halfBlockSize=b.sprite->size*0.5f;
            // find closest point on sprite
            glm::vec2 closest=pos-b.sprite->pos;    // vector to ball relative to brick
            closest = glm::clamp(closest, -halfBlockSize, halfBlockSize);   // clamped to brick size
            closest += b.sprite->pos; // transform into world coordinates
            if (glm::length(closest-pos) < radius) // hit
            {
                auto result=b.sprite;
                bool solid=(b.hp == size_t(-1));
                if (!solid)
                {
                    b.hp--;
                    if (b.hp==0)
                    {
                        b.sprite=nullptr; // destroy block
                    }
                    else if (b.hp==1)
                    {
                        b.sprite->texture=block;
                    }
                }
                return tie(result, closest, b.hp, b.score);
            }
        }
    }
    return make_tuple(SpriteManager::Sprite{}, glm::vec2{}, false, 0);
}

bool Level::isComplete()
{
    return ranges::all_of(bricks, [](auto&&b) { return (b.hp==size_t(-1)) || b.sprite==nullptr; });
}
