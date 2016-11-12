#pragma once

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include "InfoPack.h"
#include "Vec2D.h"
#include "PathFinder.h"
#include "Eviscerator.h"

#include <memory>

class MyStrategy : public Strategy {

public:
    MyStrategy();

    void move(const model::Wizard& self,
              const model::World& world,
              const model::Game& game,
              model::Move& move) override;

    void initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game);

    geom::Vec2D repelling_obs_avoidance_vector();

private:

    InfoPack m_i;
    std::unique_ptr<PathFinder> m_pf;
    std::unique_ptr<Eviscerator> m_ev;
    geom::Vec2D repelling_damage_avoidance_vector();
};

#endif
