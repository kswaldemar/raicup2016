//
// Created by valdemar on 27.11.16.
//

#pragma once

#include "InfoPack.h"
#include "model/Move.h"

class SkillBuilder {
    enum Skill {
        FIRE, ///< Fire magic, staff damage bonus and ultimate fireball
        WATER, ///< Water magic, spell damage bonus and ultimate frostbolt
        RANGE, ///< Increase cast range, ultimate Magic Missile without cooldown
        SHIELD, ///< Decrease incoming damage, ultimate Shield spell
        HASTE, ///< Increase movement speed, ultimate Haste spell
    };
public:

    SkillBuilder();

    void update_info(const InfoPack &info);

    void try_level_up(model::Move &move) const;

    const model::SkillType learn_next_skill(const std::vector<SkillBuilder::Skill> &build) const;

private:
    const InfoPack *m_i;
    int m_build_idx;
};



