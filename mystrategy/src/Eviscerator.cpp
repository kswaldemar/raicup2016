//
// Created by valdemar on 12.11.16.
//

#include "model/ActionType.h"
#include "model/Move.h"
#include "Eviscerator.h"
#include "FieldsDescription.h"
#include "VisualDebug.h"
#include "Logger.h"
#include "BehaviourConfig.h"

#include <cassert>
#include <algorithm>

Eviscerator::Eviscerator(const InfoPack &info)
    : m_i(&info) {
}

void Eviscerator::update_info(const InfoPack &info) {
    m_i = &info;
    m_target = nullptr;

    const auto my_faction = m_i->s->getFaction();

    //Setup enemy minion targets
    m_minion_target_by_id.clear();
    for (const auto &enemy : m_i->ew->get_hostile_creeps()) {
        double min_dist = enemy->getVisionRange();
        const model::LivingUnit *choosen = nullptr;
        for (const auto &tower : m_i->w->getBuildings()) {
            if (tower.getFaction() != my_faction) {
                continue;
            }
            double dist = enemy->getDistanceTo(tower);
            if (dist < min_dist) {
                choosen = &tower;
                min_dist = dist;
            }
        }

        for (const auto &i : m_i->w->getMinions()) {
            bool active = std::abs(i.getSpeedX()) + std::abs(i.getSpeedY()) > 0
                          || i.getRemainingActionCooldownTicks() > 0
                          || i.getLife() < i.getMaxLife();
            if (i.getFaction() == my_faction || (active && i.getFaction() == model::FACTION_NEUTRAL)) {
                double dist = enemy->getDistanceTo(i);
                if (dist < min_dist) {
                    choosen = &i;
                    min_dist = dist;
                }
            }
        }

        for (const auto &i : m_i->w->getWizards()) {
            if (i.getFaction() == my_faction) {
                double dist = enemy->getDistanceTo(i);
                if (dist < min_dist) {
                    choosen = &i;
                    min_dist = dist;
                }
            }
        }

        if (choosen) {
            m_minion_target_by_id[enemy->getId()] = std::make_pair(choosen, min_dist);
        }
    }

    m_bhandler.update_projectiles(*info.w, *info.g);
    m_bullet_map.clear();
    const geom::Point2D me{m_i->s->getX(), m_i->s->getY()};
    const double my_radius = m_i->s->getRadius();
    const double my_speed = m_i->g->getWizardBackwardSpeed() * m_i->ew->get_wizard_movement_factor(*m_i->s);
    const double bullet_extra_radius = 1;
    for (const auto &i : m_bhandler.get_bullets()) {
        auto hds = BulletHandler::calc_hit_description(i, me, my_radius);

        const double k = (BehaviourConfig::bullet.missile / (my_radius + i.radius + bullet_extra_radius)) / 2.0;
        bool create = true;

        if (hds.ticks > 0) {
            //TODO: Check possible evasion through simulation
            const double extra_dist = i.radius + my_radius - hds.dist;
            int need_time = static_cast<int>(ceil(extra_dist / my_speed));
            if (need_time > hds.ticks) {
                //Need more time, impossible to evade
                create = false;
            }
        }

        if (create) {
            m_bullet_map.add_field(std::make_unique<fields::LinearField>(
                hds.hit_pt, 0, my_radius + i.radius + bullet_extra_radius,
                -k, BehaviourConfig::bullet.missile
            ));
        }
        //LOG("Tick %d; ticks = %d; distance = %3.5lf; Create bullet field = %d;\n", m_i->w->getTickIndex(), hds.ticks, hds.dist, create);
    }
}

double Eviscerator::choose_enemy() {
    m_possible_targets.clear();

    const auto &conf = BehaviourConfig::targeting;

    double score;
    for (const auto &i : m_i->ew->get_hostile_creeps()) {
        const double dist = m_i->s->getDistanceTo(*i);
        if (dist + i->getRadius() > m_i->s->getCastRange()) {
            //Hunt minions only in cast range
            continue;
        }

        const double life_ratio = 1.0 - static_cast<double>(i->getLife()) / i->getMaxLife();
        if (i->getType() == model::MINION_ORC_WOODCUTTER) {
            score = conf.orc_1;
            score += (conf.orc_2 - conf.orc_1) * life_ratio;
        } else {
            score = conf.fetish_1;
            score += (conf.fetish_2 - conf.fetish_1) * life_ratio;
        }
        score += (1.0 - dist / BehaviourConfig::enemy_detect_distance) * conf.dist_w;

        if (is_minion_attack_me(*i)) {
            score += conf.minion_attack_extra;
            score +=
                conf.minion_attack_my_hp_extra * (1.0 - static_cast<double>(m_i->s->getLife()) / m_i->s->getMaxLife());
        }

        m_possible_targets.emplace_back(
            i,
            i->getType() == model::MINION_ORC_WOODCUTTER ? EnemyDesc::Type::MINION_ORC
                                                         : EnemyDesc::Type::MINION_FETISH,
            score
        );
    }

    for (const auto &i : m_i->w->getBuildings()) {
        if (i.getFaction() == m_i->s->getFaction()) {
            continue;
        }

        const double dist = m_i->s->getDistanceTo(i.getX(), i.getY());
        if (dist > BehaviourConfig::enemy_detect_distance) {
            continue;
        }
        score = conf.tower_1;
        const double life_ratio = 1.0 - static_cast<double>(i.getLife()) / i.getMaxLife();
        score += (conf.tower_2 - conf.tower_1) * life_ratio;
        score += (1.0 - dist / BehaviourConfig::enemy_detect_distance) * conf.dist_w;

        m_possible_targets.emplace_back(&i, EnemyDesc::Type::TOWER, score);
    }

    for (const auto &i : m_i->ew->get_hostile_wizards()) {
        const double dist = m_i->s->getDistanceTo(*i);
        if (dist > BehaviourConfig::enemy_detect_distance) {
            continue;
        }

        score = (1.0 - dist / BehaviourConfig::enemy_detect_distance) * conf.dist_w;

        //TODO: Improve with bullet prediction
        double extra = 0;
        if (dist - i->getRadius() < m_i->s->getCastRange()) {
            score += conf.wizard_in_range;
        }

        const double life_ratio = 1.0 - static_cast<double>(i->getLife()) / i->getMaxLife();
        score += (conf.wizard_2 - conf.wizard_1) * life_ratio;
        m_possible_targets.emplace_back(i, EnemyDesc::Type::WIZARD, score);
    }

    std::sort(
        m_possible_targets.begin(),
        m_possible_targets.end(),
        [](const EnemyDesc &lhs, const EnemyDesc &rhs) -> bool {
            return lhs.score > rhs.score;
        }
    );

    if (m_possible_targets.empty()) {
        return 0;
    } else {
        return m_possible_targets.front().score;
    }
}

Eviscerator::DestroyDesc Eviscerator::destroy(model::Move &move) {
    assert(!m_possible_targets.empty());
    m_target = &m_possible_targets.front();

    assert(m_target);

    const model::LivingUnit &unit = *m_target->unit;
    double min_range;
    if (m_target->type == EnemyDesc::Type::MINION_FETISH) {
        min_range = unit.getRadius() + m_i->g->getMinionVisionRange();
    } else if (m_target->type == EnemyDesc::Type::MINION_ORC) {
        min_range = unit.getRadius() + m_i->g->getStaffRange();
    } else {
        min_range = m_i->s->getCastRange() + unit.getRadius();
    }

    if (m_target->type == EnemyDesc::Type::WIZARD && unit.getLife() < 50) {
        min_range -= (50 - unit.getLife()) * 2;
    }

    //Change aim point to hit if it moving forward
    geom::Point2D target_pt = {unit.getX(), unit.getY()};
    if (m_target->type == EnemyDesc::Type::WIZARD) {
        geom::Vec2D shift = geom::sincos(unit.getAngle());
        shift *= 8.0;
        target_pt += shift;
    }


    VISUAL(line(m_i->s->getX(), m_i->s->getY(), target_pt.x, target_pt.y, 0x0000FF));

    VISUAL(circle(unit.getX(), unit.getY(), min_range, 0x004400));
    VISUAL(circle(unit.getX(), unit.getY(), 700, 0x008800));

    double distance = m_i->s->getDistanceTo(target_pt.x, target_pt.y);
    //double distance = m_i->s->getDistanceTo(unit) - unit.getRadius();

    const auto &skills = m_i->s->getSkills();
    const auto &cooldowns = m_i->s->getRemainingCooldownTicksByAction();

    const bool has_fireball = std::find(skills.cbegin(), skills.cend(), model::SKILL_FIREBALL) != skills.cend();
    const bool has_frostbolt = std::find(skills.cbegin(), skills.cend(), model::SKILL_FROST_BOLT) != skills.cend();

    //Choose attack method, order set priority
    double attack_range; //Depend on skill
    double missile_radius = 0;
    model::ActionType chosen = model::ACTION_NONE;

    static const double fireball_effective_dmg = 100;
    double ang;
    int time;
    if (chosen == model::ACTION_NONE && has_fireball) {
        missile_radius = m_i->g->getFireballRadius();
        const auto fire_point = get_fireball_best_point();
        const double mana_ratio = m_i->s->getMana() / m_i->s->getMaxMana();
        if (fire_point.second >= fireball_effective_dmg || (fire_point.second >= 70 && mana_ratio > 0.7)) {
            auto pt = fire_point.first;
            VISUAL(fillCircle(pt.x, pt.y, 30, 0x41C6BF));
            LOG("Fireball choosen, predicted damage = %3.1lf\n", fire_point.second);
            ang = m_i->s->getAngleTo(pt.x, pt.y);
            time = static_cast<int>(ceil(std::abs(ang) / m_i->g->getWizardMaxTurnAngle()));
            if (cooldowns[model::ACTION_FIREBALL] - time < 1) {
                chosen = model::ACTION_FIREBALL;
                target_pt = pt;
                distance = m_i->s->getDistanceTo(target_pt.x, target_pt.y);
            }
        }
    }

    //if (chosen == model::ACTION_NONE && has_frostbolt && cooldowns[model::ACTION_FROST_BOLT] == 0) {
    //    attack_range = m_i->s->getCastRange();
    //    missile_radius = m_i->g->getFrostBoltRadius();
    //    if (distance <= attack_range) {
    //        chosen = model::ACTION_FROST_BOLT;
    //    }
    //}

    if (chosen == model::ACTION_NONE) {
        attack_range = m_i->s->getCastRange();
        missile_radius = m_i->g->getMagicMissileRadius();
        ang = m_i->s->getAngleTo(target_pt.x, target_pt.y);
        time = static_cast<int>(ceil(std::abs(ang) / m_i->g->getWizardMaxTurnAngle()));
        if (distance <= attack_range + unit.getRadius() && cooldowns[model::ACTION_MAGIC_MISSILE] - time < 1) {
            chosen = model::ACTION_MAGIC_MISSILE;
        }
    }

    if (chosen == model::ACTION_NONE) {
        //Force use staff
        const double sq_dist = 200 * 200;
        const EnemyDesc *best = nullptr;
        geom::Vec2D d;
        double min_sqrdist = sq_dist;
        for (const auto &i : m_possible_targets) {
            d.x = i.unit->getX() - m_i->s->getX();
            d.y = i.unit->getY() - m_i->s->getY();
            if (d.sqr() < min_sqrdist) {
                best = &i;
                min_sqrdist = d.sqr();
            }
        }
        if (best) {
            target_pt.x = best->unit->getX();
            target_pt.y = best->unit->getY();
            distance = d.len();
            VISUAL(fillCircle(target_pt.x, target_pt.y, 7, 0x006600));
            min_range = best->unit->getRadius() + m_i->g->getStaffRange();
            if (distance <= min_range) {
                chosen = model::ACTION_STAFF;
            }
        }
    }

    const double angle = m_i->s->getAngleTo(target_pt.x, target_pt.y);
    const double can_shoot = std::abs(angle) < m_i->g->getStaffSector() / 2.0;

    move.setCastAngle(angle);
    move.setMinCastDistance(distance - missile_radius);

    if (distance <= m_i->s->getVisionRange()) {
        move.setTurn(angle);
    }

    if (chosen != model::ACTION_NONE && can_shoot) {
        if (chosen == model::ACTION_FIREBALL) {
            move.setMaxCastDistance(m_i->s->getDistanceTo(target_pt.x, target_pt.y));
        }
        move.setAction(chosen);
    }

    if (!can_shoot) {
        //Cannot attack desired enemy right now, try to find extra enemy to punish
        for (const auto &i : m_possible_targets) {
            double d = m_i->s->getDistanceTo(*i.unit) - i.unit->getRadius();
            double a = m_i->s->getAngleTo(*i.unit);
            if (d <= m_i->g->getStaffRange() && std::abs(a) < m_i->g->getStaffSector() / 2.0) {
                LOG("Extra enemy push!\n");
                move.setAction(model::ACTION_STAFF);
            }
        }
    }

    return {target_pt, min_range};
}

void Eviscerator::destroy(model::Move &move, const geom::Point2D &center, double radius) const {
    const double dist = m_i->s->getDistanceTo(center.x, center.y);
    const double angle = m_i->s->getAngleTo(center.x, center.y);
    const bool in_sector = std::abs(angle) < m_i->g->getStaffSector() / 2.0;
    if (dist <= radius + m_i->g->getStaffRange() && in_sector) {
        move.setAction(model::ACTION_STAFF);
    } else if (dist <= radius + m_i->s->getCastRange() && in_sector) {
        move.setAction(model::ACTION_MAGIC_MISSILE);
        move.setCastAngle(angle);
    }

    move.setTurn(angle);
}

bool Eviscerator::tower_maybe_attack_me(const MyBuilding &tower) {
    auto enemy_faction = m_i->s->getFaction() == model::FACTION_ACADEMY ? model::FACTION_RENEGADES
                                                                        : model::FACTION_ACADEMY;
    std::vector<const model::LivingUnit *> maybe_targets;
    geom::Vec2D dist;
    const double att_rad = tower.getAttackRange() * tower.getAttackRange();
    for (const auto &minion : m_i->w->getMinions()) {
        if (minion.getFaction() == enemy_faction) {
            continue;
        }
        bool active = std::abs(minion.getSpeedX()) + std::abs(minion.getSpeedY()) > 0
                      || minion.getRemainingActionCooldownTicks() > 0
                      || minion.getLife() < minion.getMaxLife()
                      || minion.getFaction() == m_i->s->getFaction();
        dist = {tower.getX() - minion.getX(), tower.getY() - minion.getY()};
        if (active && dist.sqr() <= att_rad) {
            maybe_targets.emplace_back(&minion);
        }
    }
    for (const auto &wizard : m_i->w->getWizards()) {
        if (wizard.getFaction() == enemy_faction) {
            continue;
        }
        dist = {tower.getX() - wizard.getX(), tower.getY() - wizard.getY()};
        if (dist.sqr() <= att_rad) {
            maybe_targets.emplace_back(&wizard);
        }
    }

    int case1_hp = 1000;
    int case2_hp = 0;
    for (const auto &i : maybe_targets) {
        if (i->getLife() >= tower.getDamage()) {
            case1_hp = std::min(i->getLife(), case1_hp);
        } else {
            case2_hp = std::max(i->getLife(), case2_hp);
        }
    }

    bool can_attack = true;
    if (case1_hp < 1000) {
        can_attack = m_i->s->getLife() == case1_hp;
    } else if (case2_hp > 0) {
        can_attack = m_i->s->getLife() == case2_hp;
    }

    //if (m_i->s->getDistanceTo(tower.getX(), tower.getY()) <= 610) {
    //    if (can_attack) {
    //        LOG("Tower definitely CAN attack me\n");
    //    } else {
    //        LOG("Tower cannot attack me!\n");
    //    }
    //}

    return can_attack;
}

bool Eviscerator::can_shoot_to_target() const {
    return !m_possible_targets.empty()
           && m_i->s->getDistanceTo(*m_possible_targets.front().unit)
              <= m_i->s->getCastRange() + m_i->g->getMagicMissileRadius();
}

double Eviscerator::get_minion_aggression_radius(const model::Minion &creep) const {
    const auto it = m_minion_target_by_id.find(creep.getId());
    if (it == m_minion_target_by_id.end()) {
        return m_i->g->getMinionVisionRange();
    } else {
        return it->second.second;
    }
}

bool Eviscerator::is_minion_attack_me(const model::Minion &creep) const {
    const auto it = m_minion_target_by_id.find(creep.getId());
    return it != m_minion_target_by_id.end() && it->second.first->getId() == m_i->s->getId();
}

bool Eviscerator::can_leave_battlefield() const {
    return m_possible_targets.empty() || m_possible_targets.front().score < BehaviourConfig::targeting.can_leave;
}

const fields::FieldMap &Eviscerator::get_bullet_damage_map() const {
    return m_bullet_map;
}

std::pair<geom::Point2D, double> Eviscerator::get_fireball_best_point() const {
    static const std::initializer_list<geom::Vec2D> shifts = {
        {0,  -1},
        {1,  -1},
        {1,  0},
        {1,  1},
        {0,  1},
        {-1, 1},
        {-1, 0},
        {-1, -1}
    };

    const double creep_effective_overlap = 10;
    double max_damage = 0;
    double damage = 0;
    geom::Point2D best_pt;
    geom::Point2D pt;
    double dist;

    const double cast_range = m_i->s->getCastRange();

    for (const auto &target : m_i->ew->get_hostile_creeps()) {
        dist = target->getRadius() + m_i->g->getFireballExplosionMinDamageRange() - creep_effective_overlap;
        for (const auto &shift : shifts) {
            pt.x = target->getX() + shift.x * dist;
            pt.y = target->getY() + shift.y * dist;
            const double to_me = m_i->s->getDistanceTo(pt.x, pt.y);
            if (to_me > m_i->s->getRadius() + m_i->g->getFireballExplosionMinDamageRange() && to_me <= cast_range) {
                damage = get_fireball_damage(pt);
                if (damage > max_damage) {
                    max_damage = damage;
                    best_pt = pt;
                }
            }
        }

        dist = target->getRadius() + m_i->g->getFireballExplosionMaxDamageRange() - 1;
        for (const auto &shift : shifts) {
            pt.x = target->getX() + shift.x * dist;
            pt.y = target->getY() + shift.y * dist;
            const double to_me = m_i->s->getDistanceTo(pt.x, pt.y);
            if (to_me > m_i->s->getRadius() + m_i->g->getFireballExplosionMinDamageRange() && to_me <= cast_range) {
                damage = get_fireball_damage(pt);
                if (damage > max_damage) {
                    max_damage = damage;
                    best_pt = pt;
                }
            }
        }
    }

    for (const auto &target : m_i->ew->get_hostile_wizards()) {
        dist = target->getRadius() + m_i->g->getFireballExplosionMaxDamageRange() - 10;
        for (const auto &shift : shifts) {
            pt.x = target->getX() + shift.x * dist;
            pt.y = target->getY() + shift.y * dist;
            damage = get_fireball_damage(pt);
            const double to_me = m_i->s->getDistanceTo(pt.x, pt.y);
            if (to_me > m_i->s->getRadius() + m_i->g->getFireballExplosionMinDamageRange() && to_me <= cast_range) {
                if (damage > max_damage) {
                    max_damage = damage;
                    best_pt = pt;
                }
            }
        }
    }

    for (const auto &target : m_i->ew->get_hostile_towers()) {
        dist = target.getRadius() + m_i->g->getFireballExplosionMinDamageRange() - 0.1;
        for (const auto &shift : shifts) {
            pt.x = target.getX() + shift.x * dist;
            pt.y = target.getY() + shift.y * dist;
            damage = get_fireball_damage(pt);
            const double to_me = m_i->s->getDistanceTo(pt.x, pt.y);
            if (to_me > m_i->s->getRadius() + m_i->g->getFireballExplosionMinDamageRange() && to_me <= cast_range) {
                if (damage > max_damage) {
                    max_damage = damage;
                    best_pt = pt;
                }
            }
        }

        dist = target.getRadius() + m_i->g->getFireballExplosionMaxDamageRange() - 0.1;
        for (const auto &shift : shifts) {
            pt.x = target.getX() + shift.x * dist;
            pt.y = target.getY() + shift.y * dist;
            damage = get_fireball_damage(pt);
            const double to_me = m_i->s->getDistanceTo(pt.x, pt.y);
            if (to_me > m_i->s->getRadius() + m_i->g->getFireballExplosionMinDamageRange() && to_me <= cast_range) {
                if (damage > max_damage) {
                    max_damage = damage;
                    best_pt = pt;
                }
            }
        }
    }

    return std::make_pair(best_pt, max_damage);
}

double Eviscerator::get_fireball_damage(const geom::Point2D &center) const {

    static const double min_dmg_range = m_i->g->getFireballExplosionMinDamageRange();
    static const double max_dmg_range = m_i->g->getFireballExplosionMaxDamageRange();
    static const double max_dmg = m_i->g->getFireballExplosionMaxDamage();
    static const double min_dmg = m_i->g->getFireballExplosionMinDamage();
    static const double k = -(max_dmg - min_dmg) / (max_dmg_range - min_dmg_range);

    static const auto fire_damage = [](double x,
                                       double y,
                                       double radius,
                                       const geom::Point2D &fire_center) -> double {
        double dist = geom::Vec2D(fire_center.x - x, fire_center.y - y).len();
        dist -= radius;
        if (dist > min_dmg_range) {
            return 0;
        } else if (dist < min_dmg_range && dist > max_dmg_range) {
            return k * (dist - max_dmg_range) + max_dmg;
        } else {
            return max_dmg;
        }
    };

    static const double tower_mult = 1.5;
    static const double wizard_mult = 3.0;

    geom::Vec2D dist;
    double sqrrad;
    double sum = 0;
    double damage;
    for (const auto &i : m_i->ew->get_hostile_creeps()) {
        dist.x = center.x - i->getX();
        dist.y = center.y - i->getY();
        sqrrad = i->getRadius() + min_dmg_range - 10;
        if (dist.sqr() < sqrrad * sqrrad) {
            damage = fire_damage(i->getX(), i->getY(), i->getRadius(), center);
            damage += m_i->g->getBurningSummaryDamage();
            damage = std::min<double>(damage, i->getLife());
            sum += damage;
        }
    }

    for (const auto &i : m_i->ew->get_hostile_towers()) {
        dist = i.getPoint() - center;
        sqrrad = i.getRadius() + min_dmg_range - 0.1;
        if (dist.sqr() < sqrrad * sqrrad) {
            damage = fire_damage(i.getX(), i.getY(), i.getRadius(), center);
            damage += m_i->g->getBurningSummaryDamage();
            damage = std::min<double>(damage, i.getLife());
            damage *= tower_mult;
            sum += damage;
        }
    }

    for (const auto &i : m_i->ew->get_hostile_wizards()) {
        dist.x = center.x - i->getX();
        dist.y = center.y - i->getY();
        sqrrad = i->getRadius() + min_dmg_range - 30;
        if (dist.sqr() < sqrrad * sqrrad) {
            damage = fire_damage(i->getX(), i->getY(), i->getRadius(), center);
            damage += m_i->g->getBurningSummaryDamage();
            damage = std::min<double>(damage, i->getLife());
            damage *= wizard_mult;
            sum += damage;
        }
    }

    return sum;
}
