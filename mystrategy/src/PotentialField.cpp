//
// Created by valdemar on 09.11.16.
//

#include "PotentialField.h"
#include "Point2D.h"
#include "Logger.h"
#include "PotentialConfig.h"

#include "model/Wizard.h"

#include <cassert>
#include <queue>

const std::initializer_list<Point2D> PotentialField::m_around_shift = {
    {-1, 0}, //Left
    {1, 0}, //Right
    {0, -1}, // Up
    {0, 1}, // Down
    {-1, -1}, //Upper left
    {1, -1}, //Upper right
    {-1, 1}, // Lower left
    {1, 1}, // Lower right
};

PotentialField::PotentialField(uint32_t width, uint32_t height, uint32_t scale) {
    assert(height % scale == 0);
    assert(width % scale == 0);
    width /= scale;
    height /= scale;
    m_f.resize(width);
    for (auto &i : m_f) {
        i.resize(height);
    }
    m_scale = scale;
    m_width = width;
    m_height = height;

    m_visited.resize(width);
    for (auto &i : m_visited) {
        i.resize(height);
    }
}

void PotentialField::relax() {
    clean();
}

void PotentialField::add_unit(const model::Wizard &wizard) {

}

void PotentialField::add_unit(const model::Minion &creep) {

}

void PotentialField::add_unit(const model::Building &build) {

}

void PotentialField::add_waypoint(const Point2D &wp) {
    clean_visited();
    Point2D cur{0, 0};
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_height; ++y) {
            cur.x = x;
            cur.y = y;
            cur *= m_scale;
            //const int len = std::min<int>(std::abs(cur.x - wp.x), std::abs(cur.y - wp.y));
            const int len = static_cast<int>(cur.get_distance_to(wp));
            if (len > 0) {
                m_f[x][y].val = potential::calc_waypoint_cost(len);
            }
        }
    }
}

void PotentialField::update_self(const model::Wizard &self) {
    m_self = &self;
}

Point2D PotentialField::potential_move(const Point2D &unit) {
    Point2D vec{0, 0};
    FieldVal *best = get_cell_value(unit);
    assert(best);
    for (const auto &s : m_around_shift) {
        FieldVal *candidate = get_cell_value(unit + s);
        if (candidate && *candidate > *best) {
            best = candidate;
            vec = s;
        }
    }
    Point2D scaled_unit{static_cast<int>(unit.x / m_scale), static_cast<int>(unit.y / m_scale)};
    scaled_unit += vec;
    scaled_unit *= m_scale;
    return scaled_unit;

}

PotentialField::FieldVal *PotentialField::get_cell_value(const Point2D &pt) {
    int x = pt.x / m_scale;
    int y = pt.y / m_scale;
    if (x >= 0 && x < m_f[0].size() && y >=0 && y < m_f.size()) {
        return &m_f[x][y];
    } else {
        LOG_ERROR("Incorrect get_cell_value call with (%d; %d)", x, y);
        return nullptr;
    }
}

void PotentialField::clean() {
    for (auto &column : m_f) {
        for (auto &fv : column) {
            fv.val = 0;
            fv.any_angle = true;
        }
    }
}

void PotentialField::clean_visited() {
    for (int x = 0; x < m_width; ++x) {
        for (int y = 0; y < m_height; ++y) {
            m_visited[x][y] = false;
        }
    }
}
