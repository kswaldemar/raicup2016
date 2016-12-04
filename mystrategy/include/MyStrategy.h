#pragma once

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include "InfoPack.h"
#include "Vec2D.h"
#include "PathFinder.h"
#include "Eviscerator.h"
#include "UnitDesc.h"
#include "FieldMap.h"
#include "MovementHandler.h"
#include "SkillBuilder.h"

#include <memory>
#include <list>

class MyStrategy : public Strategy {
public:

    enum Behaviour {
        BH_ATTACK,
        BH_SCOUT,
        BH_MINIMIZE_DANGER,
        BH_EARN_BONUS,
        BH_COUNT
    };

    MyStrategy();

    void move(const model::Wizard& self,
              const model::World& world,
              const model::Game& game,
              model::Move& move) override;

    void initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game);

    geom::Vec2D potential_vector(const geom::Point2D &pt, const std::vector<const fields::FieldMap*> &fmap) const;

    bool initialize_strategy(const model::Wizard &self, const model::World &world, const model::Game &game);

    static void visualise_field_maps(const std::vector<const fields::FieldMap *> &fmaps, const geom::Point2D &center);

    /*
     * Update danger map, according to current world info
     */
    void update_danger_map();

    void update_bonuses();

    void each_tick_update(const model::Wizard &self, const model::World &world, const model::Game &game);

    void smooth_path(const geom::Point2D &me, std::list<geom::Point2D> &path);

private:
    InfoPack m_i;
    std::unique_ptr<PathFinder> m_pf;
    std::unique_ptr<Eviscerator> m_ev;
    std::unique_ptr<SkillBuilder> m_sb;
    //Danger map
    fields::FieldMap m_danger_map = fields::FieldMap(fields::FieldMap::ADD);
    std::array<MovementHandler, BH_COUNT> m_bhs;
    BonusDesc m_bns_top;
    BonusDesc m_bns_bottom;
};

#endif
