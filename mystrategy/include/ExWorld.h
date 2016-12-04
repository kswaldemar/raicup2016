//
// Created by valdemar on 29.11.16.
//

#pragma once

#include "UnitDesc.h"
#include "Vec2D.h"
#include "model/World.h"
#include "model/Game.h"
#include "Canvas.h"
#include <array>

/**
 * World class wrapper with extended features
 */
class ExWorld {
public:

    ExWorld(const model::World &world, const model::Game &game);

    //Should be called at each tick start
    void update_world(const model::Wizard &self, const model::World &world, const model::Game &game);

    const std::vector<const model::Minion*> &get_hostile_creeps() const;

    const std::vector<const model::Wizard*> &get_hostile_wizards() const;

    const std::vector<SimpleCircle> &get_obstacles() const;

    const std::vector<TowerDesc> &get_hostile_towers() const;

    bool check_no_collision(geom::Point2D obj, double radius, SimpleCircle *out_obstacle = nullptr) const;

    bool check_in_team_vision(const geom::Point2D &pt) const;

    double get_wizard_movement_factor(const model::Wizard &who) const;

    bool line_of_sight(double x1, double y1, double x2, double y2) const;

    void show_me_canvas() const;

private:

    void update_shadow_towers(const model::World &world);

    void update_canvas(const geom::Point2D &origin);

    const double m_map_size;
    std::vector<SimpleCircle> m_obstacles;
    //Field of view description - point, view range
    std::vector<std::pair<const geom::Point2D, double>> m_fov;
    std::vector<const model::Minion*> m_en_creeps;
    std::vector<const model::Wizard*> m_en_wizards;

    std::vector<TowerDesc> m_shadow_towers;
    std::array<double, 10> m_wizard_speed_factor;

    double m_self_radius;
    geom::Point2D m_canvas_origin;
    Canvas m_im_draw;
};
