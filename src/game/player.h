#pragma once

#include "core/camera.h"
#include "game/entity.h"
#include "game/interfaces.h"
#include "input/input.h"

enum class WeaponType
{
    Primary = 0,
    Secondary,
    Melee,
    Grenade,
    Count,
};

class Player : public Entity, public IDamagable
{
public:
    Player(Input &inputManager, float aspectRatio);

    virtual void Update(float deltaTime) override;

    virtual void Damage(int amount) override;
    void Shoot();

private:
    Camera m_camera;
    Input &m_inputManager;
};