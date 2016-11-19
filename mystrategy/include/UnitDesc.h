//
// Created by valdemar on 18.11.16.
//

#pragma once

#include "model/Building.h"

/**
 * Base class to describe any living unit in uniform way
 */
//class UnitDesc {
//public:
//    enum class Type {
//        WIZARD,
//        ORC,
//        FETISH,
//        TOWER,
//        BASE
//    };
//
//    UnitDesc() = delete;
//
//    UnitDesc(const model::Building &build);
//
//private:
//
//    double x;
//    double y;
//    double r;
//    int id;
//    Type type;
//    model::Faction faction;
//    double shooting_range;
//    int rem_cooldown;
//    int cooldown;
//};

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
