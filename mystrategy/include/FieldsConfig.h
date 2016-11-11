//
// Created by valdemar on 12.11.16.
//

#pragma once

#include "FieldsDescription.h"

struct FieldsConfig {
    //Насколько сильно отталкивать от препятствий
    static constexpr double OBS_AVOID_PEEK_KOEF = 100;
    //Расстояние от конца препятствия, до начала нашего юнита, на котором действует сила обхода препятствий
    static constexpr double OBS_AVOID_EXTRA_DISTANCE = 35;
    //С какой силой притягиваемся к вейпоинту
    static constexpr double WAYPOINT_ATTRACTION = 40;
};
