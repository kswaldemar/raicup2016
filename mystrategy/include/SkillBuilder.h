//
// Created by valdemar on 27.11.16.
//

#pragma once

#include "InfoPack.h"
#include "model/Move.h"

class SkillBuilder {
public:

    SkillBuilder();

    void update_info(const InfoPack &info);

    void try_level_up(model::Move &move) const;

private:

    enum Skill {
        FIRE, ///< Fire magic, staff damage and ultimate fireball
        WATER, ///< Water magic, damage bonus and ultimate frostbolt
        RANGE, ///< Increase cast range, ultimate Magic Missile without cooldown
        SHIELD, ///< Decrease incoming damage, ultimate Shield spell
        HASTE, ///< Increase movement speed, ultimate Haste spell
    };

    const InfoPack *m_i;

    const model::SkillType learn_next_skill(const std::vector<SkillBuilder::Skill> &build) const;
    const model::SkillType next_fire_skill(const std::vector<model::SkillType> &vector) const;
    const model::SkillType next_water_skill(const std::vector<model::SkillType> &vector) const;
    const model::SkillType next_range_skill(const std::vector<model::SkillType> &vector) const;
    const model::SkillType next_shield_skill(const std::vector<model::SkillType> &vector) const;
    const model::SkillType next_haste_skill(const std::vector<model::SkillType> &vector) const;
};



