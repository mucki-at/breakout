//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#include "game.h"

//! @brief constructor
//! @param width render width
//! @param height render height
Game::Game(size_t width, size_t height) : state(Active), keys(), width(width), height(height) {}

Game::~Game() {}

void Game::init() {}

void Game::update(float dt) {}

void Game::processInput(float dt) {}

void Game::render() {}
