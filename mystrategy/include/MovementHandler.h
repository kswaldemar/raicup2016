//
// Created by valdemar on 20.11.16.
//

#pragma once

#include "Vec2D.h"
#include "model/CircularUnit.h"
#include <list>

class MovementHandler {
public:

    int PATH_SPOIL_TIME = 20;
    static constexpr int MAX_TARGET_MOVEMENT = 10;

    bool is_path_spoiled();

    void update_target(const geom::Point2D &center);

    void update_target(const model::CircularUnit &unit);

    void update_clock(int tick);

    void load_path(std::list<geom::Point2D> &&path, const geom::Point2D &center);

    void load_path(std::list<geom::Point2D> &&path, const model::CircularUnit &unit);

    geom::Vec2D get_next_direction(const geom::Point2D &self);

    geom::Vec2D get_next_direction(const model::CircularUnit &self);
private:

    struct TgDesc {
        geom::Point2D center;
    };

    TgDesc m_initial;
    TgDesc m_actual;
    int m_update_tick = -10000;
    int m_current_tick;
    std::list<geom::Point2D> m_path;
    std::list<geom::Point2D>::const_iterator m_it;
};
