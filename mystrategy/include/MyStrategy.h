#pragma once

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include "InfoPack.h"
#include "Vec2D.h"
#include "PathFinder.h"

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
    geom::Vec2D enemy_attraction_vector();

    void move_along(const geom::Vec2D &dir, model::Move &move, bool hold_face = false);

private:

    InfoPack m_i;
    std::unique_ptr<PathFinder> m_pf;
};

#endif
