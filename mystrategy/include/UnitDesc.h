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

struct BonusDesc {
    geom::Point2D pt;
    int time; //Remaining time before appearing
};

/**
 * Invisible tower description
 */
class TowerDesc {
public:
    long long int id = -1;
    double x;
    double y;
    double r;
    int rem_cooldown;
    int cooldown;
    int life;
    int max_life;
    double attack_range;
    int damage;
};
