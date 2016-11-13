//
// Created by valdemar on 12.11.16.
//

#include "Config.h"

namespace config {

//0.02 - отталкивание на радиусе = 17.26
const double OBS_AVOID_CURVATURE = 0.02;
const double OBS_AVOID_EXTRA_DISTANCE = 50;

const double NEXT_WAYPOINT_ATTRACTION = 20;

const double PREV_WAYPOINT_ATTRACTION = 5; // Завышение поля опасности, если отходим от последнего вейпоинта (будет влиять на привлечение к противнику)

const double ENEMY_DETECT_RANGE = 600;

const double CHOOSEN_ENEMY_ATTRACT = 50;


const double DAMAGE_MAX_FEAR = 100;
/*
 * Порядок коэффициентов: коэффициент - падение напряженности поля со 100, за расстояние 100
 * 0.1  - (на расстоянии 40 уменьшилась на 98.2
 * 0.01 - 73
 * 0.001 - 9.5
 */
const double DAMAGE_ZONE_CURVATURE = 0.0023; //0.0017



//240 - good
const int DMG_SIMULATION_TICKS = 360;

}