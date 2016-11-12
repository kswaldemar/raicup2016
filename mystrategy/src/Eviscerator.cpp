//
// Created by valdemar on 12.11.16.
//

#include "model/ActionType.h"
#include "model/Move.h"
#include "Config.h"
#include "Eviscerator.h"
#include "FieldsDescription.h"

#include <cassert>

Eviscerator::Eviscerator(const InfoPack &info)
    : m_i(&info) {
}

void Eviscerator::update_info_pack(const InfoPack &info) {
    m_i = &info;

    m_en_building = nullptr;
    m_en_minion = nullptr;
    m_en_wz = nullptr;
    m_attract_field = nullptr;
}

//int Eviscerator::get_incoming_damage(const geom::Point2D &me, double me_radius, const model::Minion &enemy) {
//    double attack_range;
//    int cooldown;
//    if (enemy.getType() == model::MINION_FETISH_BLOWDART) {
//        attack_range = m_i->g->getFetishBlowdartAttackRange();
//        cooldown = m_i->g->getFetishBlowdartActionCooldownTicks();
//    } else {
//        attack_range = m_i->g->getOrcWoodcutterAttackRange();
//        cooldown = m_i->g->getOrcWoodcutterActionCooldownTicks();
//    }
//    attack_range += me_radius;
//    double dist = enemy.getDistanceTo(me.x, me.y);
//
//    int attack_time = config::DMG_INCOME_TICKS_SIMULATION - enemy.getRemainingActionCooldownTicks();
//    if (dist > attack_range) {
//        attack_time -= (dist - attack_range) / m_i->g->getMinionSpeed();
//    }
//    int attack_count = attack_time / cooldown;
//    attack_count = std::max(attack_count, 0);
//    int dmg = attack_count * enemy.getDamage();
//    return dmg;
//}
//
//int Eviscerator::get_incoming_damage(const geom::Point2D &me, double me_radius, const model::Building &enemy) {
//    double attack_range = enemy.getAttackRange();
//    int cooldown = enemy.getCooldownTicks();
//    attack_range += me_radius;
//    double dist = enemy.getDistanceTo(me.x, me.y);
//
//    if (dist <= attack_range) {
//        int attack_count = (config::DMG_INCOME_TICKS_SIMULATION - enemy.getRemainingActionCooldownTicks()) / cooldown;
//        attack_count = std::max(attack_count, 0);
//        int dmg = attack_count * enemy.getDamage();
//        return dmg;
//    }
//    return 0;
//}
//
//int Eviscerator::get_incoming_damage(const geom::Point2D &me, double me_radius, const model::Wizard &enemy) {
//    double attack_range = m_i->g->getWizardCastRange();
//    attack_range += me_radius;
//    double dist = enemy.getDistanceTo(me.x, me.y);
//
//    int cooldown;
//
//    //Magic missile
//    const auto &remaining_cooldowns = enemy.getRemainingCooldownTicksByAction();
//    if (dist <= attack_range) {
//        cooldown = m_i->g->getMagicMissileCooldownTicks();
//        int rem_cooldown = std::max(enemy.getRemainingActionCooldownTicks(),
//                                    remaining_cooldowns[model::ACTION_MAGIC_MISSILE]);
//        int attack_count = (config::DMG_INCOME_TICKS_SIMULATION - rem_cooldown) / cooldown;
//        attack_count = std::max(attack_count, 0);
//        int dmg = attack_count * m_i->g->getMagicMissileDirectDamage();
//        return dmg;
//    }
//    return 0;
//}

int Eviscerator::get_myself_death_time(const model::Wizard &me, const model::Minion &enemy) {
    double attack_range;
    int cooldown;
    if (enemy.getType() == model::MINION_FETISH_BLOWDART) {
        attack_range = m_i->g->getFetishBlowdartAttackRange();
        cooldown = m_i->g->getFetishBlowdartActionCooldownTicks();
    } else {
        attack_range = m_i->g->getOrcWoodcutterAttackRange();
        cooldown = m_i->g->getOrcWoodcutterActionCooldownTicks();
    }
    attack_range += me.getRadius();
    double dist = enemy.getDistanceTo(me);

    int killing_time = 0;

    //TODO: Honor minion angle
    if (dist > attack_range) {
        killing_time += (dist - attack_range) / m_i->g->getMinionSpeed();
    }
    killing_time += enemy.getRemainingActionCooldownTicks();
    int atk_number = (me.getLife() + enemy.getDamage() - 1) / enemy.getDamage();
    killing_time += (atk_number - 1) * enemy.getCooldownTicks();

    return killing_time;
}

int Eviscerator::get_myself_death_time(const model::Wizard &me, const model::Building &enemy) {
    double attack_range = enemy.getAttackRange();
    int cooldown = enemy.getCooldownTicks();
    attack_range += me.getRadius();
    double dist = enemy.getDistanceTo(me);

    if (dist <= attack_range) {
        int killing_time = 0;
        killing_time += enemy.getRemainingActionCooldownTicks();
        int atk_number = (me.getLife() + enemy.getDamage() - 1) / enemy.getDamage();
        killing_time += (atk_number - 1) * enemy.getCooldownTicks();
        return killing_time;
    }

    //This will never happen
    return 20000;
}

int Eviscerator::get_myself_death_time(const model::Wizard &me, const model::Wizard &enemy) {
    double attack_range = m_i->g->getWizardCastRange();
    attack_range += me.getRadius();
    double dist = enemy.getDistanceTo(me);

    int cooldown;

    int killing_time = 0;
    //TODO: Honor angle
    if (dist > attack_range) {
        killing_time += (dist - attack_range) / m_i->g->getWizardForwardSpeed();
    }

    //Magic missile
    const auto &remaining_cooldowns = enemy.getRemainingCooldownTicksByAction();
    cooldown = m_i->g->getMagicMissileCooldownTicks();
    int rem_cooldown = std::max(enemy.getRemainingActionCooldownTicks(),
                                remaining_cooldowns[model::ACTION_MAGIC_MISSILE]);
    killing_time += rem_cooldown;

    int damage = m_i->g->getMagicMissileDirectDamage();
    int atk_number = (me.getLife() + damage - 1) / damage;

    killing_time += (atk_number - 1) * cooldown;

    return killing_time;
}

bool Eviscerator::choose_enemy() {
    const auto &buildings = m_i->w->getBuildings();
    const auto &wizards = m_i->w->getWizards();
    const auto &creeps = m_i->w->getMinions();

    double dist;
    double min_dist = 1e9;
    bool first_time = true;

    for (const auto &i : buildings) {
        if (i.getFaction() == model::FACTION_NEUTRAL || i.getFaction() == m_i->s->getFaction()) {
            continue;
        }
        dist = m_i->s->getDistanceTo(i);
        if (first_time || dist < min_dist) {
            first_time = false;
            min_dist = dist;
            m_en_building = &i;
            m_en_minion = nullptr;
            m_en_wz = nullptr;
        }
    }

    for (const auto &i : creeps) {
        if (i.getFaction() == model::FACTION_NEUTRAL || i.getFaction() == m_i->s->getFaction()) {
            continue;
        }
        dist = m_i->s->getDistanceTo(i);
        if (first_time || dist < min_dist) {
            first_time = false;
            min_dist = dist;
            m_en_building = nullptr;
            m_en_minion = &i;
            m_en_wz = nullptr;
        }
    }

    for (const auto &i : wizards) {
        if (i.getFaction() == model::FACTION_NEUTRAL || i.getFaction() == m_i->s->getFaction()) {
            continue;
        }
        dist = m_i->s->getDistanceTo(i);
        if (first_time || dist < min_dist) {
            first_time = false;
            min_dist = dist;
            m_en_building = nullptr;
            m_en_minion = nullptr;
            m_en_wz = &i;
        }
    }

    if (min_dist > config::ENEMY_DETECT_RANGE) {
        m_en_building = nullptr;
        m_en_minion = nullptr;
        m_en_wz = nullptr;
        return false;
    }
    return true;
}

void Eviscerator::destroy(model::Move &move) {
    assert(m_en_building || m_en_minion || m_en_wz);

    const model::LivingUnit *target;
    if (m_en_minion) {
        target = m_en_minion;
    } else if (m_en_wz) {
        target = m_en_wz;
    } else {
        target = m_en_building;
    }

    m_attract_field = std::make_unique<fields::ConstRingField>(
        geom::Point2D{target->getX(), target->getY()},
        m_i->s->getCastRange(),
        config::ENEMY_DETECT_RANGE,
        config::CHOOSEN_ENEMY_ATTRACT
    );

    double distance = m_i->s->getDistanceTo(*target);
    if (distance <= m_i->s->getCastRange()) {
        //Target near
        double angle = m_i->s->getAngleTo(*target);
        const double very_small = 1 * (pi / 180.0);
        if (std::abs(angle) > very_small) {
            move.setTurn(angle);
        }
        if (std::abs(angle) < m_i->g->getStaffSector() / 2.0) {
            //Attack
            if (distance <= m_i->g->getStaffRange()) {
                move.setAction(model::ACTION_STAFF);
            } else {
                move.setAction(model::ACTION_MAGIC_MISSILE);
            }
            move.setCastAngle(angle);
            move.setMinCastDistance(distance - target->getRadius() + m_i->g->getMagicMissileRadius());
        }
    }
}

geom::Vec2D Eviscerator::apply_enemy_attract_field(const model::Wizard &me) {
    assert(m_attract_field);
    return m_attract_field->apply_force(me.getX(), me.getY());
}
