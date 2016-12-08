//
// Created by valdemar on 11.11.16.
//

#include "PathFinder.h"
#include "Logger.h"
#include "VisualDebug.h"
#include "BehaviourConfig.h"
#include "model/LaneType.h"

#include <map>
#include <cassert>
#include <queue>

using geom::Point2D;

PathFinder::PathFinder(const InfoPack &info) {
    m_i = &info;
    const double map_size = m_i->g->getMapSize();
    switch (m_i->s->getId()) {
        case 1:
        case 2:
        case 6:
        case 7:m_current_lane = model::LANE_TOP;
            break;
        case 3:
        case 8:m_current_lane = model::LANE_MIDDLE;
            break;
        case 4:
        case 5:
        case 9:
        case 10:m_current_lane = model::LANE_BOTTOM;
            break;
        default:break;
    }

    m_last_wp[model::LANE_TOP] = {200, map_size - 800 - WAYPOINT_RADIUS};
    m_last_wp[model::LANE_MIDDLE] = {900 + WAYPOINT_RADIUS, map_size - 900 - WAYPOINT_RADIUS};
    m_last_wp[model::LANE_BOTTOM] = {800 + WAYPOINT_RADIUS, map_size - 200};
}

void PathFinder::update_info(const InfoPack &info, const fields::FieldMap &danger_map) {
    m_i = &info;
    m_damage_map = &danger_map;

    //Calculate battle front for each lane
    std::array<double, model::_LANE_COUNT_> bfront;
    bfront.fill(1);

    //If line pushed, go to enemy base
    m_last_wp[model::LANE_TOP] = {3400, 200};
    m_last_wp[model::LANE_BOTTOM] = {3800, 600};
    m_last_wp[model::LANE_MIDDLE] = {3400, 600};

    for (const auto &i : m_i->ew->get_hostile_creeps()) {
        if (i->getFaction() == model::FACTION_NEUTRAL) {
            continue; //Neutral cannot push lane
        }
        const geom::Point2D pt{i->getX(), i->getY()};
        auto lane = get_lane_by_coord(pt);
        double front = 1;
        geom::Point2D projection;
        switch (lane) {
            case model::LANE_TOP:
                projection = top_lane_projection(pt, 0, &front);
                break;
            case model::LANE_MIDDLE:
                projection = middle_lane_projection(pt, 0, &front);
                break;
            case model::LANE_BOTTOM:
                projection = bottom_lane_projection(pt, 0, &front);
                break;
            default: break;
        }
        if (lane != model::_LANE_UNKNOWN_ && front < bfront[lane]) {
            bfront[lane] = front;
            m_last_wp[lane] = projection;
        }
    }
    for (const auto &i : m_i->ew->get_hostile_towers()) {
        if (i.getMaxLife() == m_i->g->getFactionBaseLife()) {
            //Exclude faction base
            continue;
        }
        const geom::Point2D pt = i.getPoint();
        auto lane = get_lane_by_coord(pt);
        double front = 1;
        int shift = static_cast<int>(-i.getAttackRange() + WAYPOINT_RADIUS);
        geom::Point2D projection;
        switch (lane) {
            case model::LANE_TOP:
                projection = top_lane_projection(pt, shift, &front);
                break;
            case model::LANE_MIDDLE:
                projection = middle_lane_projection(pt, shift, &front);
                break;
            case model::LANE_BOTTOM:
                projection = bottom_lane_projection(pt, shift, &front);
                break;
            default: break;
        }
        if (lane != model::_LANE_UNKNOWN_ && front < bfront[lane]) {
            bfront[lane] = front;
            m_last_wp[lane] = projection;
        }
    }
    m_lane_push_status = bfront;

    if (m_i->w->getTickIndex() % 50 == 0) {
        LOG("Lane push status: top = %3.3lf; middle = %3.2lf; bottom = %3.2lf\n",
            m_lane_push_status[model::LANE_TOP],
            m_lane_push_status[model::LANE_MIDDLE],
            m_lane_push_status[model::LANE_BOTTOM]);
    }
}

Point2D PathFinder::get_next_waypoint() {
    return m_last_wp[m_current_lane];
}

Point2D PathFinder::get_previous_waypoint() {
    const Point2D me{m_i->s->getX(), m_i->s->getY()};

    auto lane = get_lane_by_coord(me);
    if (lane == model::_LANE_UNKNOWN_) {
        lane = m_current_lane;
    }

    switch (lane) {
        case model::LANE_TOP:return top_lane_projection(me, -WAYPOINT_RADIUS - WAYPOINT_SHIFT);
        case model::LANE_MIDDLE:return middle_lane_projection(me, -WAYPOINT_RADIUS - WAYPOINT_SHIFT);
        case model::LANE_BOTTOM:return bottom_lane_projection(me, -WAYPOINT_RADIUS - WAYPOINT_SHIFT);
        default: break;
    }

    //We are in forest or between lanes
    return {0, 0};
}

model::LaneType PathFinder::get_lane_by_coord(Point2D pt) const {

    const double ym_up = -pt.x + 3200;
    const double ym_down = -pt.x + 4800;
    const double ym_st = pt.x + 2400;
    if (pt.y >= ym_up && pt.y <= ym_down && pt.y <= ym_st) {
        return model::LANE_MIDDLE;
    }


    if ((pt.x <= 600 && pt.y <= 3200)
        || (pt.x <= 800 && pt.y <= 800)
        || (pt.x <= 3600 && pt.y <= 600)) {
        return model::LANE_TOP;
    }

    if ((pt.x >= 800 && pt.y >= 3400)
        || (pt.x >= 3200 && pt.y >= 3200)
        || (pt.x >= 3400 && pt.y <= 3200)) {
        return model::LANE_BOTTOM;
    }


    return model::_LANE_UNKNOWN_;
}

geom::Point2D PathFinder::top_lane_projection(const geom::Point2D &me, int shift, double *battle_front) {
    static constexpr double x1 = 200;
    static constexpr double x2 = 800;
    static constexpr double x3 = 3600;
    static constexpr double y0 = 3200;
    static constexpr double y1 = 800;
    static constexpr double y2 = 200;
    static const double sa = sin(pi / 4);
    static const double ca = sa;
    static constexpr double l1 = y0 - y1;
    static const geom::Vec2D s2 = geom::normalize({x2 - x1, y2 - y1});
    static const double l2 = geom::Vec2D(x2 - x1, y2 - y1).len();
    static const double l3 = x3 - x2;

    double len = 0;
    if (me.y >= y1) {
        len = y0 - me.y;
    } else if (me.x <= x2) {
        const geom::Vec2D cur = {me.x - x1, me.y - y1};
        const double c = cur * s2;
        len = l1 + c;
    } else {
        len = l1 + l2 + (me.x - x2);
    }

    len += shift;

    if (battle_front) {
        *battle_front = len / (l1 + l2 + l3);
    }

    if (len <= l1) {
        return {x1, y0 - len};
    } else if (len <= l1 + l2) {
        const double dl = len - l1;
        return {x1 + dl * sa, y1 - dl * ca};
    } else {
        const double dl = len - l2 - l1;
        return {x2 + dl, y2};
    }
}

geom::Point2D PathFinder::middle_lane_projection(const geom::Point2D &me, int shift, double *battle_front) {
    static const geom::Vec2D mid_line = geom::normalize({4000, -4000});
    const geom::Vec2D cur = {me.x, me.y - 4000};
    const double c = cur * mid_line;

    if (battle_front) {
        static const double norm_len = 2800 * sqrt(2);
        static const double start_len = 500 * sqrt(2);
        const double norm_c = std::min(c - start_len, norm_len);
        *battle_front = norm_c / norm_len;
    }

    geom::Vec2D res_pt = mid_line * (c + shift);
    res_pt.y += 4000;
    return res_pt;
}

geom::Point2D PathFinder::bottom_lane_projection(const geom::Point2D &me, int shift, double *battle_front) {
    auto ret = top_lane_projection({4000 - me.y, 4000 - me.x}, shift, battle_front);
    std::swap(ret.x, ret.y);
    ret.x = 4000 - ret.x;
    ret.y = 4000 - ret.y;
    return ret;
}

void PathFinder::move_along(const geom::Vec2D &dir, model::Move &move, bool hold_face) {
    const double turn_angle = m_i->s->getAngleTo(m_i->s->getX() + dir.x, m_i->s->getY() + dir.y);
    const double my_angle = m_i->s->getAngle() + pi / 2;

    const double turn_cos = cos(-my_angle);
    const double turn_sin = sin(-my_angle);

    double x1 = dir.x * turn_cos - dir.y * turn_sin;
    double y1 = dir.y * turn_cos + dir.x * turn_sin;

    move.setStrafeSpeed(x1);
    move.setSpeed(-y1);

    if (!hold_face) {
        move.setTurn(turn_angle);
    }
}

std::list<geom::Point2D> PathFinder::find_way(const geom::Point2D &to, double radius) {

    if (m_i->ew->line_of_sight(m_i->s->getX(), m_i->s->getY(), to.x, to.y)) {
        //Go straight
        return {to};
    }


    const double ex_r = m_i->s->getRadius();

    //Clear map
    for (int i = 0; i < m_map.size(); ++i) {
        for (int j = 0; j < m_map[i].size(); ++j) {
            m_map[i][j] = {geom::CellCoord(i, j), nullptr, 1e9};
        };
    }
    //Clear closed cells
    static std::array<std::array<bool, CELL_COUNT>, CELL_COUNT> closed;
    for (auto &column : closed) {
        column.fill(false);
    }

    //Heuristic function for A-star
    //const auto &dmap = m_damage_map;
    const auto astar_h = [](const Point2D &cur, const Point2D &target) {
        static const double D2 = sqrt(2);
        double dx = std::abs(cur.x - target.x);
        double dy = std::abs(cur.y - target.y);
        return (dx + dy) + (D2 - 2) * std::min(dx, dy);
    };

    struct CCWithCost {
        CCWithCost(const geom::CellCoord &pt_, double cost_)
            : pt(pt_),
              cost(cost_) {
        }

        geom::CellCoord pt;
        double cost;

        bool operator>(const CCWithCost &other) const {
            return cost > other.cost;
        }
    };

    std::priority_queue<CCWithCost, std::vector<CCWithCost>, std::greater<CCWithCost>> open;
    geom::CellCoord initial;
    initial = world_to_cell({m_i->s->getX(), m_i->s->getY()});
    m_map[initial.x][initial.y].cost = 0;

    open.emplace(initial, m_map[initial.x][initial.y].cost + astar_h(cell_to_world(initial), to));

    int visited_cells = 0;
    const auto sqrradius = radius * radius;
    const Cell *found = nullptr;
    geom::CellCoord nearest = initial;
    int min_manh = 100000000;
    const geom::CellCoord cell_target = world_to_cell(to);
    const double my_radius = m_i->s->getRadius();
    bool first = true;
    while (!open.empty()) {
        geom::CellCoord next = open.top().pt;
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
        if (closed[next.x][next.y] || !is_correct_cell(next, initial)) {
            continue;
        }

        const MyLivingUnit *obst = nullptr;
        if (!first && !m_i->ew->check_no_collision(wnext, my_radius, &obst)) {
            if (!obst || (obst && obst->getType() != MyLivingUnit::TREE)) {
                continue;
            }
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
        static const std::initializer_list<geom::CellCoord> shifts = {
            {0, -1},
            {1, -1},
            {1, 0},
            {1, 1},
            {0, 1},
            {-1, 1},
            {-1, 0},
            {-1, -1}
        };
        geom::CellCoord neigh;
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

    smooth_path({m_i->s->getX(), m_i->s->getY()}, ret);

    //LOG("Way to point (%3.1lf, %3.1lf) %s. Vertexes visited %d\n",
    //    to.x,
    //    to.y,
    //    found ? "found" : "not found",
    //    visited_cells);

    return ret;
}

bool PathFinder::is_correct_cell(const geom::CellCoord &tocheck, const geom::CellCoord &initial) {
    const bool inbound = tocheck.x >= 0 && tocheck.x < CELL_COUNT && tocheck.y >= 0 && tocheck.y < CELL_COUNT;
    return inbound;
}

geom::CellCoord PathFinder::world_to_cell(const Point2D &wpt) {
    return geom::CellCoord(static_cast<int>(round(wpt.x / GRID_SIZE)), static_cast<int>(round(wpt.y / GRID_SIZE)));
}

geom::Point2D PathFinder::cell_to_world(const geom::CellCoord &cpt) {
    return Point2D(cpt.x * GRID_SIZE, cpt.y * GRID_SIZE);
}

bool PathFinder::update_cost(const geom::CellCoord &pt_from, const geom::CellCoord &pt_to) {
    const double D2 = sqrt(2) * GRID_SIZE;
    const double D = GRID_SIZE;
    int dx = std::abs(pt_from.x - pt_to.x);
    int dy = std::abs(pt_from.y - pt_to.y);
    double mv_cost = D * (dx + dy) + (D2 - 2 * D) * std::min(dx, dy);

    mv_cost += mv_cost * m_damage_map->get_value(cell_to_world(pt_from)) * BehaviourConfig::pathfinder_damage_mult;

    //Check tree in destination point
    const MyLivingUnit *obst = nullptr;
    auto wpto = cell_to_world(pt_to);
    if (!m_i->ew->check_no_collision(wpto, m_i->s->getRadius(), &obst)) {
        //Maybe it is tree
        if (obst && obst->getType() == MyLivingUnit::TREE) {
            int time_to_cut = (obst->getLife() / 12) * 60;
            if (time_to_cut == 0) {
                time_to_cut = 7;
            }
            const double my_speed = m_i->g->getWizardForwardSpeed() * m_i->ew->get_wizard_movement_factor(*m_i->s);
            mv_cost += (time_to_cut * my_speed);
        }
    }


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
    return m_lane_push_status[m_current_lane] > 0.4 && m_lane_push_status[m_current_lane] < 0.82;
}

void PathFinder::smooth_path(const geom::Point2D &me, std::list<geom::Point2D> &path) const {
    auto almost_last = path.cend();
    --almost_last;
    for (auto it = path.cbegin(); it != almost_last;) {
        if (m_i->ew->line_of_sight(me.x, me.y, it->x, it->y)) {
            it = path.erase(it);
        } else {
            break;
        }
    }
}
