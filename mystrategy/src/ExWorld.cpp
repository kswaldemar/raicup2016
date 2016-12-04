//
// Created by valdemar on 29.11.16.
//

#include "ExWorld.h"
#include "Logger.h"
#include "VisualDebug.h"
#include "model/Game.h"
#include <cassert>

using namespace model;

ExWorld::ExWorld(const model::World &world, const model::Game &game)
    : m_map_size(game.getMapSize()) {
    int my_faction = world.getMyPlayer().getFaction();

    //Set enemy towers as our towers with mirrored coordinates
    m_shadow_towers.clear();
    TowerDesc tds;
    for (const auto &tower : world.getBuildings()) {
        if (tower.getFaction() != my_faction) {
            continue;
        }

        const double map_size = game.getMapSize();

        tds.x = map_size - tower.getX();
        tds.y = map_size - tower.getY();
        tds.r = tower.getRadius();
        tds.cooldown = tower.getCooldownTicks();
        tds.rem_cooldown = tower.getRemainingActionCooldownTicks();
        tds.life = tower.getLife();
        tds.max_life = tower.getMaxLife();
        tds.attack_range = tower.getAttackRange();
        tds.damage = tower.getDamage();

        m_shadow_towers.push_back(tds);
    }

}

void ExWorld::update_world(const model::Wizard &self, const model::World &world, const model::Game &game) {
    const int my_faction = world.getMyPlayer().getFaction();
    m_self_radius = self.getRadius();

    //Update hostile creeps
    m_en_creeps.clear();
    for (const auto &i : world.getMinions()) {
        const bool active = std::abs(i.getSpeedX()) + std::abs(i.getSpeedY()) > 0
                            || i.getRemainingActionCooldownTicks() > 0
                            || i.getLife() < i.getMaxLife();
        if ((i.getFaction() != my_faction && i.getFaction() != FACTION_NEUTRAL)
            || (i.getFaction() == FACTION_NEUTRAL && active)) {
            m_en_creeps.push_back(&i);
        }
    }

    //Update hostile wizards
    m_en_wizards.clear();
    for (const auto &i : world.getWizards()) {
        if (i.getFaction() != my_faction) {
            m_en_wizards.push_back(&i);
        }
    }

    //Update obstacles and FOV
    m_obstacles.clear();
    m_fov.clear();
    for (const auto &i : world.getMinions()) {
        m_obstacles.push_back(&i);

        if (i.getFaction() == my_faction) {
            m_fov.emplace_back(geom::Point2D{i.getX(), i.getY()}, i.getVisionRange());
        }
    }
    for (const auto &i : world.getWizards()) {
        if (!i.isMe()) {
            m_obstacles.push_back(&i);
        }

        if (i.getFaction() == my_faction) {
            m_fov.emplace_back(geom::Point2D{i.getX(), i.getY()}, i.getVisionRange());
        }
    }
    for (const auto &i : world.getBuildings()) {
        m_obstacles.push_back(&i);

        if (i.getFaction() == my_faction) {
            m_fov.emplace_back(geom::Point2D{i.getX(), i.getY()}, i.getVisionRange());
        }
    }
    for (const auto &i : world.getTrees()) {
        m_obstacles.push_back(&i);
    }

    update_shadow_towers(world);


    //Calculate wizards speed factor
    for (const auto &wizard : world.getWizards()) {
        double factor = 1.0;

        //Check for haste
        const auto &statuses = wizard.getStatuses();
        for (const auto &st : statuses) {
            if (st.getType() == model::STATUS_HASTENED) {
                factor += game.getHastenedMovementBonusFactor();
                break;
            }
        }

        //Passive skills
        for (const auto &skill : wizard.getSkills()) {
            if (skill == model::SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_1
                || skill == model::SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_2) {
                factor += game.getMovementBonusFactorPerSkillLevel();
            }
        }

        //Allies aura
        int max_aura = 0;
        for (const auto &ally : world.getWizards()) {
            if (ally.getFaction() != wizard.getFaction()) {
                continue;
            }
            for (const auto &skill : ally.getSkills()) {
                if (ally.getDistanceTo(wizard) <= game.getAuraSkillRange()
                    && (skill == model::SKILL_MOVEMENT_BONUS_FACTOR_AURA_1
                        || skill == model::SKILL_MOVEMENT_BONUS_FACTOR_AURA_2)) {

                    max_aura = std::max<int>(model::SKILL_MOVEMENT_BONUS_FACTOR_AURA_1, max_aura);
                }
            }
        }
        if (max_aura > 0) {
            max_aura -= model::SKILL_MOVEMENT_BONUS_FACTOR_AURA_1;
            factor += game.getMovementBonusFactorPerSkillLevel() * (max_aura + 1);
        }

        assert(wizard.getId() > 0 && wizard.getId() <= 10);
        m_wizard_speed_factor[wizard.getId()] = factor;
    }

    m_canvas_origin = {self.getX(), self.getY()};
    update_canvas(m_canvas_origin);
}

void ExWorld::update_canvas(const geom::Point2D &origin) {
    m_im_draw.clear();
    for (const auto &i : m_obstacles) {
        geom::Point2D t = {i->getX() - origin.x, i->getY() - origin.y};
        t.x += m_im_draw.MAP_SIZE / 2;
        t.y = -t.y;
        t.y += m_im_draw.MAP_SIZE / 2;
        auto translated = m_im_draw.to_internal(t.x, t.y);
        if (m_im_draw.is_correct_point(translated)) {
            int radius = m_im_draw.to_internal(i->getRadius() + 35 - m_im_draw.GRID_SIZE);
            m_im_draw.draw_circle(translated, radius);
        }
    }
}

void ExWorld::update_shadow_towers(const World &world) {
    int my_faction = world.getMyPlayer().getFaction();

    /*
     * First decrease cooldown by one
     */
    for (auto &shadow : m_shadow_towers) {
        if (shadow.rem_cooldown > 0) {
            shadow.rem_cooldown--;
        }
    }

    /*
     * Looks for enemy towers in building list, maybe we see some of them
     */
    std::vector<bool> updated(m_shadow_towers.size(), false);
    for (const auto &enemy_tower : world.getBuildings()) {
        if (enemy_tower.getFaction() == my_faction) {
            continue;
        }
        //Search corresponding tower among shadows
        int idx = 0;
        for (auto &shadow : m_shadow_towers) {
            if (enemy_tower.getId() == shadow.id
                || enemy_tower.getDistanceTo(shadow.x, shadow.y) <= enemy_tower.getRadius()) {
                //Found, update info
                shadow.id = enemy_tower.getId();
                shadow.rem_cooldown = enemy_tower.getRemainingActionCooldownTicks();
                shadow.life = enemy_tower.getLife();
                updated[idx] = true;
            }
            ++idx;
        }
    }

    /*
     * Check for destroy, if we see place, but don't see tower, it is destroyed
     */
    geom::Vec2D dist{0, 0};
    for (const auto &unit_fov : m_fov) {
        int idx = 0;
        auto it = m_shadow_towers.cbegin();
        while (it != m_shadow_towers.cend()) {
            dist.x = it->x - unit_fov.first.x;
            dist.y = it->y - unit_fov.first.y;
            if (dist.sqr() < (unit_fov.second * unit_fov.second) && !updated[idx]) {
                //We should see it, but it not appeared in world.getBuildings, so it is destroyed
                LOG("Don't see tower: %3lf <= %3lf, %d\n", dist.len(), unit_fov.second, (int) updated[idx]);
                it = m_shadow_towers.erase(it);
            } else {
                ++it;
            }
            ++idx;
        }
    }
}

const std::vector<const model::Minion *> &ExWorld::get_hostile_creeps() const {
    return m_en_creeps;
}

const std::vector<const model::Wizard *> &ExWorld::get_hostile_wizards() const {
    return m_en_wizards;
}

const std::vector<const model::CircularUnit *> &ExWorld::get_obstacles() const {
    return m_obstacles;
}

const std::vector<TowerDesc> &ExWorld::get_hostile_towers() const {
    return m_shadow_towers;
}

bool ExWorld::check_no_collision(const geom::Point2D &obj, const double radius) const {
    const bool wall = obj.x <= radius || obj.x >= m_map_size - radius
                      || obj.y <= radius || obj.y >= m_map_size - radius;
    if (wall) {
        return false;
    }
    double sqrradius;
    geom::Vec2D dist{0, 0};
    for (const auto &obstacle : m_obstacles) {
        sqrradius = radius + obstacle->getRadius();
        sqrradius = sqrradius * sqrradius;
        dist.x = obstacle->getX() - obj.x;
        dist.y = obstacle->getY() - obj.y;
        if (dist.sqr() <= sqrradius) {
            return false;
        }
    }
    return true;
}

bool ExWorld::check_in_team_vision(const geom::Point2D &pt) const {
    geom::Vec2D dist{0, 0};
    for (const auto &unit_fov : m_fov) {
        dist.x = pt.x - unit_fov.first.x;
        dist.y = pt.y - unit_fov.first.y;
        if (dist.sqr() <= (unit_fov.second * unit_fov.second)) {
            return true;
        }
    }
    return false;
}

double ExWorld::get_wizard_movement_factor(const Wizard &who) const {
    return m_wizard_speed_factor[who.getId()];
}

bool ExWorld::line_of_sight(double x1, double y1, double x2, double y2) const {
    geom::Point2D pt1 = {x1 - m_canvas_origin.x, y1 - m_canvas_origin.y};
    geom::Point2D pt2 = {x2 - m_canvas_origin.x, y2 - m_canvas_origin.y};
    static const geom::Point2D shift(m_im_draw.MAP_SIZE / 2, m_im_draw.MAP_SIZE / 2);
    pt1.y = -pt1.y;
    pt2.y = -pt2.y;
    pt1 += shift;
    pt2 += shift;

    auto tr1 = m_im_draw.to_internal(pt1.x, pt1.y);
    auto tr2 = m_im_draw.to_internal(pt2.x, pt2.y);

    return m_im_draw.check_line_intersection(tr1.x, tr1.y, tr2.x, tr2.y);
}

void ExWorld::show_me_canvas() const {
    static const geom::Point2D shift(m_im_draw.MAP_SIZE / 2, m_im_draw.MAP_SIZE / 2);
    for (int x = 0; x < m_im_draw.CELL_COUNT; ++x) {
        for (int y = 0; y < m_im_draw.CELL_COUNT; ++y) {
            if (m_im_draw.get_pixel(x, y)) {
                double wx = x * m_im_draw.GRID_SIZE + m_canvas_origin.x - shift.x;
                double wy = -(y * m_im_draw.GRID_SIZE - shift.y) + m_canvas_origin.y;
                VISUAL(fillCircle(wx, wy, m_im_draw.GRID_SIZE / 2, 0x118800));
            }
        }
    }
}
