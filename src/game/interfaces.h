#pragma once

class IDamagable
{
public:
    virtual ~IDamagable() = default;
    virtual void Damage(int amount) = 0;
};