//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#pragma once

#include "common.h"
#include "spritemanager.h"
#include "level.h"

//! @brief Game holds all game-related state and functionality.
//! Combines all game-related data into a single class for
//! easy access to each of the components and manageability.
class Game
{
public:
    static constexpr size_t KeyCount = 1024;

public:
    enum State
    {
        Active,
        Menu,
        Win
    };

public:
    // constructor/destructor
    Game(glm::vec2 fieldSize);
    ~Game();

    // game loop
    void updateScreenSize();
    void processInput(float dt);
    void update(float dt);
    void draw(const vk::CommandBuffer& commandBuffer) const;

    inline void setKey(size_t key, bool pressed)
    {
        if (key < KeyCount)
            keys[key] = pressed;
    }

private:
    // game state
    State  state;
    bool   keys[KeyCount];
    glm::vec2 fieldSize;

    // draws all our sprites
    SpriteManager sprites;
    SpriteManager::Sprite background;

    // level specific data
    unique_ptr<Level> level;
};
