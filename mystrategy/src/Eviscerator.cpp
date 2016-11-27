//
// Created by valdemar on 12.11.16.
//

#include "model/ActionType.h"
#include "model/Move.h"
#include "Config.h"
#include "Eviscerator.h"
#include "FieldsDescription.h"
#include "VisualDebug.h"
#include "Logger.h"

#include <cassert>
#include <algorithm>

Eviscerator::Eviscerator(const InfoPack &info)
    : m_i(&info) {
}

void Eviscerator::update_info(const InfoPack &info) {
    m_i = &info;

    m_attract_field = nullptr;
    m_target = nullptr;

    //Create enemies list
    m_enemies.clear();
    const auto my_faction = m_i->s->getFaction();
    for (const auto &i : m_i->w->getWizards()) {
        if (i.getFaction() != my_faction) {
            m_enemies.emplace_back(&i, EnemyDesc::Type::WIZARD);
        }
    }
    for (const auto &i : m_i->w->getMinions()) {
        bool aggresive_neutral = false;
        if (i.getFaction() == model::FACTION_NEUTRAL) {
            //Check for aggression
            aggresive_neutral = std::abs(i.getSpeedX()) + std::abs(i.getSpeedY()) > 0
                                || i.getRemainingActionCooldownTicks() > 0
                                || i.getLife() < i.getMaxLife();
            if (aggresive_neutral) {
                LOG("Neutral aggresive! %lf, %d, %d\n", std::abs(i.getSpeedX()) + std::abs(i.getSpeedY()),
                    i.getRemainingActionCooldownTicks(), i.getLife());
            }
        }
        if (aggresive_neutral || (i.getFaction() != my_faction && i.getFaction() != model::FACTION_NEUTRAL)) {
            if (i.getType() == model::MINION_FETISH_BLOWDART) {
                m_enemies.emplace_back(&i, EnemyDesc::Type::MINION_FETISH);
            } else {
                m_enemies.emplace_back(&i, EnemyDesc::Type::MINION_ORC);
            }
        }
    }
    for (const auto &i : m_i->w->getBuildings()) {
        if (i.getFaction() != my_faction) {
            m_enemies.emplace_back(&i, EnemyDesc::Type::TOWER);
        }
    }
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

int Eviscerator::choose_enemy() {

    const auto estimate = [](const EnemyDesc &enemy, const double dist) -> int {
        int cost = 0;
        const auto &unit = *enemy.unit;
        switch (enemy.type) {
            case EnemyDesc::Type::MINION_ORC:
                if (unit.getFaction() == model::FACTION_NEUTRAL) {
                    cost = 100;
                } else {
                    cost = 600;
                }
                break;
            case EnemyDesc::Type::MINION_FETISH:
                if (unit.getFaction() == model::FACTION_NEUTRAL) {
                    cost = 200;
                } else {
                    cost = 800;
                }
                break;
            case EnemyDesc::Type::WIZARD:
                cost = 1800;
                break;
            case EnemyDesc::Type::TOWER:
                if (unit.getLife() < 50) {
                    cost = 800;
                } else if (unit.getLife() < 25) {
                    cost = 1500;
                }
                break;
        }

        if (dist > config::ENEMY_DETECT_RANGE) {
            return 0;
        }
        return cost
               + 600 - static_cast<int>(dist)
               + static_cast<int>(180.0 * (1.0 - unit.getLife() / unit.getMaxLife()));
    };

    int max_est = 0;
    const EnemyDesc *best = nullptr;
    for (const auto &i : m_enemies) {
        int est = estimate(i, m_i->s->getDistanceTo(*i.unit));
        if (!best || est > max_est) {
            best = &i;
            max_est = est;
        }
    }
    //if (best) {
    //    LOG("Best enemy estimation = %d; (%lf, %lf)\n",
    //        max_est,
    //        best->unit->getX(), best->unit->getY());
    //}
    if (max_est > 0) {
        m_target = best;
    }
    return max_est;
}

Eviscerator::DestroyDesc Eviscerator::destroy(model::Move &move) {
    assert(m_target);

    const model::LivingUnit &unit = *m_target->unit;
    double min_range;
    if (m_target->type == EnemyDesc::Type::MINION_FETISH){
        min_range = unit.getRadius() + m_i->g->getMinionVisionRange();
    } else if (m_target->type == EnemyDesc::Type::MINION_ORC) {
        min_range = unit.getRadius() + m_i->g->getStaffRange();
    } else {
        min_range = m_i->s->getCastRange() + unit.getRadius();
    }

    VISUAL(line(m_i->s->getX(), m_i->s->getY(), unit.getX(), unit.getY(), 0x0000FF));

    if (m_target->type == EnemyDesc::Type::WIZARD && unit.getLife() < 50) {
        min_range -= (50 - unit.getLife()) * 2;
    }

    VISUAL(circle(unit.getX(), unit.getY(), min_range, 0x004400));
    VISUAL(circle(unit.getX(), unit.getY(), config::ENEMY_DETECT_RANGE, 0x008800));

    double distance = m_i->s->getDistanceTo(unit) - unit.getRadius();

    const auto &skills = m_i->s->getSkills();
    const auto &cooldowns = m_i->s->getRemainingCooldownTicksByAction();

    const bool has_fireball = std::find(skills.cbegin(), skills.cend(), model::SKILL_FIREBALL) != skills.cend();
    const bool has_frostbolt = std::find(skills.cbegin(), skills.cend(), model::SKILL_FROST_BOLT) != skills.cend();

    //Choose attack method, order set priority
    double attack_range = unit.getRadius(); //Depend on skill
    double missile_radius = 0;
    model::ActionType chosen = model::ACTION_NONE;

    if (chosen == model::ACTION_NONE && cooldowns[model::ACTION_STAFF] == 0) {
        attack_range = m_i->g->getStaffRange();
        if (distance <= attack_range) {
            chosen = model::ACTION_STAFF;
        }
    }

    if (chosen == model::ACTION_NONE && has_fireball && cooldowns[model::ACTION_FIREBALL] == 0) {

        attack_range = m_i->s->getCastRange() + m_i->g->getFireballRadius() / 2.0;
        missile_radius = m_i->g->getFireballRadius();
        if (distance <= attack_range) {
            chosen = model::ACTION_FIREBALL;
        }
    }

    if (chosen == model::ACTION_NONE && has_frostbolt && cooldowns[model::ACTION_FROST_BOLT] == 0) {
        attack_range = m_i->s->getCastRange();
        missile_radius = m_i->g->getFrostBoltRadius();
        if (distance <= attack_range) {
            chosen = model::ACTION_FROST_BOLT;
        }
    }

    if (chosen == model::ACTION_NONE && cooldowns[model::ACTION_MAGIC_MISSILE] == 0) {
        attack_range = m_i->s->getCastRange();
        missile_radius = m_i->g->getMagicMissileRadius();
        if (distance <= attack_range) {
            chosen = model::ACTION_MAGIC_MISSILE;
        }
    }

    const double angle = m_i->s->getAngleTo(unit);
    const double can_shoot = std::abs(angle) < m_i->g->getStaffSector() / 2.0;

    move.setCastAngle(angle);
    move.setMinCastDistance(distance - missile_radius);

    if (distance <= m_i->s->getVisionRange()) {
        move.setTurn(angle);
    }

    if (chosen != model::ACTION_NONE && can_shoot) {
        move.setAction(chosen);
    }

    if (!can_shoot) {
        //Cannot attack desired enemy right now, try to find extra enemy to punish
        for (const auto &i : m_enemies) {
            double d = m_i->s->getDistanceTo(*i.unit) - i.unit->getRadius();
            double a = m_i->s->getAngleTo(*i.unit);
            if (d <= m_i->g->getStaffRange() && std::abs(a) < m_i->g->getStaffSector() / 2.0) {
                LOG("Extra enemy push!\n");
                move.setAction(model::ACTION_STAFF);
            }
        }
    }

    return {m_target->unit, min_range};
}

geom::Vec2D Eviscerator::apply_enemy_attract_field(const model::Wizard &me) {
    assert(m_attract_field);
    //return m_attract_field->apply_force(me.getX(), me.getY());
    return {0, 0};
}

bool Eviscerator::tower_maybe_attack_me(const TowerDesc &tower) {
    std::vector<const model::LivingUnit*> maybe_targets;
    geom::Vec2D dist;
    const double att_rad = tower.attack_range * tower.attack_range;
    for (const auto &minion : m_i->w->getMinions()) {
        if (minion.getFaction() != m_i->s->getFaction()) {
            continue;
        }
        bool aggresive_neutral = std::abs(minion.getSpeedX()) + std::abs(minion.getSpeedY()) > 0
                            || minion.getRemainingActionCooldownTicks() > 0
                            || minion.getLife() < minion.getMaxLife();
        dist = {tower.x - minion.getX(), tower.y - minion.getY()};
        if (aggresive_neutral && dist.sqr() <= att_rad) {
            maybe_targets.emplace_back(&minion);
        }
    }
    for (const auto &wizard : m_i->w->getWizards()) {
        if (wizard.isMe() || wizard.getFaction() != m_i->s->getFaction()) {
            continue;
        }
        dist = {tower.x - wizard.getX(), tower.y - wizard.getY()};
        if (dist.sqr() <= att_rad) {
            maybe_targets.emplace_back(&wizard);
        }
    }

    int case1_hp = 1000;
    int case2_hp = 0;
    for (const auto &i : maybe_targets) {
        if (i->getLife() >= tower.damage) {
            case1_hp = std::min(i->getLife(), case1_hp);
        } else {
            case2_hp = std::max(i->getLife(), case2_hp);
        }
    }

    if (case1_hp < 1000) {
        return m_i->s->getLife() == case1_hp;
    } else if (case2_hp > 0) {
        return m_i->s->getLife() == case2_hp;
    }
    return true;
}

const std::vector<EnemyDesc> &Eviscerator::get_enemies() const {
    return m_enemies;
}

bool Eviscerator::can_shoot_to_target() const {
    return m_target && m_i->s->getDistanceTo(*m_target->unit) <= m_i->s->getCastRange() + m_i->g->getMagicMissileRadius();
}
