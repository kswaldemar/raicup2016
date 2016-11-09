#pragma once

#ifndef _MY_STRATEGY_H_
#define _MY_STRATEGY_H_

#include "Strategy.h"
#include "InfoPack.h"
#include "PotentialField.h"

#include <memory>

class MyStrategy : public Strategy {

public:
    MyStrategy();

    void move(const model::Wizard& self,
              const model::World& world,
              const model::Game& game,
              model::Move& move) override;

private:

    void initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game);

    InfoPack m_i;
    std::unique_ptr<PotentialField> m_field = nullptr;
};

#endif
