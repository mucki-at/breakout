//!@author mucki (code@mucki.dev)
//!@copyright Copyright (c) 2025
//! please see LICENSE file in root folder for licensing terms.
#include "game.h"
#include "vulkan.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>

#include <SDL3/SDL_scancode.h>

//! @brief constructor
Game::Game(const filesystem::path& levels) :
    state(Active),
    keys(),
    fieldTL(FieldPosition),
    fieldBR(FieldPosition+FieldSize),
    sprites(3, 1024, 16),
    trail(ceil(TrailEmitsPerSecond*TrailDuration)+1, "textures/circle.png"),
    brickParts(128,  "textures/fragment.png"),
    score(0),
    nextTrailEmit(0.0f),
    font("textures/exan3.ttf")
{
    for (auto const& dir_entry : std::filesystem::directory_iterator{levels})
    {
        if (dir_entry.is_regular_file()) levelList.push_back(dir_entry.path());
    }
    if (levelList.empty()) throw runtime_error("Failed to find any levels");
    ranges::sort(levelList);
    curLevel=levelList.end();

    auto bg=sprites.getOrCreateTexture("background", "textures/background.png");
    background=sprites.createSprite(BackgroundLayer, LogicalSize*0.5f, bg, BackgroundSize);

    auto wallTex=sprites.getOrCreateTexture("wall", "textures/wall.png");
    for (float x=1.0f; x<30.0f; x+=2.0f)
    {
        walls.push_back(sprites.createSprite(ForegroundLayer, {x, 1.0f}, wallTex, {2.0f, 2.0f}));
    }
    for (float y=3.0f; y<30.0f; y+=2.0f)
    {  
        walls.push_back(sprites.createSprite(ForegroundLayer, {1.0f, y}, wallTex, {2.0f, 2.0f}));
        walls.push_back(sprites.createSprite(ForegroundLayer, {29.0f, y}, wallTex, {2.0f, 2.0f}));
    }

    auto defaultPaddle = sprites.getOrCreateTexture("paddle","textures/paddle.png");
    player=sprites.createSprite(
        GameLayer,
        {}, // position will be set up when level is initialied
        defaultPaddle,
        InitialPlayerSize
    );

    ball.radius = InitialBallSize;
    ball.sprite = sprites.createSprite(
        GameLayer,
        {}, // position is reset when level starts
        sprites.getOrCreateTexture("ball", "textures/awesomeface.png"),
        { ball.radius*2.2f, ball.radius*2.2f }
    );

    powerupDefinitions.emplace_back(PowerUp::None, defaultPaddle, NeutralPowerupColor, 0.0f, 0.0f);
    powerupDefinitions.emplace_back(PowerUp::Speed, sprites.getOrCreateTexture("speed", "textures/powerup_speed.png"), GoodPowerupColor, 2.0f, 30.0f);
    powerupDefinitions.emplace_back(PowerUp::Sticky, sprites.getOrCreateTexture("sticky", "textures/powerup_sticky.png"), GoodPowerupColor, 1.0f, 30.0f);
    powerupDefinitions.emplace_back(PowerUp::PassThrough, sprites.getOrCreateTexture("passthrough", "textures/powerup_passthrough.png"), GoodPowerupColor, 1.0f, 10.0f);
    powerupDefinitions.emplace_back(PowerUp::Size, sprites.getOrCreateTexture("increase", "textures/powerup_increase.png"), GoodPowerupColor, 2.0f, 30.0f);
    powerupDefinitions.emplace_back(PowerUp::Confuse, sprites.getOrCreateTexture("confuse", "textures/powerup_confuse.png"), BadPowerupColor, 1.0f, 5.0f);
    powerupDefinitions.emplace_back(PowerUp::Chaos, sprites.getOrCreateTexture("chaos", "textures/powerup_chaos.png"), BadPowerupColor, 1.0f, 5.0f);

    float sum=0.0f;
    for (auto&& pd : powerupDefinitions) sum+=pd.chance;
    for (auto&& pd : powerupDefinitions) pd.chance=(pd.chance*PowerupChance)/sum;


    activePowerup.type=PowerUp::None;
    activePowerup.timeLeft=0.0f;

    nextLevel();

    brick=audioManager.loadWavWithVariations("sounds/brick0.wav","sounds/brick1.wav","sounds/brick2.wav");
    go=audioManager.loadWav("sounds/go.wav");
    lost=audioManager.loadWav("sounds/lost.wav");
    paddle=audioManager.loadWavWithVariations("sounds/paddle0.wav","sounds/paddle1.wav");
    solid=audioManager.loadWav("sounds/solid.wav");
    wall=audioManager.loadWavWithVariations("sounds/wall0.wav","sounds/wall1.wav","sounds/wall2.wav");
}

Game::~Game()
{}

void Game::updateScreenSize(const vk::Extent2D& extent)
{
    glm::vec2 screen={extent.width, extent.height};

    float fieldAspect=LogicalSize.x/LogicalSize.y;
    float screenAspect=screen.x/screen.y;

    glm::vec2 viewport;
    if (fieldAspect > screenAspect) // field is wider than screen
    {
        viewport.x = LogicalSize.x;
        viewport.y = LogicalSize.x/screenAspect;
    }
    else
    {
        viewport.x = LogicalSize.y*screenAspect;
        viewport.y = LogicalSize.y;
    }
    glm::vec2 offset=(viewport-LogicalSize)*0.5f;

    auto ortho=glm::orthoRH_ZO(
        -offset.x, viewport.x-offset.x,
        -offset.y, viewport.y-offset.y,
        0.0f, 1.0f);
    sprites.setLayerTransform(BackgroundLayer, ortho);
    sprites.setLayerTransform(GameLayer, ortho);
    sprites.setLayerTransform(ForegroundLayer, ortho);
    trail.setTransformation(ortho);
    brickParts.setTransformation(ortho);

    font.resize(ortho, extent, FontSize);
}

void Game::update(float dt, PostProcess& post)
{
    float decay=powf(1.0f-TrailDecayPerSecond,dt);

    trail.update(dt, [dt,decay](auto& p) {
        p.move(p.velocity*dt);
        p.rotate(p.angularVelocity*dt);
        p.velocity*=decay;
        p.angularVelocity*=decay;
        p.color.a*=decay;
    });

    brickParts.update(dt, [dt,decay](auto& p) {
        p.move(p.velocity*dt);
        p.rotate(p.angularVelocity*dt);
        p.velocity.y+=dt*Gravity;
        p.color.a*=decay;
    });


    updatePowerups(dt,post);

    if (level->isComplete()) nextLevel();

    if (ball.stuck)
    {
        ball.sprite->pos.x=player->pos.x+ball.stickOffset;
        ball.sprite->pos.y=player->top()-ball.radius;
    }
    else
    {
        updateBall(dt,post);
    }
}

void Game::updateBall(float dt, PostProcess& post)
{
    // move ball 
    auto& bp=ball.sprite->pos;

    nextTrailEmit+=TrailEmitsPerSecond*dt;
    while (nextTrailEmit>1.0f)
    {
        auto ofs=glm::linearRand(-TrailPosVar, TrailPosVar);
        trail.spawnParticleP(
            TrailDuration,
            TrailColor,
            bp+ofs,
            glm::linearRand(TrailSizeMin, TrailSizeMax),
            glm::linearRand(0.0f, float(M_PI)*0.5f),
            ofs*2.0f,
            glm::linearRand(-float(M_PI),float(M_PI))*3.0f
        );
        nextTrailEmit-=1.0f;
    }

    bp += ball.velocity * dt;

    // bounce off of walls
    if (bp.x <= fieldTL.x+ball.radius)
    {
        wall->play();
        reflectBall(true, fieldTL.x+ball.radius);
    }
    else if (bp.x >= fieldBR.x-ball.radius)
    {
        wall->play();
        reflectBall(true, fieldBR.x-ball.radius);
    }

    if (bp.y <= fieldTL.y+ball.radius)
    {
        wall->play();
        reflectBall(false, fieldTL.y+ball.radius);
    }
    else if (bp.y >= fieldBR.y+ball.radius)
    {
        lost->play();
        resetPlayer();
        return;
    }

    // check collision with level and reflect accordingly
    auto [block, closest, hp, score]=level->getBallCollision(bp, ball.radius);
    if (block)
    {
        if (hp==size_t(-1))
        {
            solid->play();
            post.shake(0.05);
        }
        else if (hp>0)
        {
           solid->play();
           explodeBrick(block->color, block->pos, block->size, closest, glm::length(ball.velocity));             
        }
        else
        {
            brick->play();
            explodeBrick(block->color, block->pos, block->size, closest, glm::length(ball.velocity));
            this->score+=score;
            maybeSpawnPowerups(block);
        }

        if ((activePowerup.type!=PowerUp::PassThrough) || (hp>0))
        {
            glm::vec2 impactDirection = closest-ball.sprite->pos;
            // TODO: handle corners better
            if (fabs(impactDirection.x) > fabs(impactDirection.y))  // reflect horizontally
            {
                if (impactDirection.x > 0) reflectBall(true, closest.x-ball.radius);
                else reflectBall(true, closest.x+ball.radius);
            }
            else
            {
                if (impactDirection.y > 0) reflectBall(false, closest.y-ball.radius);
                else reflectBall(false, closest.y+ball.radius);
            }
        }
    }   
    
    // check paddle collision
    glm::vec2 halfPlayerSize=player->size*0.5f;
    // find closest point on sprite
    glm::vec2 playerHitPos=bp-player->pos;    // vector to ball relative to player
    playerHitPos = glm::clamp(playerHitPos, -halfPlayerSize, halfPlayerSize);   // clamped to player size
    if (glm::length((playerHitPos+player->pos)-bp) < ball.radius) // hit
    {
        paddle->play();

        reflectBall(false, player->pos.y-halfPlayerSize.y-ball.radius);

        // check where it hit the board, and change velocity based on where it hit the board
        float percentage = playerHitPos.x / halfPlayerSize.x;
        // then move accordingly
        float strength = 1.0f;
        float oldVelocity = glm::length(ball.velocity);
        ball.velocity.x = InitialBallVelocity.x * percentage * strength; 
        ball.velocity = glm::normalize(ball.velocity) * oldVelocity;         
        if (activePowerup.type==PowerUp::Sticky)
        {
            ball.stickOffset=ball.sprite->pos.x - player->pos.x;
            ball.stuck=true;
        }
    } 
}

void Game::updatePowerups(float dt, PostProcess& post)
{
    activePowerup.timeLeft -= dt;

    PowerUp newPowerUp=activePowerup;
    if (activePowerup.timeLeft <= 0.0f)
    {
        newPowerUp={PowerUp::None, 0.0f};
    }

    for (auto&& p : floatingPowerups)
    {
        p->pos.y+=PowerupFallSpeed*dt;

        if (p->intersects(*player))
        {
            auto&& def=getPowerUpFromTexture(p->texture);
            newPowerUp.type = def.type;
            newPowerUp.timeLeft = def.duration;
            p->pos.y = fieldBR.y+p->size.y;
        }
    }
    erase_if(floatingPowerups, [limit=fieldBR.y](const auto& p) { return p->top()>limit; });

    if (activePowerup.type == newPowerUp.type)
    {
        activePowerup.timeLeft = max(activePowerup.timeLeft, newPowerUp.timeLeft);
    }
    else
    {
        switch (activePowerup.type)
        {
        case PowerUp::Speed: ball.velocity = glm::normalize(ball.velocity) * glm::length(InitialBallVelocity); break;
        case PowerUp::Size: player->size = InitialPlayerSize; break;
        case PowerUp::Confuse: post.confuse(0.0f); break;
        case PowerUp::Chaos: post.chaos(0.0f); break;
        default: break;
        }

        activePowerup.type = newPowerUp.type;
        activePowerup.timeLeft = newPowerUp.timeLeft;

        switch (activePowerup.type)
        {
        case PowerUp::Speed: ball.velocity *= PowerupBallVelocity; break;
        case PowerUp::Size: player->size = PowerUpPlayerSize; break;
        case PowerUp::Confuse: post.confuse(activePowerup.timeLeft); break;
        case PowerUp::Chaos: post.chaos(activePowerup.timeLeft); break;
        default:break;
        }

        auto&& def = getPowerUpFromType(activePowerup.type);
        player->texture = def.texture;
        player->color = def.color;
    }
}

void Game::maybeSpawnPowerups(const SpriteManager::Sprite& brick)
{
    float draw=SDL_randf();
    for (const auto& pd : powerupDefinitions)
    {
        if (draw<pd.chance)
        {
            floatingPowerups.emplace_back(sprites.createSprite(BackgroundLayer, brick->pos, pd.texture, PowerupSize, pd.color));
            return;
        }
        else
        {
            draw-=pd.chance;
        }
    }
}

void Game::forceSpawnPowerup(PowerUp::Type type, const glm::vec2& pos)
{
    auto&& pd=getPowerUpFromType(type);
    floatingPowerups.emplace_back(sprites.createSprite(BackgroundLayer, pos, pd.texture, PowerupSize, pd.color));
}

const Game::PowerUpDefinition& Game::getPowerUpFromTexture(SpriteManager::Texture texture)
{
    for (const auto& def : powerupDefinitions)
    {
        if (def.texture == texture) return def;
    }
    return getPowerUpFromType(PowerUp::None);
}

const Game::PowerUpDefinition& Game::getPowerUpFromType(Game::PowerUp::Type type)
{
    return powerupDefinitions[static_cast<size_t>(type)];
}

void Game::processInput(float dt)
{
    if (state == Active)
    {
        if (keys[SDL_SCANCODE_L])
        {
            nextLevel();
            keys[SDL_SCANCODE_L]=false;
        }

        for (int pu=PowerUp::Speed; pu<=PowerUp::MAX; ++pu)
        {
            if (keys[SDL_SCANCODE_1+pu-1])
            {
                forceSpawnPowerup(PowerUp::Type(pu), player->pos);
                keys[SDL_SCANCODE_1+pu-1]=false;
            }
        }

        float ds = PlayerVelocity * dt;
        // move playerboard
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        {
            player->pos.x = max(player->pos.x-ds, fieldTL.x+player->size.x*0.5f);
        }
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        {
            player->pos.x = min(player->pos.x+ds, fieldBR.x-player->size.x*0.5f);
        }

        if (keys[SDL_SCANCODE_SPACE] && ball.stuck)
        {
            ball.stuck=false;
            keys[SDL_SCANCODE_SPACE]=false;
            go->play();
        }
    }
}

void Game::draw(const vk::CommandBuffer& commandBuffer) const
{
    sprites.drawLayer(BackgroundLayer, commandBuffer);
    trail.draw(commandBuffer);
    sprites.drawLayer(GameLayer, commandBuffer);
    brickParts.draw(commandBuffer);
    sprites.drawLayer(ForegroundLayer, commandBuffer);

    font.renderText(commandBuffer, ScoreLabelPos, "SCORE");

    font.renderText(commandBuffer, ScorePos, format("{:05}", score));
}

void Game::reflectBall(bool horizontal, float limit)
{
   auto& bp=ball.sprite->pos;
    if (horizontal)
    {
        ball.velocity.x = -ball.velocity.x;
        bp.x=2.0f*limit-bp.x;
    }
    else
    {
        ball.velocity.y = -ball.velocity.y;
        bp.y=2.0f*limit-bp.y;
    }
}

void Game::nextLevel()
{
    if (curLevel==levelList.end()) curLevel=levelList.begin();
    else ++curLevel;
    if (curLevel==levelList.end()) curLevel=levelList.begin();

    level=make_unique<Level>(*curLevel, FieldPosition, BlockSize, sprites, GameLayer);
    resetPlayer();
}

void Game::resetPlayer()
{
    player->pos={(fieldTL.x+fieldBR.x)*0.5f, fieldBR.y-player->size.y};
    ball.stuck=true;
    ball.stickOffset=0.0f;
    ball.velocity=InitialBallVelocity;

    floatingPowerups.clear();
    activePowerup.timeLeft=0.0f;
}

void Game::explodeBrick(
    const glm::vec4& color,
    const glm::vec2& brickPos,
    const glm::vec2& brickSize,
    const glm::vec2& hitPoint,
    float velocity
)
{
    for (float y=-0.25f; y<0.5f; y+=0.5f)
    {
        for (float x=-0.375f; x<0.5f; x+=0.25f)
        {
            auto center=brickPos+glm::vec2{x,y}*brickSize;
            auto dir=glm::normalize(center-hitPoint);
            brickParts.spawnParticleP(
                1.0f,
                color,
                center,
                glm::vec2{brickSize.x*0.25f, brickSize.y*0.5f},
                glm::linearRand(0.0f, float(M_PI)*2.0f),
                dir*velocity,
                glm::linearRand(-float(M_PI),float(M_PI))*5.0f
            );

        }
    }
}
