//
// Created by valdemar on 05.12.16.
//

#pragma once

#include "model/Building.h"

class MyBuilding {
public:

    MyBuilding(double x, double y, double radius, int damage, double attack_range, int max_life, int cooldown);

    void update(const model::Building &building);

    void updateClock(int current_tick);

    bool isSame(const model::Building &building);

    int getRemainingActionCooldownTicks() const;

    double getX() const;

    double getY() const;

    geom::Point2D getPoint() const;

    double getRadius() const;

    int getDamage() const;

    double getAttackRange() const;

    int getLife() const;

    int getMaxLife() const;

    int getCooldown() const;

private:
    double m_x;
    double m_y;
    double m_radius;
    int m_damage;
    double m_attack_range;
    int m_life;
    int m_max_life;
    int m_cooldown;
    int m_last_update;
    int m_last_shoot_tick;
};
