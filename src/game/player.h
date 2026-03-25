#pragma once

#include "core/camera.h"
#include "game/character.h"
#include "game/interfaces.h"
#include "input/input_manager.h"

enum class WeaponType
{
    Primary = 0,
    Secondary,
    Melee,
    Grenade,
    Count,
};

class Player : public Character, public IDamagable
{
public:
    Player(InputManager &inputManager, float aspectRatio);

    virtual void Update(float deltaTime) override;

    virtual void Damage(int amount) override;
    void Shoot();

private:
    Camera camera_;
    InputManager &inputManager_;
};