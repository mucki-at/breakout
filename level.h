//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "spritemanager.h"

class Level
{
public:
    Level(const filesystem::path& file, glm::vec2 size, SpriteManager& sprites);

private:
    struct Brick
    {
        SpriteManager::Sprite sprite;
        bool solid;
    };

    SpriteManager::Texture block, solid;
    vector<Brick> bricks;
};