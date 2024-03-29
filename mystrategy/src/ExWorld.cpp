//
// Created by valdemar on 29.11.16.
//

#include "ExWorld.h"
#include "Logger.h"
#include "VisualDebug.h"
#include "MyLivingUnit.h"
#include "model/Game.h"
#include <cassert>

using namespace model;

ExWorld::ExWorld(const model::World &world, const model::Game &game)
    : m_map_size(game.getMapSize()) {
    int my_faction = world.getMyPlayer().getFaction();

    //Set enemy towers as our towers with mirrored coordinates
    for (const auto &tower : world.getBuildings()) {
        if (tower.getFaction() != my_faction) {
            continue;
        }

        const double map_size = game.getMapSize();

        m_shadow_towers.emplace_back(
            map_size - tower.getX(),
            map_size - tower.getY(),
            tower.getRadius(),
            tower.getDamage(),
            tower.getAttackRange(),
            tower.getMaxLife(),
            tower.getCooldownTicks()
        );
    }

    m_bonuses.emplace_back(1200, 1200, world.getTickIndex());
    m_bonuses.emplace_back(2800, 2800, world.getTickIndex());
}

void ExWorld::update_world(const model::Wizard &self, const model::World &world, const model::Game &game) {
    const int my_faction = world.getMyPlayer().getFaction();
    self.getRadius();

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

    //Should be first, to have actual FOV info
    update_obstacles_and_fov(world);

    update_shadow_towers(world);

    update_bonuses(world);

    update_wizards_speed_factors(world, game);

    m_canvas_origin = {self.getX(), self.getY()};
    update_canvas(m_canvas_origin);
}

void ExWorld::update_obstacles_and_fov(const World &world)  {
    int my_faction = world.getMyPlayer().getFaction();

    m_obstacles.clear();
    m_fov.clear();
    for (const auto &i : world.getMinions()) {
        m_obstacles.emplace_back(MyLivingUnit::MINION, i);
        if (i.getFaction() == my_faction) {
            m_fov.emplace_back(geom::Point2D{i.getX(), i.getY()}, i.getVisionRange());
        }
    }
    for (const auto &i : world.getWizards()) {
        if (!i.isMe()) {
            m_obstacles.emplace_back(MyLivingUnit::WIZARD, i);
        }
        if (i.getFaction() == my_faction) {
            m_fov.emplace_back(geom::Point2D{i.getX(), i.getY()}, i.getVisionRange());
        }
    }
    for (const auto &i : world.getBuildings()) {
        m_obstacles.emplace_back(MyLivingUnit::TOWER, i);
        if (i.getFaction() == my_faction) {
            m_fov.emplace_back(geom::Point2D{i.getX(), i.getY()}, i.getVisionRange());
        }
    }

    std::set<int64_t> visible_trees;
    for (const auto &i : world.getTrees()) {
        if (m_known_trees.find(i.getId()) == m_known_trees.cend()) {
            m_trees.emplace_back(MyLivingUnit::TREE, i);
            m_known_trees.insert(i.getId());
        }
        visible_trees.insert(i.getId());
    }


    if (world.getTickIndex() % TREE_UPDATE_FREQ == 0) {
        //Check for destroyed trees
        for (auto obs = m_trees.cbegin(); obs != m_trees.cend();) {
            const bool visible = visible_trees.find(obs->getId()) != visible_trees.cend();
            if (!visible && point_in_team_vision(obs->getX(), obs->getY())) {
                obs = m_trees.erase(obs);
            } else {
                ++obs;
            }
        }
    }

    for (const auto &tree : m_trees) {
        m_obstacles.push_back(tree);
    }
}

void ExWorld::update_canvas(const geom::Point2D &origin) {
    m_im_draw.clear();
    for (const auto &i : m_obstacles) {
        geom::Point2D t = {i.getX() - origin.x, i.getY() - origin.y};
        t.x += m_im_draw.MAP_SIZE / 2;
        t.y = -t.y;
        t.y += m_im_draw.MAP_SIZE / 2;
        auto translated = m_im_draw.to_internal(t.x, t.y);
        if (m_im_draw.is_correct_point(translated)) {
            int radius = m_im_draw.to_internal(i.getRadius() + 35/* - m_im_draw.GRID_SIZE*/);
            m_im_draw.draw_circle(translated, radius);
        }
    }
}

void ExWorld::update_shadow_towers(const World &world) {
    int my_faction = world.getMyPlayer().getFaction();

    /*
     * Update remaining cooldown
     */
    for (auto &shadow : m_shadow_towers) {
        shadow.updateClock(world.getTickIndex());
    }

    /*
     * Looks for enemy towers in building list, maybe we see some of them
     */
    std::vector<bool> visible(m_shadow_towers.size(), false);
    for (const auto &enemy_tower : world.getBuildings()) {
        if (enemy_tower.getFaction() == my_faction) {
            continue;
        }
        //Search corresponding tower among shadows
        int idx = 0;
        for (auto &shadow : m_shadow_towers) {
            if (shadow.isSame(enemy_tower)) {
                //Found, update info
                shadow.update(enemy_tower);
                visible[idx] = true;
            }
            ++idx;
        }
    }

    /*
     * Check for destroy, if we see place, but don't see tower, it is destroyed
     */
    int idx = 0;
    auto it = m_shadow_towers.cbegin();
    while (it != m_shadow_towers.cend()) {
        if (point_in_team_vision(it->getPoint()) && !visible[idx]) {
            //We should see it, but it not appeared in world.getBuildings, so it is destroyed
            LOG("Don't see tower: (%3.1lf, %3.1lf)\n", it->getX(), it->getY());
            it = m_shadow_towers.erase(it);
        } else {
            ++it;
        }
        ++idx;
    }
}

void ExWorld::update_bonuses(const model::World &world) {
    for (auto &b : m_bonuses) {
        b.update(world.getTickIndex());
        if (point_in_team_vision(b.getX(), b.getY()) && b.getRemainingTime() == 0) {
            bool visible = false;
            for (const auto &i : world.getBonuses()) {
                if (eps_equal(i.getX(), b.getX()) && eps_equal(i.getY(), b.getY())) {
                    visible = true;
                    break;
                }
            }
            if (!visible) {
                b.setTaken();
            }
        }
    }
}

void ExWorld::update_wizards_speed_factors(const model::World &world, const model::Game &game) {
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

        //assert(wizard.getId() > 0 && wizard.getId() <= 10);
        m_wizard_speed_factor[wizard.getId()] = factor;
    }
}

const std::vector<const model::Minion *> &ExWorld::get_hostile_creeps() const {
    return m_en_creeps;
}

const std::vector<const model::Wizard *> &ExWorld::get_hostile_wizards() const {
    return m_en_wizards;
}

const std::vector<MyLivingUnit> &ExWorld::get_obstacles() const {
    return m_obstacles;
}

const std::vector<MyBuilding> &ExWorld::get_hostile_towers() const {
    return m_shadow_towers;
}

const std::vector<MyBonus> &ExWorld::get_bonuses() const {
    return m_bonuses;
}

bool ExWorld::check_no_collision(geom::Point2D obj, double radius, const MyLivingUnit **out_obstacle) const {
    const bool wall = obj.x <= radius || obj.x >= m_map_size - radius
                      || obj.y <= radius || obj.y >= m_map_size - radius;
    if (wall) {
        return false;
    }
    double sqrradius;
    geom::Vec2D dist{0, 0};

    for (const auto &obstacle : m_obstacles) {
        sqrradius = radius + obstacle.getRadius();
        sqrradius = sqrradius * sqrradius;
        dist.x = obstacle.getX() - obj.x;
        dist.y = obstacle.getY() - obj.y;
        if (dist.sqr() <= sqrradius) {
            if (out_obstacle) {
                *out_obstacle = &obstacle;
            }
            return false;
        }
    }
    return true;
}

bool ExWorld::point_in_team_vision(const geom::Point2D &pt) const {
    return point_in_team_vision(pt.x, pt.y);
}

bool ExWorld::point_in_team_vision(double x, double y) const {
    geom::Vec2D dist{0, 0};
    for (const auto &unit_fov : m_fov) {
        dist.x = x - unit_fov.first.x;
        dist.y = y - unit_fov.first.y;
        if (dist.sqr() <= (unit_fov.second * unit_fov.second)) {
            return true;
        }
    }
    return false;
}

double ExWorld::get_wizard_movement_factor(const Wizard &who) const {
    return m_wizard_speed_factor[who.getId()];
}

double ExWorld::get_wizard_turn_factor(const model::Wizard &who) const {
    return 1;
}

double ExWorld::get_wizard_magical_damage_absorb_factor() const {
    return 1;
}

double ExWorld::get_wizard_damage_absorb_factor() const {
    return 1;
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
