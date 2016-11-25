//
// Created by valdemar on 11.11.16.
//

#pragma once

#include "model/Move.h"
#include "InfoPack.h"
#include "Vec2D.h"
#include "FieldMap.h"
#include "Config.h"
#include <array>

/**
 * Path find and moving related class
 */
class PathFinder {
public:
    static constexpr int WAYPOINT_RADIUS = 200;
    //Pathfind grid step
    static constexpr int GRID_SIZE = 5;
    //Hardcoded
    static constexpr int WORLD_SIZE = 4000;
    static constexpr int CELL_COUNT = (WORLD_SIZE + GRID_SIZE - 1)/ GRID_SIZE;
    //Max cell number to visit before manual halt (if more think there is no way)
    static constexpr int ASTAR_MAX_VISIT = 7000;


    struct CellCoord {
        int x, y;

        CellCoord operator+(const CellCoord &other) const {
            return {x + other.x, y + other.y};
        }
    };
    struct Cell {
        //To build path, after searching
        enum DIR {
            U = 1,
            R = 2,
            D = 4,
            L = 8,
        };
        CellCoord me;
        const Cell *parent;
        double cost;
    };

    PathFinder(const InfoPack &info);


    void update_info(const InfoPack &info, const fields::FieldMap &danger_map);

    geom::Point2D get_next_waypoint();

    geom::Point2D get_previous_waypoint();

    void move_along(const geom::Vec2D &dir, model::Move &move, bool hold_face);

    /**
     * Find best way to destination with radius
     */
    std::list<geom::Point2D> find_way(const geom::Point2D &to, double radius);

    bool check_no_collision(const geom::Point2D &pt, double radius) const;

    static CellCoord world_to_cell(const geom::Point2D &wpt);

    static geom::Point2D cell_to_world(const CellCoord &cpt);

    bool bonuses_is_under_control() const;

private:

    bool is_correct_cell(const CellCoord &tocheck, const CellCoord &initial);

    const InfoPack *m_i;
    const std::vector<geom::Point2D> *m_waypoints = nullptr;
    std::vector<geom::Point2D>::const_iterator m_next_wp, m_last_wp;
    std::array<std::array<Cell, CELL_COUNT>, CELL_COUNT> m_map;
    const fields::FieldMap *m_danger_map;
    std::vector<const model::CircularUnit *> m_obstacles;
    bool update_cost(const CellCoord &pt_from, const CellCoord &pt_to);
};
