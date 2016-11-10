//
// Created by valdemar on 09.11.16.
//

#include "PotentialConfig.h"

#include <cstdio>
#include <initializer_list>

namespace potential {

/**
 * Config values
 * Vector field speed (value of each vector in field)
 */
const int WAYPOINT = 200;


/**
 * Get equivalent potential field value in particular point in vector field
 */
inline double get_value(double field_radius, double distance_to_center, double field_speed) {
    if (field_radius < distance_to_center) {
        return 0;
    }
    return (field_radius - distance_to_center) * field_speed;
}

/**
 * Functions implementations
 */

geom::Vec2D waypoint_attraction(const geom::Vec2D &waypoint_vector) {
    return geom::normalize(waypoint_vector) *= WAYPOINT;
    //return {0, 0};
}

geom::Vec2D obstacle_avoidance(double radius, const geom::Vec2D &rp) {
    double x = rp.len();
    x -= radius;
    double y = 0;
    static const std::initializer_list<geom::Vec2D> fn {
        { 5, 20000 },
        { 10, 2000 },
        { 15, 100 },
        { 20, 50 },
        { 25, 25 },
        { 30, 15 },
        { 35, 10 }
    };

    for (const auto &i : fn) {
        if (x <= i.x) {
            y = i.y;
            break;
        }
    }

    return geom::normalize(rp) *= y;
}

}