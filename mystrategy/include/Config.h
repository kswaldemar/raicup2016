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

//Сколько тиков симулировать нанесение урона, при оценке ценности уничтожения противника
extern const int DMG_SIMULATION_TICKS;

//Запас, между зоной удара противника и нашим краем, при рассчете поля урона
extern const int DMG_AVOID_EXTRA_DISTANCE;

//На каком расстоянии начинать обращать внимание на врагов
extern const double ENEMY_DETECT_RANGE;

//С какой силой притягиваться к радиусу удара к выбраному врагу
extern const double CHOOSEN_ENEMY_ATTRACT;

//Насколько сильно мы боимся смерти. Умножается на радиус зоны возможной смерти в формуле экспоненциального поля избегания урона
extern const double DEAD_PEEK_KOEF;
//Насколько сильно боимся подойти к краю зоны потенциальной смерти
extern const double DAMAGE_ZONE_END_FORCE;

};
