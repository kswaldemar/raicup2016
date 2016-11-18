#pragma once

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include "InfoPack.h"
#include "Vec2D.h"
#include "PathFinder.h"
#include "Eviscerator.h"
#include "UnitDesc.h"

#include <memory>
#include <list>

class MyStrategy : public Strategy {
    using DangerMap = std::vector<std::unique_ptr<fields::IVectorField>>;
public:
    MyStrategy();

    void move(const model::Wizard& self,
              const model::World& world,
              const model::Game& game,
              model::Move& move) override;

    void initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game);

    geom::Vec2D repelling_obs_avoidance_vector();

    geom::Vec2D repelling_damage_avoidance_vector();

    bool initialize_strategy(const model::Wizard &self, const model::World &world, const model::Game &game);

    /*
     * Analyse information about vision and update any tower-related information if possible
     */
    static void update_shadow_towers(std::list<TowerDesc> &towers,
                                     const model::World &world,
                                     const model::Faction my_faction);

    static void visualise_danger_map(const DangerMap& danger, const geom::Point2D &center);

    /*
     * Update danger map, according to current world info
     */
    void update_danger_map();

private:
    InfoPack m_i;
    std::unique_ptr<PathFinder> m_pf;
    std::unique_ptr<Eviscerator> m_ev;
    //Enemy towers, to not forget in fog of war
    std::list<TowerDesc> m_enemy_towers;
    //Danger map
    DangerMap m_danger_map;
};

#endif
