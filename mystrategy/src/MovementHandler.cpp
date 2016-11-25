//
// Created by valdemar on 20.11.16.
//

#include "VisualDebug.h"
#include "MovementHandler.h"

bool MovementHandler::is_path_spoiled() {
    if (m_current_tick - m_update_tick >= PATH_SPOIL_TIME || m_it == m_path.cend()) {
        return true;
    }
    geom::Vec2D dist = m_actual.center - m_initial.center;
    return dist.sqr() >= (MAX_TARGET_MOVEMENT * MAX_TARGET_MOVEMENT);
}

void MovementHandler::update_target(const geom::Point2D &center, double radius) {
    m_actual.center = center;
    m_actual.radius = radius;
}

void MovementHandler::update_target(const model::CircularUnit &unit) {
    m_actual.center = {unit.getX(), unit.getY()};
    m_actual.radius = unit.getRadius();
}

void MovementHandler::update_clock(int tick) {
    m_current_tick = tick;
}

void MovementHandler::load_path(std::list<geom::Point2D> &&path, const geom::Point2D &center, double radius) {
    m_path = std::move(path);
    m_it = m_path.cbegin();
    m_update_tick = m_current_tick;
    m_initial.center = center;
    m_initial.radius = radius;
}

void MovementHandler::load_path(std::list<geom::Point2D> &&path, const model::CircularUnit &unit) {
    m_path = std::move(path);
    m_it = m_path.cbegin();
    m_update_tick = m_current_tick;
    m_initial.center = {unit.getX(), unit.getY()};
    m_initial.radius = unit.getRadius();
}

geom::Vec2D MovementHandler::get_next_direction(const geom::Point2D &self) {
#ifdef RUNNING_LOCAL
    auto st_p = m_it;
    auto prev = self;
    while (st_p != m_path.cend()) {
        VISUAL(line(prev.x, prev.y, st_p->x, st_p->y, 0xE813D1));
        prev = *st_p;
        ++st_p;
    }
#endif
    if (m_it != m_path.cend()) {
        auto next_pt = m_it;
        ++next_pt;

        geom::Vec2D dist1 = self - *m_it;
        geom::Vec2D dist2 = self - *next_pt;
        if (dist1.len() <= 4 || (next_pt != m_path.cend() && dist2.len() <= (*next_pt - *m_it).len())) {
            ++m_it;
        }

        if (m_it != m_path.cend()) {
            return *m_it - self;
        }
    }

    return {0, 0};
}

geom::Vec2D MovementHandler::get_next_direction(const model::CircularUnit &self) {
    return get_next_direction({self.getX(), self.getY()});
}
