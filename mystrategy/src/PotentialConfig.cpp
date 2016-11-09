//
// Created by valdemar on 09.11.16.
//

#include "PotentialConfig.h"

#include <cstdio>

namespace potential {

/**
 * Config values
 */
//Linear
const int WAYPOINT = 8'000;
//Hyberbolic
const int OBSTACLE = 1000;

/**
 * Functions implementations
 */

geom::Vec2D waypoint_attraction(const geom::Vec2D &waypoint_vector) {
    double x = waypoint_vector.len();
    double y = WAYPOINT - x;
    if (y < 0) {
        return {0, 0};
    }
    return geom::normalize(waypoint_vector) *= y;
}

geom::Vec2D obstacle_avoidance(double radius, const geom::Vec2D &rp) {
    double x = rp.len();
    double y = OBSTACLE * radius / (x);
    if (x > radius) {
        y = 0;
    }
    if (y <= 0) {
        return {0, 0};
    }
    return geom::normalize(rp) *= y;
}

}