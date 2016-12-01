//
// Created by valdemar on 12.11.16.
//

#include "Config.h"

namespace config {

const double ENEMY_DETECT_RANGE = 600;

const double ATTACK_THRESH = 55;
const double SCOUT_THRESH = 50;
const double BONUS_EARN_THRESH = 40;

const double DAMAGE_MAX_FEAR = 100;
/*
 * Порядок коэффициентов: коэффициент - падение напряженности поля со 100, за расстояние 100
 * 0.1  - (на расстоянии 40 уменьшилась на 98.2
 * 0.01 - 73
 * 0.001 - 9.5
 */
const double DAMAGE_ZONE_CURVATURE = 0.0021; //0.0017

}