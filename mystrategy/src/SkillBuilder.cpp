//
// Created by valdemar on 27.11.16.
//

#include "SkillBuilder.h"
#include "Logger.h"

#include <array>
#include <cassert>

using namespace model;

SkillBuilder::SkillBuilder() {

}

void SkillBuilder::update_info(const InfoPack &info) {
    m_i = &info;

    const auto &skills = m_i->s->getSkills();

    m_build_idx = static_cast<int>(m_i->s->getId() % 2);
}

void SkillBuilder::try_level_up(model::Move &move) const {

    static const std::vector<Skill> build_mm {
        RANGE, RANGE, RANGE, RANGE, RANGE,
        SHIELD, SHIELD, SHIELD, SHIELD, SHIELD,
        FIRE, FIRE, FIRE, FIRE, FIRE,
        HASTE, HASTE, HASTE, HASTE,
        WATER, WATER, WATER, WATER,
    };

    static const std::vector<Skill> build_fireball {
        FIRE, FIRE, FIRE, FIRE, FIRE,
        RANGE, RANGE, RANGE, RANGE, RANGE,
        SHIELD, SHIELD, SHIELD, SHIELD, SHIELD,
        HASTE, HASTE, HASTE, HASTE,
        WATER, WATER, WATER, WATER,
    };


    static const std::vector<Skill> build_test {
        FIRE, FIRE, FIRE, FIRE, FIRE,
        SHIELD, SHIELD, SHIELD, SHIELD, SHIELD,
        HASTE, HASTE, HASTE, HASTE, HASTE,
        RANGE, RANGE, RANGE, RANGE, RANGE,
        WATER, WATER, WATER, WATER,
    };

    //move.setSkillToLearn(learn_next_skill(m_build_idx == 0 ? build_mm : build_fireball));
    move.setSkillToLearn(learn_next_skill(build_test));


}

const model::SkillType SkillBuilder::learn_next_skill(const std::vector<SkillBuilder::Skill> &build) const {
    const auto &skills = m_i->s->getSkills();
    const auto next_idx = skills.size();
    if (build.size() <= next_idx) {
        //Build is over, return random skill
        return SKILL_ADVANCED_MAGIC_MISSILE;
    }
    int ultimate = 0;
    switch (build[next_idx]) {
        case FIRE:
            ultimate = SKILL_FIREBALL;
            break;
        case WATER:
            ultimate = SKILL_FROST_BOLT;
            break;
        case RANGE:
            ultimate = SKILL_ADVANCED_MAGIC_MISSILE;
            break;
        case SHIELD:
            ultimate = SKILL_SHIELD;
            break;
        case HASTE:
            ultimate = SKILL_HASTE;
            break;
    }

    int last_skill = -1;
    for (const auto &i : skills) {
        if (i < ultimate && i > last_skill) {
            last_skill = i;
        }
    }

    if (ultimate - last_skill > 5) {
        last_skill = ultimate - 5;
    }

    return static_cast<SkillType>(last_skill + 1);

}
