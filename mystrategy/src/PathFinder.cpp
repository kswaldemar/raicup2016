//
// Created by valdemar on 11.11.16.
//

#include "PathFinder.h"
#include "Logger.h"
#include "VisualDebug.h"
#include "Config.h"
#include "model/LaneType.h"

#include <map>
#include <cassert>
#include <queue>

using geom::Point2D;

PathFinder::PathFinder(const InfoPack &info) {
    m_i = &info;

    const double map_size = m_i->g->getMapSize();

    static const std::vector<Point2D> middle_wp = {
        {600,  map_size - 600},
        {1000, map_size - 1000},
        {1400, map_size - 1400},
        {1800, map_size - 1800},
        {2200, map_size - 2200},
        {2600, map_size - 2600},
        {3000, map_size - 3000},
        {3300, map_size - 3300},
        {3600, map_size - 3600},
    };

    static const std::vector<Point2D> top_wp = {
        {200.0,  map_size - 600.0},
        {200.0,  map_size - 1000.0},
        {200.0,  map_size - 1400.0},
        {200.0,  map_size - 1800.0},
        {200.0,  map_size - 2200.0},
        {200.0,  map_size - 2600.0},
        {200.0,  map_size - 3000.0},
        {500,    500.0},
        {600.0,  200.0},
        {1000.0, 200.0},
        {1400.0, 200.0},
        {1800.0, 200.0},
        {2200.0, 200.0},
        {2600.0, 200.0},
        {3000.0, 200.0},
        {3600.0, 400.0}
    };

    static const std::vector<Point2D> bottom_wp = {
        {600.0,            map_size - 200.0},
        {1000.0,           map_size - 200.0},
        {1400.0,           map_size - 200.0},
        {1800.0,           map_size - 200.0},
        {2200.0,           map_size - 200.0},
        {2600.0,           map_size - 200.0},
        {3000.0,           map_size - 200.0},
        {map_size - 500,   map_size - 500.0},
        {map_size - 200.0, 3000.0},
        {map_size - 200.0, 2600.0},
        {map_size - 200.0, 2200.0},
        {map_size - 200.0, 1800.0},
        {map_size - 200.0, 1400.0},
        {map_size - 200.0, 1000.0},
        {map_size - 200.0, 600.0},
        {3600.0,           400.0}
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
    const bool wall = pt.x <= radius || pt.x >= m_i->g->getMapSize() - radius
                      || pt.y <= radius || pt.y >= m_i->g->getMapSize() - radius;
    if (wall) {
        return false;
    }
    double sqrradius;
    geom::Vec2D dist;
    for (const auto &obs : m_obstacles) {
        sqrradius = radius + obs->getRadius();
        sqrradius = sqrradius * sqrradius;
        dist.x = obs->getX() - pt.x;
        dist.y = obs->getY() - pt.y;
        if (dist.sqr() <= sqrradius) {
            VISUAL(fillCircle(pt.x, pt.y, 3, 0xff9933));
            return false;
        }
    }
    //VISUAL(fillCircle(pt.x, pt.y, 3, 0x33cc33));
    return true;

}

std::list<geom::Point2D> PathFinder::find_way(const geom::Point2D &to, double radius) {
    const double ex_r = m_i->s->getRadius() + 0.1;

    //Clear map
    for (int i = 0; i < m_map.size(); ++i) {
        for (int j = 0; j < m_map[i].size(); ++j) {
            m_map[i][j] = {{i, j}, nullptr, 1e9};
        };
    }
    //Clear closed cells
    static std::array<std::array<bool, CELL_COUNT>, CELL_COUNT> closed;
    for (auto &column : closed) {
        column.fill(false);
    }

    //Heuristic function for A-star
    const auto &dmap = m_danger_map;
    const auto astar_h = [&dmap](const Point2D &cur, const Point2D &target) {
        return (target - cur).len() + dmap->get_value(cur);
    };

    struct CCWithCost {
        CCWithCost(const CellCoord &pt_, double cost_)
            : pt(pt_),
              cost(cost_) {
        }

        CellCoord pt;
        double cost;

        bool operator>(const CCWithCost &other) const {
            return cost > other.cost;
        }
    };

    std::priority_queue<CCWithCost, std::vector<CCWithCost>, std::greater<CCWithCost>> open;
    CellCoord initial;
    initial = world_to_cell({m_i->s->getX(), m_i->s->getY()});
    m_map[initial.x][initial.y].cost = 0;

    open.emplace(initial, m_map[initial.x][initial.y].cost + astar_h(cell_to_world(initial), to));

    int visited_cells = 0;
    const auto sqrradius = radius * radius;
    const Cell *found = nullptr;
    bool first = true;
    CellCoord nearest = initial;
    int min_manh = 100000000;
    const CellCoord cell_target = world_to_cell(to);
    while (!open.empty()) {
        CellCoord next = open.top().pt;
        //LOG("Looking cell (%d, %d) with cost %lf\n", next.x, next.y, open.top().cost);
        open.pop();
        ++visited_cells;

        //Remember nearest cell, to at least move if cannot find path
        const int manh = std::abs(cell_target.x - next.x) + std::abs(cell_target.y - next.y);
        if (min_manh > manh) {
            min_manh = manh;
            nearest = next;
        }

        if (visited_cells > ASTAR_MAX_VISIT) {
            break;
        }

        Point2D wnext = cell_to_world(next);
        if (closed[next.x][next.y] || !is_correct_cell(next, initial) || (!first && !check_no_collision(wnext, ex_r))) {
            continue;
        }
        first = false;

        //Check for goal
        const geom::Vec2D dist = to - wnext;
        if (dist.sqr() <= sqrradius) {
            found = &m_map[next.x][next.y];
            break;
        }
        //Don't need to visit it anymore
        closed[next.x][next.y] = true;
        //For each neighbor
        static const std::initializer_list<CellCoord> shifts = {
            {0,  -1},
            {1,  -1},
            {1,  0},
            {1,  1},
            {0,  1},
            {-1, 1},
            {-1, 0},
            {-1, -1}
        };
        CellCoord neigh;
        for (const auto &shift: shifts) {
            neigh = next + shift;
            if (!closed[neigh.x][neigh.y]) {
                //Update vertex
                if (update_cost(next, neigh)) {
                    double updated_cost = m_map[neigh.x][neigh.y].cost;
                    double h = astar_h(cell_to_world(neigh), to);
                    open.emplace(neigh, updated_cost + h);
                }
            }
        }
    }

    std::list<Point2D> ret;

    const Cell *next = nullptr;
    if (found) {
        ret.push_front(cell_to_world(found->me));
        next = found->parent;
    } else {
        ret.push_front(cell_to_world(nearest));
        next = &m_map[nearest.x][nearest.y];
    }

    while (next) {
        ret.push_front(cell_to_world(next->me));
        next = next->parent;
    }

    //LOG("Way to point (%3.1lf, %3.1lf) %s. Vertexes visited %d\n",
    //    to.x,
    //    to.y,
    //    found ? "found" : "not found",
    //    visited_cells);

    return ret;
}

bool PathFinder::is_correct_cell(const PathFinder::CellCoord &tocheck, const PathFinder::CellCoord &initial) {
    const bool inbound = tocheck.x >= 0 && tocheck.x < CELL_COUNT && tocheck.y >= 0 && tocheck.y < CELL_COUNT;
    return inbound;
}

PathFinder::CellCoord PathFinder::world_to_cell(const Point2D &wpt) {
    return {static_cast<int>(round(wpt.x / GRID_SIZE)), static_cast<int>(round(wpt.y / GRID_SIZE))};
}

geom::Point2D PathFinder::cell_to_world(const PathFinder::CellCoord &cpt) {
    return Point2D(cpt.x * GRID_SIZE, cpt.y * GRID_SIZE);
}

bool PathFinder::update_cost(const PathFinder::CellCoord &pt_from, const PathFinder::CellCoord &pt_to) {
    const double D2 = sqrt(2) * GRID_SIZE;
    const double D = GRID_SIZE;
    int dx = std::abs(pt_from.x - pt_to.x);
    int dy = std::abs(pt_from.y - pt_to.y);
    double mv_cost = D * (dx + dy) + (D2 - 2 * D) * std::min(dx, dy);

    const Cell &cfrom = m_map[pt_from.x][pt_from.y];
    Cell &cto = m_map[pt_to.x][pt_to.y];

    if (cfrom.cost + mv_cost < cto.cost) {
        cto.parent = &cfrom;
        cto.cost = cfrom.cost + mv_cost;
        return true;
    }
    return false;
}

bool PathFinder::bonuses_is_under_control() const {
    long wp_idx = m_last_wp - m_waypoints->cbegin();
    switch (m_i->s->getId()) {
        case 1:
        case 2:
        case 6:
        case 7:
            return wp_idx >= 5;
        case 3:
        case 8:
            return wp_idx >= 2;
        case 4:
        case 5:
        case 9:
        case 10:
            return wp_idx >= 5;
            break;
        default:break;
    }
}
