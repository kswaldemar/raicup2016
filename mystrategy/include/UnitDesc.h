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

struct SimpleCircle {

    enum Type {
        STATIC, ///< Cannot move by definition (tree or building)
        DYNAMIC, ///< Anything else
        TYPES_COUNT,
    };

    SimpleCircle(Type type_, double x_, double y_, double r_) : type(type_), x(x_), y(y_), r(r_) {}

    Type type;
    double x;
    double y;
    double r;
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
