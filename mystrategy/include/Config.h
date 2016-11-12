//
// Created by valdemar on 12.11.16.
//

#pragma once

namespace config {

//Насколько сильно отталкивать от препятствий
extern const double OBS_AVOID_PEEK_KOEF;
//Расстояние от конца препятствия, до начала нашего юнита, на котором действует сила обхода препятствий
extern const double OBS_AVOID_EXTRA_DISTANCE;
//С какой силой притягиваемся к следующему/предыдущему вейпоинту
extern const double NEXT_WAYPOINT_ATTRACTION;
extern const double PREV_WAYPOINT_ATTRACTION;

//Сколько тиков вперед учитывать, при подсчете наносимого урона
extern const int DMG_INCOME_TICKS_SIMULATION;
//Запас, между зоной удара противника и нашим краем, при рассчете поля урона
extern const int DMG_AVOID_EXTRA_DISTANCE;

//На каком расстоянии начинать обращать внимание на врагов
extern const double ENEMY_DETECT_RANGE;

//С какой силой притягиваться к радиусу удара к выбраному врагу
extern const double CHOOSEN_ENEMY_ATTRACT;

//Пиковое значение боязни смерти. Применяется как максимальное значение в экспоненциальной формуле избегания урона
extern const double DEAD_PEEK_FEARING;
//Насколько сильно боимся подойти к краю зоны потенциальной смерти
extern const double DAMAGE_ZONE_END_FORCE;

};
