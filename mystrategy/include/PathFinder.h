//
// Created by valdemar on 11.11.16.
//

#pragma once

#include "InfoPack.h"
#include "Vec2D.h"

//Synonim with another meaning
using Point2D = geom::Vec2D;

/**
 * Path find and moving related class
 */
class PathFinder {
public:
    static const int WAYPOINT_RADIUS;

    PathFinder(const InfoPack &info);

    void update_info_pack(const InfoPack &info);

    Point2D get_next_waypoint() const;

    Point2D get_previous_waypoint() const;

private:
    const std::vector<Point2D> *m_waypoints = nullptr;
    const InfoPack *m_i;
};
