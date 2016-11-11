//
// Created by valdemar on 11.11.16.
//

#include "PathFinder.h"
#include "model/LaneType.h"

#include <map>
#include <cassert>

const int PathFinder::WAYPOINT_RADIUS = 100;

using geom::Point2D;

PathFinder::PathFinder(const InfoPack &info) {
    m_i = &info;

    const double map_size = m_i->g->getMapSize();

    static const std::vector<Point2D> middle_wp = {
        {100, map_size - 100},
        {600, map_size - 200},
        {800, map_size - 800},
    };

    static const std::vector<Point2D> top_wp = {
        {100.0,            map_size - 100.0},
        {100.0,            map_size - 400.0},
        {200.0,            map_size - 800.0},
        {200.0,            map_size * 0.75},
        {200.0,            map_size * 0.5},
        {200.0,            map_size * 0.25},
        {200.0,            200.0},
        {map_size * 0.25,  200.0},
        {map_size * 0.5,   200.0},
        {map_size * 0.75,  200.0},
        {map_size - 200.0, 200.0}};

    static const std::vector<Point2D> bottom_wp = {
        {100.0,            map_size - 100.0},
        {400.0,            map_size - 100.0},
        {800.0,            map_size - 200.0},
        {map_size * 0.25,  map_size - 200.0},
        {map_size * 0.5,   map_size - 200.0},
        {map_size * 0.75,  map_size - 200.0},
        {map_size - 200.0, map_size - 200.0},
        {map_size - 200.0, map_size * 0.75},
        {map_size - 200.0, map_size * 0.5},
        {map_size - 200.0, map_size * 0.25},
        {map_size - 200.0, 200.0}};

    switch (m_i->s->getId()) {
        case 1:
        case 2:
        case 6:
        case 7:
            m_waypoints = &top_wp;
            break;
        case 3:
        case 8:
            m_waypoints = &middle_wp;
            break;
        case 4:
        case 5:
        case 9:
        case 10:
            m_waypoints = &bottom_wp;
            break;
        default:
        break;
    }
    assert(m_waypoints);
}

void PathFinder::update_info_pack(const InfoPack &info) {
    m_i = &info;
}

Point2D PathFinder::get_next_waypoint() const {
    using WpIter = std::vector<Point2D>::const_iterator;

    WpIter last_wp = m_waypoints->cend();
    --last_wp;

    for (WpIter i = m_waypoints->cbegin(); i != last_wp; ++i) {
        if (m_i->s->getDistanceTo(i->x, i->y) <= WAYPOINT_RADIUS) {
            return *(++i);
        }
        geom::Vec2D wpdist = *last_wp - *i;
        if (wpdist.len() < m_i->s->getDistanceTo(last_wp->x, last_wp->y)) {
            return *i;
        }
    }

    return *last_wp;
}

Point2D PathFinder::get_previous_waypoint() const {
    using WpIter = std::vector<Point2D>::const_reverse_iterator;

    WpIter first_wp = m_waypoints->crend();
    --first_wp;

    for (WpIter i = m_waypoints->crbegin(); i != first_wp; ++i) {
        if (m_i->s->getDistanceTo(i->x, i->y) <= WAYPOINT_RADIUS) {
            return *(++i);
        }
        geom::Vec2D wpdist = *first_wp - *i;
        if (wpdist.len() < m_i->s->getDistanceTo(first_wp->x, first_wp->y)) {
            return *i;
        }
    }

    return *first_wp;
}
