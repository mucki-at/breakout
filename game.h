//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
#pragma once

#include "common.h"

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
    Game(size_t width, size_t height);
    ~Game();

    // initialize game state (load all shaders/textures/levels)
    void init();

    // game loop
    void processInput(float dt);
    void update(float dt);
    void render();

    inline void setKey(size_t key, bool pressed)
    {
        if (key < KeyCount)
            keys[key] = pressed;
    }

private:
    // game state
    State  state;
    bool   keys[KeyCount];
    size_t width, height;
};
