//
// Created by valdemar on 11.11.16.
//

#include "PathFinder.h"
#include "model/LaneType.h"
#include "Config.h"

#include <map>
#include <cassert>
#include <queue>
#include <VisualDebug.h>
#include <Logger.h>

const int PathFinder::WAYPOINT_RADIUS = 200;

using geom::Point2D;

PathFinder::PathFinder(const InfoPack &info) {
    m_i = &info;

    const double map_size = m_i->g->getMapSize();

    static const std::vector<Point2D> middle_wp = {
        {600,            map_size - 600},
        {1000,           map_size - 1000},
        {1400,           map_size - 1400},
        {1800,           map_size - 1800},
        {2200,           map_size - 2200},
        {2600,           map_size - 2600},
        {3000,           map_size - 3000},
        {3300,           map_size - 3300},
        {3600,           map_size - 3600},
    };

    static const std::vector<Point2D> top_wp = {
        {200.0, map_size - 600.0},
        {200.0, map_size - 1000.0},
        {200.0, map_size - 1400.0},
        {200.0, map_size - 1800.0},
        {200.0, map_size - 2200.0},
        {200.0, map_size - 2600.0},
        {200.0, map_size - 3000.0},
        {500, 500.0},
        {600.0,  200.0},
        {1000.0, 200.0},
        {1400.0, 200.0},
        {1800.0, 200.0},
        {2200.0, 200.0},
        {2600.0, 200.0},
        {3000.0, 200.0},
    };

    static const std::vector<Point2D> bottom_wp = {
        {600.0, map_size - 200.0},
        {1000.0, map_size - 200.0},
        {1400.0, map_size - 200.0},
        {1800.0, map_size - 200.0},
        {2200.0, map_size - 200.0},
        {2600.0, map_size - 200.0},
        {3000.0, map_size - 200.0},
        {500,   500.0},
        {map_size - 200.0, 600.0},
        {map_size - 200.0, 1000.0},
        {map_size - 200.0, 1400.0},
        {map_size - 200.0, 1800.0},
        {map_size - 200.0, 2200.0},
        {map_size - 200.0, 2600.0},
        {map_size - 200.0, 3000.0},
    };

    switch (m_i->s->getId()) {
        case 1:
        case 2:
        case 6:
        case 7:m_waypoints = &top_wp;
            break;
        case 3:
        case 8:m_waypoints = &middle_wp;
            break;
        case 4:
        case 5:
        case 9:
        case 10:m_waypoints = &bottom_wp;
            break;
        default:break;
    }
    assert(m_waypoints);
    m_next_wp = m_waypoints->cbegin();
    m_last_wp = m_waypoints->cbegin();
}

void PathFinder::update_info(const InfoPack &info, const fields::FieldMap &danger_map) {
    m_i = &info;
    m_danger_map = &danger_map;
    //Remember each obstacle
    m_obstacles.clear();
    for (const auto &i : m_i->w->getTrees()) {
        m_obstacles.push_back(&i);
    }
    for (const auto &i : m_i->w->getBuildings()) {
        m_obstacles.push_back(&i);
    }
    for (const auto &i : m_i->w->getWizards()) {
        if (i.isMe()) {
            continue;
        }
        m_obstacles.push_back(&i);
    }
    for (const auto &i : m_i->w->getMinions()) {
        m_obstacles.push_back(&i);
    }
}

Point2D PathFinder::get_next_waypoint() {
    if (m_i->s->getDistanceTo(m_next_wp->x, m_next_wp->y) <= WAYPOINT_RADIUS) {
        m_last_wp = m_next_wp++;
        if (m_next_wp == m_waypoints->cend()) {
            --m_next_wp;
        }
    }
    return *m_next_wp;
}

Point2D PathFinder::get_previous_waypoint() {
    if (m_i->s->getDistanceTo(m_last_wp->x, m_last_wp->y) <= WAYPOINT_RADIUS) {
        m_next_wp = m_last_wp;
        if (m_last_wp != m_waypoints->cbegin()) {
            m_last_wp--;
        }
    }
    return *m_last_wp;
}

void PathFinder::move_along(const geom::Vec2D &dir, model::Move &move, bool hold_face) {
    const double turn_angle = m_i->s->getAngleTo(m_i->s->getX() + dir.x, m_i->s->getY() + dir.y);
    double strafe = sin(turn_angle);
    double f_b = cos(turn_angle);

    strafe *= m_i->g->getWizardStrafeSpeed();
    if (f_b > 0) {
        f_b *= m_i->g->getWizardForwardSpeed();
    } else {
        f_b *= m_i->g->getWizardBackwardSpeed();
    }

    //Checking for haste
    const auto &statuses = m_i->s->getStatuses();
    for (const auto &st : statuses) {
        if (st.getType() == model::STATUS_HASTENED) {
            strafe *= 1.0 + m_i->g->getHastenedMovementBonusFactor();
            f_b *= 1.0 + m_i->g->getHastenedMovementBonusFactor();
        }
    }

    move.setStrafeSpeed(strafe);
    move.setSpeed(f_b);

    if (!hold_face) {
        move.setTurn(turn_angle);
    }
}

bool PathFinder::check_no_collision(const Point2D &pt, double radius) const {
    const bool wall =    pt.x <= radius || pt.x >= m_i->g->getMapSize() - radius
                      || pt.y <= radius || pt.y >= m_i->g->getMapSize() - radius;
    if (wall) {
        return false;
    }
    for (const auto &obs : m_obstacles) {
        if (obs->getDistanceTo(pt.x, pt.y) <= radius + obs->getRadius()) {
            VISUAL(fillCircle(pt.x, pt.y, 3, 0xff9933));
            return false;
        }
    }
    //VISUAL(fillCircle(pt.x, pt.y, 3, 0x33cc33));
    return true;

}

std::list<Point2D> PathFinder::find_way(const geom::Point2D &to, double radius, double max_danger) {
    const double ex_r = m_i->s->getRadius();

    //Clear map
    for (int i = 0; i < m_map.size(); ++i) {
        for (int j = 0; j < m_map[i].size(); ++j) {
            m_map[i][j] = {1e9, 1e6, {0, 0}, {i, j}};
        };
    }

    std::queue<CellCoord> way;
    CellCoord start = world_to_cell({m_i->s->getX(), m_i->s->getY()});
    //auto wpt = cell_to_world(start);
    //VISUAL(fillCircle(wpt.x, wpt.y, 7, 0xe6005c));
    m_map[start.x][start.y].dist = 0;
    way.push(start);
    const Cell *best = nullptr;
    bool first = true;
    while (!way.empty()) {
        CellCoord next = way.front();
        way.pop();
        if (!is_correct_cell(next, start)) {
            continue;
        }

        Cell *cell = &m_map[next.x][next.y];

        if (first || check_no_collision(Point2D(next.x * GRID_SIZE, next.y * GRID_SIZE), ex_r)) {
            first = false;
            const double dist_to_target = geom::Vec2D{to.x - next.x * GRID_SIZE, to.y - next.y * GRID_SIZE}.len();
            if (dist_to_target <= radius && (best == nullptr || *cell > *best)) {
                //Found new best
                best = cell;
            }
            static const std::initializer_list<CellCoord> shifts = {
                {0, -1},
                {1, -1},
                {1, 0},
                {1, 1},
                {0, 1},
                {-1, 1},
                {-1, 0},
                {-1, -1},
            };
            for (const auto & shift: shifts) {
                CellCoord cand_coord{next.x + shift.x, next.y + shift.y};
                if (is_correct_cell(cand_coord, start)) {
                    Cell *candidate = &m_map[cand_coord.x][cand_coord.y];
                    if (update_if_better(*cell, *candidate)) {
                        way.push({next.x + shift.x, next.y + shift.y});
                    }
                }
            }
        }
    }

    std::list<Point2D> ret;
    if (best) {
        ret.push_front(cell_to_world(best->me));
        CellCoord prev = best->me;
        CellCoord next;
        while (true) {
            const Cell *pcell = &m_map[prev.x][prev.y];
            next = pcell->me + pcell->parent_shift;
            if (next.x == start.x && next.y == start.y) {
                break;
            }
            ret.push_front(cell_to_world(next));
            VISUAL(line(next.x * GRID_SIZE, next.y * GRID_SIZE, prev.x * GRID_SIZE, prev.y * GRID_SIZE, 0x0000ff));
            prev = next;
        }
    }
    return ret;
}

bool PathFinder::is_correct_cell(const PathFinder::CellCoord &tocheck, const PathFinder::CellCoord &initial) {
    const bool inbound = tocheck.x >= 0 && tocheck.x < CELL_COUNT && tocheck.y >= 0 && tocheck.y < CELL_COUNT;
    geom::Vec2D dist(tocheck.x - initial.x, tocheck.y - initial.y);
    return inbound && dist.len() <= SEARCH_RADIUS;
}

bool PathFinder::update_if_better(PathFinder::Cell &from, PathFinder::Cell &to) {
    static constexpr double diag_cost = 1.4142135623730951 * GRID_SIZE;
    int manh = std::abs(to.me.x - from.me.x) + std::abs(to.me.y - from.me.y);
    double dist;
    if (manh > 1) {
        assert(manh == 2);
        dist = diag_cost;
    } else {
        assert(manh == 1);
        dist = manh * GRID_SIZE;
    }
    PathFinder::Cell candidate = from;
    candidate.dist += dist;
    //Sum dangers
    candidate.danger += m_danger_map->get_value(cell_to_world(from.me));
    if (candidate > to) {
        to.dist = candidate.dist;
        to.danger = candidate.danger;
        to.parent_shift.x = from.me.x - to.me.x;
        to.parent_shift.y = from.me.y - to.me.y;
        return true;
    }
    return false;
}

PathFinder::CellCoord PathFinder::world_to_cell(const Point2D &wpt) {
    return {static_cast<int>(round(wpt.x / GRID_SIZE)), static_cast<int>(round(wpt.y / GRID_SIZE))};
}

geom::Point2D PathFinder::cell_to_world(const PathFinder::CellCoord &cpt) {
    return Point2D(cpt.x * GRID_SIZE, cpt.y * GRID_SIZE);
}
