#pragma once

class IDamagable
{
public:
    virtual ~IDamagable() = 0;
    virtual void Damage(int amount) = 0;
};