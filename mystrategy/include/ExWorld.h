//
// Created by valdemar on 29.11.16.
//

#pragma once

#include "UnitDesc.h"
#include "Vec2D.h"
#include "model/World.h"
#include "model/Game.h"
#include <array>

/**
 * World class wrapper with extended features
 */
class ExWorld {
public:

    ExWorld(const model::World &world, const model::Game &game);

    //Should be called at each tick start
    void update_world(const model::World &world, const model::Game &game);

    const std::vector<const model::Minion*> &get_hostile_creeps() const;

    const std::vector<const model::Wizard*> &get_hostile_wizards() const;

    const std::vector<const model::CircularUnit*> &get_obstacles() const;

    const std::vector<TowerDesc> &get_hostile_towers() const;

    bool check_no_collision(const geom::Point2D &obj, const double radius) const;

    bool check_in_team_vision(const geom::Point2D &pt) const;

    double get_wizard_movement_factor(const model::Wizard &who) const;

private:

    void update_shadow_towers(const model::World &world);

    const double m_map_size;
    std::vector<const model::CircularUnit*> m_obstacles;
    //Field of view description - point, view range
    std::vector<std::pair<const geom::Point2D, double>> m_fov;
    std::vector<const model::Minion*> m_en_creeps;
    std::vector<const model::Wizard*> m_en_wizards;

    std::vector<TowerDesc> m_shadow_towers;
    std::array<double, 10> m_wizard_speed_factor;
};
