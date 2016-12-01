//
// Created by valdemar on 12.11.16.
//

#pragma once

namespace config {



//На каком расстоянии начинать обращать внимание на врагов
extern const double ENEMY_DETECT_RANGE;


//Максимальное значение опасности для атаки
extern const double ATTACK_THRESH;

//Максимальное значение опасности для движения по вейпоинтам
extern const double SCOUT_THRESH;

//Максимальное значение страха, если сила поля урона в точке достигает этого значения, значит нас могут убить за 1 такт
extern const double DAMAGE_MAX_FEAR;
//Кривизна экспоненты боязни смерти
extern const double DAMAGE_ZONE_CURVATURE;

extern const double BONUS_EARN_THRESH;

};
