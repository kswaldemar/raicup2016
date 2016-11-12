//
// Created by valdemar on 12.11.16.
//

#pragma once

namespace config {

//Кривизна поля отталкивания от препятствий
extern const double OBS_AVOID_CURVATURE;
//С какой силой притягиваемся к следующему/предыдущему вейпоинту
extern const double NEXT_WAYPOINT_ATTRACTION;
extern const double PREV_WAYPOINT_ATTRACTION;

//Сколько тиков симулировать нанесение урона, при оценке ценности уничтожения противника
extern const int DMG_SIMULATION_TICKS;

//На каком расстоянии начинать обращать внимание на врагов
extern const double ENEMY_DETECT_RANGE;

//С какой силой притягиваться к радиусу удара к выбраному врагу
extern const double CHOOSEN_ENEMY_ATTRACT;

//Максимальное значение страха, если сила поля урона в точке достигает этого значения, значит нас могут убить за 1 такт
extern const double DAMAGE_MAX_FEAR;
//Кривизна экспоненты боязни смерти
extern const double DAMAGE_ZONE_CURVATURE;

};
