//
// Created by valdemar on 12.11.16.
//

#include "model/ActionType.h"
#include "model/Move.h"
#include "Config.h"
#include "Eviscerator.h"
#include "FieldsDescription.h"
#include "VisualDebug.h"

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

    double angle = std::abs(enemy.getAngleTo(me));
    killing_time += angle / m_i->g->getMinionMaxTurnAngle();

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

int Eviscerator::get_myself_death_time(const model::Wizard &me, const TowerDesc &enemy) {
    double attack_range = enemy.attack_range;
    attack_range += me.getRadius();
    double dist = me.getDistanceTo(enemy.x, enemy.y);

    if (dist <= attack_range) {
        int killing_time = 0;
        killing_time += enemy.rem_cooldown;
        int atk_number = (me.getLife() + enemy.damage - 1) / enemy.damage;
        killing_time += (atk_number - 1) * enemy.cooldown;
        return killing_time;
    }

    //This will never happen
    return 20000;
}

double Eviscerator::calc_dead_zone(const RunawayUnit &me, const AttackUnit &enemy) {
    //Assume that we are in shooting range
    int killing_time = 0;
    killing_time += enemy.rem_cooldown;
    int atk_number = (me.life + enemy.dmg - 1) / enemy.dmg;
    killing_time += (atk_number - 1) * enemy.cooldown;

    return enemy.range - (killing_time * me.speed);
}

bool Eviscerator::choose_enemy() {
    const auto &buildings = m_i->w->getBuildings();
    const auto &wizards = m_i->w->getWizards();
    const auto &creeps = m_i->w->getMinions();

    double dist;
    double min_dist = 1e9;
    bool first_time = true;

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

    if (m_en_wz && m_i->s->getDistanceTo(*m_en_wz) <= config::ENEMY_DETECT_RANGE) {
        return true;
    }


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


    if (min_dist > config::ENEMY_DETECT_RANGE) {
        m_en_building = nullptr;
        m_en_minion = nullptr;
        m_en_wz = nullptr;
        return false;
    }
    return true;
}

Eviscerator::DestroyDesc Eviscerator::destroy(model::Move &move) {
    assert(m_en_building || m_en_minion || m_en_wz);

    const model::LivingUnit *target;
    if (m_en_minion) {
        target = m_en_minion;
    } else if (m_en_wz) {
        target = m_en_wz;
    } else {
        target = m_en_building;
    }

    VISUAL(line(m_i->s->getX(), m_i->s->getY(), target->getX(), target->getY(), 0x0000FF));

    double min_range = m_i->s->getCastRange() + target->getRadius() + m_i->g->getMagicMissileRadius();
    if (target->getLife() < 50) {
        min_range -= (50 - target->getLife()) * 2;
    }
    m_attract_field = std::make_unique<fields::ConstRingField>(
        geom::Point2D{target->getX(), target->getY()},
        min_range,
        config::ENEMY_DETECT_RANGE,
        config::CHOOSEN_ENEMY_ATTRACT
    );

    VISUAL(circle(target->getX(), target->getY(), min_range, 0x004400));
    VISUAL(circle(target->getX(), target->getY(), config::ENEMY_DETECT_RANGE, 0x008800));

    double distance = m_i->s->getDistanceTo(*target) - target->getRadius() - m_i->g->getMagicMissileRadius();
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
            move.setMinCastDistance(distance);
        }
    }
    return {target, min_range};
}

geom::Vec2D Eviscerator::apply_enemy_attract_field(const model::Wizard &me) {
    assert(m_attract_field);
    return m_attract_field->apply_force(me.getX(), me.getY());
}