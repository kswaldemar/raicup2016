#include "FieldsDescription.h"
#include "Config.h"
#include "MyStrategy.h"
#include "PathFinder.h"
#include "Logger.h"
#include "CustomField.h"
#include "VisualDebug.h"

#include <cassert>
#include <list>

using namespace model;
using namespace std;
using namespace geom;

const double EPS = 0.00003;
InfoPack g_info;

bool eps_equal(double d1, double d2) {
    const double diff = d1 - d2;
    return diff < EPS && diff > -EPS;
}


MyStrategy::MyStrategy() {
}

void MyStrategy::move(const Wizard &self, const World &world, const Game &game, Move &move) {
    //Initialize info pack first
    initialize_info_pack(self, world, game);

    //Initialize once
    static bool initialized = initialize_strategy(self, world, game);
    assert(initialized);

    //Update information
    each_tick_update(self, world, game);

    //Try learn something
    if (game.isSkillsEnabled()) {
        m_sb->try_level_up(move);
    }

    //Check for stucking in place
    static int hold_time = 0;
    if (std::abs(self.getSpeedX()) + std::abs(self.getSpeedY()) < 0.1) {
        ++hold_time;
    } else {
        hold_time = 0;
    }
    const bool not_moved = hold_time >= 3;
    if (not_moved) {
        LOG("Tick %d: Not moved in last tick - spd (%3.1lf, %3.1lf)\n",
            world.getTickIndex(),
            self.getSpeedX(),
            self.getSpeedY());
    }

    const Point2D me{self.getX(), self.getY()};

    //Pre actions
    VISUAL(beginPre());

    visualise_danger_map(m_danger_map, me);

    VISUAL(endPre());

    VISUAL(beginPost());
#ifdef RUNNING_LOCAL
    {
        char buf[20];
        sprintf(buf, "%d", m_bns_top.time);
        VISUAL(text(m_bns_top.pt.x, m_bns_top.pt.y + 50, buf, 0x111199));
        sprintf(buf, "%d", m_bns_bottom.time);
        VISUAL(text(m_bns_bottom.pt.x, m_bns_bottom.pt.y + 50, buf, 0x111199));
    }
#endif

#ifdef RUNNING_LOCAL
    for (const auto &i : m_enemy_towers) {
        char buf[20];
        sprintf(buf, "%d", i.rem_cooldown);
        VISUAL(text(i.x, i.y + 50, buf, 0x00CC00));
    }
#endif
    Behaviour current_action = BH_COUNT;
    static Behaviour prev_action = BH_COUNT;

    Vec2D dir;
    double danger_level = m_danger_map.get_value(me);

    int best_enemy = m_ev->choose_enemy();
    const bool have_target = best_enemy > 0;
    if (danger_level <= config::ATTACK_THRESH && current_action == BH_COUNT) {
        //Danger is ok to attack
        m_pf->get_next_waypoint();
        if (have_target) {
            //Enemy choosen
            prev_action = current_action;
            current_action = BH_ATTACK;
            auto description = m_ev->destroy(move);
            if (self.getDistanceTo(*description.unit) >= description.att_range) {
                m_bhs[BH_ATTACK].update_target(*description.unit);
                if (m_bhs[BH_ATTACK].is_path_spoiled()) {
                    m_bhs[BH_ATTACK].load_path(
                        m_pf->find_way({description.unit->getX(), description.unit->getY()}, description.att_range),
                        *description.unit
                    );
                }
                dir = m_bhs[BH_ATTACK].get_next_direction(self);
                m_pf->move_along(dir, move, true);
            } else {
                if (self.getDistanceTo(*description.unit) <= description.att_range) {
                    current_action = BH_MINIMIZE_DANGER;
                }
            }
        }
    }

    static constexpr int NOT_EXPENSIVE_ENEMY = 1400;
    if (danger_level <= config::BONUS_EARN_THRESH
        && (best_enemy <= NOT_EXPENSIVE_ENEMY)
        && m_pf->bonuses_is_under_control()) {
        //Here check for bonuses time and go to one of them
        bool will_go = false;
        if (m_bhs[BH_EARN_BONUS].is_path_spoiled()) {
            geom::Vec2D shift_top = m_bns_top.pt - me;
            geom::Vec2D shift_bottom = m_bns_bottom.pt - me;
            double my_speed = game.getWizardForwardSpeed();
            const auto &statuses = m_i.s->getStatuses();
            for (const auto &st : statuses) {
                if (st.getType() == model::STATUS_HASTENED) {
                    my_speed *= 1.0 + m_i.g->getHastenedMovementBonusFactor();
                }
            }

            const double max_dist = 1600;
            double td = shift_top.len();
            double tb = shift_bottom.len();

            bool want_bottom = tb <= max_dist && static_cast<int>(tb / my_speed) >= m_bns_bottom.time;
            bool want_top = td <= max_dist && static_cast<int>(td / my_speed) >= m_bns_top.time;

            if (want_bottom && want_top) {
                want_bottom = tb <= td;
                want_top = td <= tb;
            }

            if (want_top) {
                m_bhs[BH_EARN_BONUS].update_target(m_bns_top.pt, game.getBonusRadius() + 5);
                auto way = m_pf->find_way(m_bns_top.pt, game.getBonusRadius() + 5);
                m_bhs[BH_EARN_BONUS].load_path(
                    std::move(way),
                    m_bns_top.pt,
                    game.getBonusRadius() + 5
                );

                will_go = true;
            } else if (want_bottom) {
                m_bhs[BH_EARN_BONUS].update_target(m_bns_bottom.pt, game.getBonusRadius() + 4);
                auto way = m_pf->find_way(m_bns_bottom.pt, game.getBonusRadius() + 4);
                m_bhs[BH_EARN_BONUS].load_path(
                    std::move(way),
                    m_bns_bottom.pt,
                    game.getBonusRadius() + 5
                );

                will_go = true;
            }

        }

        if (!m_bhs[BH_EARN_BONUS].is_path_spoiled()) {
            will_go = true;
            dir = m_bhs[BH_EARN_BONUS].get_next_direction(self);
            m_pf->move_along(dir, move, m_ev->can_shoot_to_target());
        }

        if (will_go) {
            prev_action = current_action;
            current_action = BH_EARN_BONUS;
        }
    }

    if (danger_level <= config::SCOUT_THRESH && current_action == BH_COUNT) {
        //Move by waypoints
        prev_action = current_action;
        current_action = BH_SCOUT;
        Vec2D wp_next = m_pf->get_next_waypoint();
        VISUAL(circle(wp_next.x, wp_next.y, PathFinder::WAYPOINT_RADIUS, 0x001111));
        m_bhs[BH_SCOUT].update_target(wp_next, PathFinder::WAYPOINT_RADIUS);
        if (m_bhs[BH_SCOUT].is_path_spoiled()) {
            auto way = m_pf->find_way(wp_next, PathFinder::WAYPOINT_RADIUS - PathFinder::GRID_SIZE);
            m_bhs[BH_SCOUT].load_path(
                std::move(way),
                wp_next,
                PathFinder::WAYPOINT_RADIUS
            );
        }
        dir = m_bhs[BH_SCOUT].get_next_direction(self);
        VISUAL(line(self.getX(), self.getY(), self.getX() + dir.x, self.getY() + dir.y, 0xFF0000));
        if (dir.len() > EPS) {
            m_pf->move_along(dir, move, false);
        }
    }

    if (current_action == BH_COUNT || current_action == BH_MINIMIZE_DANGER) {
        prev_action = current_action;
        current_action = BH_MINIMIZE_DANGER;


        auto wp_prev = m_pf->get_previous_waypoint();
        m_danger_map.add_field(
            std::make_unique<fields::LinearField>(
                wp_prev, 0, 500, -20
            )
        );

        Point2D from{self.getX(), self.getY()};
        if (m_bhs[BH_MINIMIZE_DANGER].is_path_spoiled() && not_moved) {
            //Possibly stuck in place
            auto way = m_pf->find_way(wp_prev, PathFinder::WAYPOINT_RADIUS - PathFinder::GRID_SIZE);
            m_bhs[BH_MINIMIZE_DANGER].update_target(wp_prev, PathFinder::WAYPOINT_RADIUS);
            m_bhs[BH_MINIMIZE_DANGER].load_path(
                std::move(way),
                wp_prev,
                PathFinder::WAYPOINT_RADIUS
            );
        }

        if (!m_bhs[BH_MINIMIZE_DANGER].is_path_spoiled()) {
            dir = m_bhs[BH_MINIMIZE_DANGER].get_next_direction(self);
        } else {
            dir = damage_avoid_vector(from);
        }
        m_pf->move_along(dir, move, have_target);
    }

    //If we stuck in the tree - destroy it
    if (not_moved) {
        double min_dist = 1e9;
        double min_angle = pi;
        const model::Tree *target = nullptr;
        for (const auto &tree : world.getTrees()) {
            double ang = std::abs(self.getAngleTo(tree));
            double dist = self.getDistanceTo(tree);
            if (dist < min_dist || (eps_equal(dist, min_dist) && ang < min_angle)) {
                min_dist = dist;
                min_angle = ang;
                target = &tree;
            }
        }
        if (target && min_dist <= target->getRadius() + game.getStaffRange()) {
            double ang = self.getAngleTo(*target);
            if (!have_target && (std::abs(ang) >= game.getStaffSector() / 2.0)) {
                move.setTurn(ang);
            }
            if (std::abs(ang) < game.getStaffSector() / 2.0) {
                move.setAction(ACTION_STAFF);
            }
        }
    }

    //Try to destroy enemy, while going to another targets
    if ((current_action == BH_MINIMIZE_DANGER || current_action == BH_EARN_BONUS) && have_target) {
        m_ev->destroy(move);
    }


#ifdef RUNNING_LOCAL
    dir *= 3;
    VISUAL(line(self.getX(), self.getY(), self.getX() + dir.x, self.getY() + dir.y, 0xB325DA));
    char buf[10];
    char c = 'N';
    assert(current_action != BH_COUNT);
    switch (current_action) {
        case BH_MINIMIZE_DANGER:c = 'M';
            break;
        case BH_ATTACK:c = 'A';
            break;
        case BH_SCOUT:c = 'R';
            break;
        case BH_EARN_BONUS: c = 'B';
            break;
    }
    sprintf(buf, "%c %3.1lf%%", c, m_danger_map.get_value(self.getX(), self.getY()));
    VISUAL(text(self.getX() - 70, self.getY() - 60, buf, 0xFF0000));
    //Vec2D spd{self.getSpeedX(), self.getSpeedY()};
    //sprintf(buf, "spd %3.1lf", spd.len());
    //VISUAL(text(self.getX(), self.getY() - 70, buf, 0x009911));
#endif


    VISUAL(endPost());

    if (world.getTickIndex() % 100 == 0) {
        LOG("Try to learn %d\n", move.getSkillToLearn());
    }


}

void MyStrategy::each_tick_update(const model::Wizard &self, const model::World &world, const model::Game &game) {

    //Check about our death
    static int last_tick = 0;
    int cur_tick = world.getTickIndex();
    if (cur_tick - last_tick >= game.getWizardMinResurrectionDelayTicks()) {
        //Seems resurrection
        m_pf = make_unique<PathFinder>(m_i);

    }
    last_tick = cur_tick;

    //Update towers info
    update_shadow_towers(m_enemy_towers, world, self.getFaction());
    //Calculate danger map
    update_danger_map();
    //Update bonuses info
    update_bonuses();

    m_pf->update_info(m_i, m_danger_map);
    m_ev->update_info(m_i);
    m_sb->update_info(m_i);
    for (int i = 0; i < BH_COUNT; ++i) {
        m_bhs[i].update_clock(world.getTickIndex());
    }
}

void MyStrategy::initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game) {
    m_i.s = &self;
    m_i.w = &world;
    m_i.g = &game;

    g_info.s = &self;
    g_info.w = &world;
    g_info.g = &game;
}

bool MyStrategy::initialize_strategy(const model::Wizard &self, const model::World &world, const model::Game &game) {

    m_pf = make_unique<PathFinder>(m_i);
    m_ev = make_unique<Eviscerator>(m_i);
    m_sb = make_unique<SkillBuilder>();

    m_bhs[BH_MINIMIZE_DANGER].PATH_SPOIL_TIME = 100;

    m_bns_top.pt = {1200, 1200};
    m_bns_top.time = 2501;
    m_bns_bottom.pt = {2800, 2800};
    m_bns_bottom.time = m_bns_top.time;

    //Fill enemy towers list, we know what they is mirrored
    m_enemy_towers.clear();
    TowerDesc tds;
    for (const auto &tower : world.getBuildings()) {
        if (tower.getFaction() != self.getFaction()) {
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

        m_enemy_towers.push_back(tds);
    }

    return true;
}

geom::Vec2D MyStrategy::damage_avoid_vector(const Point2D &from) {
    constexpr double ANGLE_STEP = 5;
    constexpr int all_steps = static_cast<int>((360 + ANGLE_STEP - 1) / ANGLE_STEP);
    std::array<double, all_steps + 1> forces;
    forces.fill(1e9);

    Vec2D fwd{m_i.g->getWizardForwardSpeed(), m_i.g->getWizardStrafeSpeed()};
    double len = fwd.len();

    Vec2D back{-m_i.g->getWizardBackwardSpeed(), m_i.g->getWizardStrafeSpeed()};
    double len2 = back.len();


    const int steps = static_cast<int>(360.0 / ANGLE_STEP);
    for (int i = 0; i <= steps; ++i) {
        double angle = deg_to_rad(i * ANGLE_STEP);

        if (angle > pi) {
            len = len2;
        }

        Vec2D cur{len * cos(angle), len * sin(angle)};
        cur.x += from.x;
        cur.y += from.y;
        if (m_pf->check_no_collision(cur, m_i.s->getRadius())) {
            forces[i] = m_danger_map.get_value(cur.x, cur.y);
        }
    }

    const double cur_danger = m_danger_map.get_value(from);
    int min_idx = all_steps;
    forces[all_steps] = cur_danger;
    for (int i = 0; i < forces.size(); ++i) {
        if (forces[i] < forces[min_idx]) {
            min_idx = i;
        }
    }


    if (min_idx == all_steps) {
        //Local minimum found, so best to stay here
        return {0, 0};
    }

    double angle = deg_to_rad(min_idx * ANGLE_STEP);
    Vec2D best{0, 0};
    if (angle > pi) {
        best.x += len2 * cos(angle);
        best.y += len2 * sin(angle);
    } else {
        best.x += len * cos(angle);
        best.y += len * sin(angle);
    }


    best = normalize(best);
    if (angle > pi) {
        best *= len2;
    } else {
        best *= len;
    }
    return best;
}

void MyStrategy::update_shadow_towers(std::list<TowerDesc> &towers,
                                      const model::World &world,
                                      const model::Faction my_faction) {

    /*
     * First decrease cooldown by one
     */
    for (auto &shadow : towers) {
        if (shadow.rem_cooldown > 0) {
            shadow.rem_cooldown--;
        }
    }

    /*
     * Looks for our towers in building list, maybe we see some of them
     */
    std::vector<bool> updated(towers.size(), false);
    for (const auto &enemy_tower : world.getBuildings()) {
        if (enemy_tower.getFaction() == my_faction) {
            continue;
        }
        //Search corresponding tower among shadows
        int idx = 0;
        for (auto &shadow : towers) {
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
    struct VisionRange {
        VisionRange(double x_, double y_, double r_)
            : x(x_),
              y(y_),
              r(r_) {
        }

        double x, y, r;
    };
    std::vector<VisionRange> team_vision;
    for (const auto &creep : world.getMinions()) {
        if (creep.getFaction() != my_faction) {
            continue;
        }
        team_vision.emplace_back(creep.getX(), creep.getY(), creep.getVisionRange());
    }
    for (const auto &wizard : world.getWizards()) {
        if (wizard.getFaction() != my_faction) {
            continue;
        }
        team_vision.emplace_back(wizard.getX(), wizard.getY(), wizard.getVisionRange());
    }
    //Don't check buildings, because it cannot see another building

    Vec2D dist{0, 0};
    for (const auto &vision : team_vision) {
        int idx = 0;
        auto it = towers.cbegin();
        while (it != towers.cend()) {
            dist.x = it->x - vision.x;
            dist.y = it->y - vision.y;
            if (dist.sqr() <= (vision.r * vision.r) && !updated[idx]) {
                //We should see it, but it not appeared in world.getBuildings, so it is destroyed
                LOG("Don't see tower: %3lf <= %3lf, %d\n", dist.len(), vision.r, (int) updated[idx]);
                it = towers.erase(it);
            } else {
                ++it;
            }
            ++idx;
        }
    }
}

void MyStrategy::update_danger_map() {

    m_danger_map.clear();
    auto &damage_fields = m_danger_map;

    const auto &creeps = m_i.w->getMinions();
    const auto &wizards = m_i.w->getWizards();
    const double ME_RETREAT_SPEED = 4.0;

    const auto &add_unit_damage_fields = [&damage_fields](const Point2D &unit_center,
                                                          double attack_range,
                                                          double dead_zone_r) {
        damage_fields.add_field(
            std::make_unique<fields::ExpRingField>(
                unit_center,
                0,
                8000,
                false,
                fields::ExpConfig::from_two_points(config::DAMAGE_ZONE_CURVATURE, config::DAMAGE_MAX_FEAR, dead_zone_r)
            )
        );
    };

    const RunawayUnit me{m_i.s->getLife(), m_i.s->getMaxLife(), ME_RETREAT_SPEED};
    for (const auto &minion : creeps) {

        if (minion.getFaction() == FACTION_NEUTRAL || minion.getFaction() == m_i.s->getFaction()) {
            continue;
        }

        double attack_range;
        if (minion.getType() == MINION_ORC_WOODCUTTER) {
            attack_range = m_i.g->getOrcWoodcutterAttackRange();
        } else {
            attack_range = m_i.g->getFetishBlowdartAttackRange();
        }
        attack_range += m_i.s->getRadius();

        const AttackUnit enemy{minion.getRemainingActionCooldownTicks(),
                               minion.getCooldownTicks(),
                               minion.getDamage(),
                               attack_range,
                               m_i.g->getMinionSpeed()};
        double dead_zone_r = Eviscerator::calc_dead_zone(me, enemy);

        if (minion.getType() == MINION_ORC_WOODCUTTER) {
            damage_fields.add_field(
                std::make_unique<fields::CustomField>(
                    Point2D{minion.getX(), minion.getY()},
                    0,
                    minion.getRadius() + m_i.g->getStaffRange(),
                    [](double x) -> double {
                        return 70;
                    }
                )
            );
        } else {
            damage_fields.add_field(
                std::make_unique<fields::CustomField>(
                    Point2D{minion.getX(), minion.getY()},
                    0,
                    m_i.g->getFetishBlowdartAttackRange() + m_i.s->getRadius(),
                    [](double x) -> double {
                        return 20;
                    }
                )
            );
        }
    }
    for (const auto &wizard : wizards) {
        if (wizard.getFaction() == FACTION_NEUTRAL || wizard.getFaction() == m_i.s->getFaction()) {
            continue;
        }

        const double attack_range = wizard.getCastRange() + m_i.s->getRadius() + m_i.g->getMagicMissileRadius();
        const AttackUnit enemy{wizard.getRemainingActionCooldownTicks(),
                               m_i.g->getMagicMissileCooldownTicks(),
                               m_i.g->getMagicMissileDirectDamage(),
                               attack_range,
                               m_i.g->getWizardForwardSpeed()};
        double dead_zone_r = Eviscerator::calc_dead_zone(me, enemy);
        add_unit_damage_fields({wizard.getX(), wizard.getY()}, attack_range, dead_zone_r);
    }

    for (const auto &tower : m_enemy_towers) {
        double attack_range = tower.attack_range + m_i.s->getRadius();
        const AttackUnit enemy{tower.rem_cooldown,
                               tower.cooldown,
                               tower.damage,
                               attack_range,
                               0};
        double dead_zone_r = Eviscerator::calc_dead_zone(me, enemy);
        bool under_attack = m_ev->tower_maybe_attack_me(tower);
        double cost = config::DAMAGE_MAX_FEAR;
        if (under_attack) {
            cost *= 50;
        }
        damage_fields.add_field(
            std::make_unique<fields::ExpRingField>(
                Point2D{tower.x, tower.y},
                0,
                tower.attack_range + 5,
                false,
                fields::ExpConfig::from_two_points(config::DAMAGE_ZONE_CURVATURE, cost, dead_zone_r)
            )
        );

        VISUAL(circle(tower.x, tower.y, tower.attack_range, 0x440000));
        char buf[10];
        sprintf(buf, "%3d", tower.rem_cooldown);
        VISUAL(text(tower.x, tower.y + 30, buf, 0x004400));
    }
}

void MyStrategy::visualise_danger_map(const fields::FieldMap &danger, const geom::Point2D &center) {
#ifdef RUNNING_LOCAL
    const int GRID_SIZE = 30;
    const int half = GRID_SIZE / 2;
    Vec2D tmp;
    for (int x = -600; x <= 600; x += GRID_SIZE) {
        for (int y = -600; y <= 600; y += GRID_SIZE) {
            double x_a = center.x + x;
            double y_a = center.y + y;
            double force = danger.get_value(x_a, y_a);
            if (force > config::DAMAGE_MAX_FEAR) {
                force = config::DAMAGE_MAX_FEAR;
            }
            if (force < -config::DAMAGE_MAX_FEAR) {
                force = -config::DAMAGE_MAX_FEAR;
            }

            int color = static_cast<uint8_t>((255.0 / config::DAMAGE_MAX_FEAR) * std::abs(force));
            color = 255 - color;
            if (force < 0) {
                color = (color << 16) | 0xff00 | color;
            } else {
                color = (color << 16) | (color << 8) | color;
            }


            VISUAL(fillRect(x_a - half - 1, y_a - half - 1, x_a + half, y_a + half, color));
        }
    }
#endif
}

void MyStrategy::update_bonuses() {

    auto my_faction = m_i.s->getFaction();
    struct VisionRange {
        VisionRange(double x_, double y_, double r_)
            : x(x_),
              y(y_),
              r(r_) {
        }

        double x, y, r;
    };
    std::vector<VisionRange> team_vision;
    for (const auto &creep : m_i.w->getMinions()) {
        if (creep.getFaction() != my_faction) {
            continue;
        }
        team_vision.emplace_back(creep.getX(), creep.getY(), creep.getVisionRange());
    }
    for (const auto &wizard : m_i.w->getWizards()) {
        if (wizard.getFaction() != my_faction) {
            continue;
        }
        team_vision.emplace_back(wizard.getX(), wizard.getY(), wizard.getVisionRange());
    }

    //Check if bonuses in vision range and exists
    bool bottom_up = false;
    bool top_up = false;
    for (const auto &bns : m_i.w->getBonuses()) {
        if (bns.getDistanceTo(m_bns_top.pt.x, m_bns_top.pt.y) <= m_i.g->getBonusRadius()) {
            m_bns_top.time = 0; //Already appeared
            top_up = true;
        }
        if (bns.getDistanceTo(m_bns_bottom.pt.x, m_bns_bottom.pt.y) <= m_i.g->getBonusRadius()) {
            m_bns_bottom.time = 0; //Already appeared
            bottom_up = true;
        }
    }

    geom::Vec2D ally;
    geom::Vec2D dist;
    int bonus_interval = m_i.g->getBonusAppearanceIntervalTicks();
    for (const auto &ally_vis : team_vision) {
        ally = {ally_vis.x, ally_vis.y};
        dist = ally - m_bns_top.pt;
        if (dist.sqr() <= (ally_vis.r * ally_vis.r) && !top_up && m_bns_top.time == 0) {
            //In vision but not updated, so it was taken
            int next_tick = ((m_i.w->getTickIndex() + bonus_interval - 1) / bonus_interval) * bonus_interval;
            m_bns_top.time = next_tick - m_i.w->getTickIndex() + 2;
            LOG("Top bonus was taken!\n");
        }

        dist = ally - m_bns_bottom.pt;
        if (dist.sqr() <= (ally_vis.r * ally_vis.r) && !bottom_up && m_bns_bottom.time == 0) {
            //In vision but not updated, so it was taken
            int next_tick = ((m_i.w->getTickIndex() + bonus_interval - 1) / bonus_interval) * bonus_interval;
            m_bns_bottom.time = next_tick - m_i.w->getTickIndex() + 2;
            LOG("Bottom bonus was taken! next time = %d\n", m_bns_bottom.time);
        }
    }

    //Decrease remaining time
    if (m_bns_bottom.time > 0) {
        --m_bns_bottom.time;
    }
    if (m_bns_top.time > 0) {
        --m_bns_top.time;
    }
}
