//
// Created by valdemar on 05.12.16.
//

#pragma once

#include "MyLivingUnit.h"
#include "model/Building.h"

class MyBuilding : public MyLivingUnit {
public:

    MyBuilding(double x, double y, double radius, int damage, double attack_range, int max_life, int cooldown);

    void update(const model::Building &building);

    void updateClock(int current_tick);

    bool isSame(const model::Building &building);

    int getRemainingActionCooldownTicks() const;

    int getDamage() const;

    double getAttackRange() const;

    int getCooldown() const;

private:
    int m_damage;
    double m_attack_range;
    int m_cooldown;
    int m_last_update;
    int m_last_shoot_tick;
};
