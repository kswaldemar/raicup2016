//
// Created by valdemar on 09.11.16.
//

#pragma once

#include "Vec2D.h"

namespace potential {

geom::Vec2D waypoint_attraction(const geom::Vec2D &waypoint_vector);

geom::Vec2D obstacle_avoidance(double r, const geom::Vec2D &rp);

}
