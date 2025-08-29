//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#include "game.h"
#include "vulkan.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <GLFW/glfw3.h>

//! @brief constructor
Game::Game(glm::vec2 fieldSize) :
    state(Active),
    keys(),
    fieldSize(fieldSize),
    sprites(1024, 16)
{
    auto bg=sprites.getOrCreateTexture("background", "textures/background.jpg");
    background=sprites.createSprite(fieldSize*0.5f, bg, fieldSize);
    level=make_unique<Level>("levels/1.txt", glm::vec2{ fieldSize.x, fieldSize.y/2 }, sprites);
}

Game::~Game()
{}

void Game::updateScreenSize()
{
    glm::vec2 screen={vulkan.getViewport().width, vulkan.getViewport().height};

    float fieldAspect=fieldSize.x/fieldSize.y;
    float screenAspect=screen.x/screen.y;

    glm::vec2 viewport;
    if (fieldAspect > screenAspect) // field is wider than screen
    {
        viewport.x = fieldSize.x;
        viewport.y = fieldSize.x/screenAspect;
    }
    else
    {
        viewport.x = fieldSize.y*screenAspect;
        viewport.y = fieldSize.y;
    }
    glm::vec2 offset=(viewport-fieldSize)*0.5f;

    sprites.transformation=glm::orthoRH_ZO(
        -offset.x, viewport.x-offset.x,
        -offset.y, viewport.y-offset.y,
        0.0f, 1.0f);
}

void Game::update(float dt)
{
}

void Game::processInput(float dt)
{
}

void Game::draw(const vk::CommandBuffer& commandBuffer) const
{
    sprites.drawSprites(commandBuffer);
}
