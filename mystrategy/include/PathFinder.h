//
// Created by valdemar on 11.11.16.
//

#pragma once

#include "model/Move.h"
#include "InfoPack.h"
#include "Vec2D.h"

//Synonim with another meaning
/**
 * Path find and moving related class
 */
class PathFinder {
public:
    static const int WAYPOINT_RADIUS;

    PathFinder(const InfoPack &info);


    void update_info_pack(const InfoPack &info);

    geom::Point2D get_next_waypoint() const;

    geom::Point2D get_previous_waypoint() const;

    void move_along(const geom::Vec2D &dir, model::Move &move, bool hold_face);

private:
    const std::vector<geom::Point2D> *m_waypoints = nullptr;
    const InfoPack *m_i;
};
