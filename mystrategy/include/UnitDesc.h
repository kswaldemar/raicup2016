//
// Created by valdemar on 18.11.16.
//

#pragma once

#include "Vec2D.h"
#include "model/Building.h"

/**
 * Values to calculate attack rate of the unit
 */
struct AttackUnit {
    int rem_cooldown;
    int cooldown;
    int dmg;
    double range;
    double speed;
};

struct RunawayUnit {
    int life;
    int max_life;
    double speed;
};
