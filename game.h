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

private:
    // game state
    State  state;
    bool   keys[1024];
    size_t width, height;
};
