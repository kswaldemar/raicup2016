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
    static constexpr int WAYPOINT_RADIUS = 100;
    static constexpr int WAYPOINT_SHIFT = 200;
    //Pathfind grid step
    static constexpr int GRID_SIZE = 5;
    //Hardcoded
    static constexpr int WORLD_SIZE = 4000;
    static constexpr int CELL_COUNT = (WORLD_SIZE + GRID_SIZE - 1)/ GRID_SIZE;
    //Max cell number to visit before manual halt (if more think there is no way)
    static constexpr int ASTAR_MAX_VISIT = 5000;

    struct Cell {
        //To build path, after searching
        enum DIR {
            U = 1,
            R = 2,
            D = 4,
            L = 8,
        };
        geom::CellCoord me;
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

    static geom::CellCoord world_to_cell(const geom::Point2D &wpt);

    static geom::Point2D cell_to_world(const geom::CellCoord &cpt);

    bool bonuses_is_under_control() const;

    geom::Point2D top_lane_projection(const geom::Point2D &me, int shift, double *battle_front = nullptr);

    geom::Point2D middle_lane_projection(const geom::Point2D &me, int shift, double *battle_front = nullptr);

    geom::Point2D bottom_lane_projection(const geom::Point2D &me, int shift, double *battle_front = nullptr);

    model::LaneType get_lane_by_coord(geom::Point2D pt) const;

private:

    bool is_correct_cell(const geom::CellCoord &tocheck, const geom::CellCoord &initial);

    bool update_cost(const geom::CellCoord &pt_from, const geom::CellCoord &pt_to);

    const InfoPack *m_i;
    const fields::FieldMap *m_danger_map;
    std::array<std::array<Cell, CELL_COUNT>, CELL_COUNT> m_map;
    std::array<geom::Point2D, model::_LANE_COUNT_> m_last_wp;
    std::array<double, model::_LANE_COUNT_> m_lane_push_status;
    model::LaneType m_current_lane = model::_LANE_UNKNOWN_;

};
