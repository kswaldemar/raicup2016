//
// Created by valdemar on 29.11.16.
//

#pragma once

#include "UnitDesc.h"
#include "Vec2D.h"
#include "Canvas.h"
#include "MyLivingUnit.h"
#include "MyBonus.h"
#include "MyBuilding.h"
#include "model/Game.h"
#include "model/World.h"
#include <array>
#include <list>
#include <set>

/**
 * World class wrapper with extended features
 */
class ExWorld {
    static constexpr int TREE_UPDATE_FREQ = 1;
public:

    ExWorld(const model::World &world, const model::Game &game);

    //Should be called at each tick start
    void update_world(const model::Wizard &self, const model::World &world, const model::Game &game);

    const std::vector<const model::Minion*> &get_hostile_creeps() const;

    const std::vector<const model::Wizard*> &get_hostile_wizards() const;

    const std::vector<MyLivingUnit> &get_obstacles() const;

    const std::vector<MyBuilding> &get_hostile_towers() const;

    const std::vector<MyBonus> &get_bonuses() const;

    bool check_no_collision(geom::Point2D obj, double radius, const MyLivingUnit **out_obstacle = nullptr) const;

    bool point_in_team_vision(const geom::Point2D &pt) const;

    bool point_in_team_vision(double x, double y) const;

    double get_wizard_movement_factor(const model::Wizard &who) const;

    double get_wizard_turn_factor(const model::Wizard &who) const;

    double get_wizard_magical_damage_absorb_factor() const;

    double get_wizard_damage_absorb_factor() const;

    bool line_of_sight(double x1, double y1, double x2, double y2) const;

    void show_me_canvas() const;

private:

    void update_shadow_towers(const model::World &world);

    void update_canvas(const geom::Point2D &origin);

    void update_bonuses(const model::World &world);

    void update_obstacles_and_fov(const model::World &world);

    void update_wizards_speed_factors(const model::World &world, const model::Game &game);

    const double m_map_size;
    std::list<MyLivingUnit> m_trees;
    std::vector<MyLivingUnit> m_obstacles;
    std::vector<MyBonus> m_bonuses;
    std::set<int64_t> m_known_trees;
    //Field of view description - point, view range
    std::vector<std::pair<const geom::Point2D, double>> m_fov;
    std::vector<const model::Minion*> m_en_creeps;
    std::vector<const model::Wizard*> m_en_wizards;
    std::vector<MyBuilding> m_shadow_towers;
    std::array<double, 11> m_wizard_speed_factor;
    geom::Point2D m_canvas_origin;
    Canvas m_im_draw;
};
