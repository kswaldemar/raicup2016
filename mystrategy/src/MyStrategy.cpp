#include "FieldsDescription.h"
#include "Config.h"
#include "MyStrategy.h"
#include "PathFinder.h"
#include "Logger.h"
#include "VisualDebug.h"

#include <cassert>
#include <list>
#include <algorithm>

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
    const bool not_moved = hold_time >= 7;
    //if (not_moved) {
    //    LOG("Tick %d: Not moved in last tick - spd (%3.1lf, %3.1lf)\n",
    //        world.getTickIndex(),
    //        self.getSpeedX(),
    //        self.getSpeedY());
    //}

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
    for (const auto &i : m_i.ew->get_hostile_towers()) {
        char buf[20];
        sprintf(buf, "%d", i.rem_cooldown);
        VISUAL(text(i.x, i.y + 50, buf, 0x00CC00));
    }
#endif
    Behaviour current_action = BH_COUNT;
    static Behaviour prev_action = BH_COUNT;

    Vec2D dir;
    double danger_level = m_danger_map.get_value(me);

    fields::FieldMap navigation(fields::FieldMap::MIN);
    const double NAV_POTENTIAL = -25;
    int nav_radius = PathFinder::GRID_SIZE * 2;

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
                        m_pf->find_way({description.unit->getX(), description.unit->getY()}, description.att_range - PathFinder::GRID_SIZE),
                        *description.unit
                    );
                }
                dir = m_bhs[BH_ATTACK].get_next_direction(self);
                navigation.add_field(std::make_unique<fields::LinearField>(
                    me + dir, 0, nav_radius, NAV_POTENTIAL
                ));
            } else {
                if (self.getDistanceTo(*description.unit) <= description.att_range) {
                    current_action = BH_MINIMIZE_DANGER;
                }
            }
        }
    }

    static constexpr int NOT_EXPENSIVE_ENEMY = 1600;
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
            double td = shift_top.len() - game.getBonusRadius();
            double tb = shift_bottom.len() - game.getBonusRadius();

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
            navigation.clear();
            navigation.add_field(std::make_unique<fields::LinearField>(
                me + dir, 0, nav_radius, NAV_POTENTIAL - 3
            ));
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
        navigation.clear();
        navigation.add_field(std::make_unique<fields::LinearField>(
            me + dir, 0, nav_radius, NAV_POTENTIAL
        ));

        VISUAL(line(self.getX(), self.getY(), self.getX() + dir.x, self.getY() + dir.y, 0xFF0000));
    }

    if (current_action == BH_COUNT || current_action == BH_MINIMIZE_DANGER) {
        prev_action = current_action;
        current_action = BH_MINIMIZE_DANGER;


        auto wp_prev = m_pf->get_previous_waypoint();
        VISUAL(circle(wp_prev.x, wp_prev.y, PathFinder::WAYPOINT_RADIUS, 0x002211));
        m_danger_map.add_field(
            std::make_unique<fields::LinearField>(
                wp_prev, 0, 500, -13
            )
        );

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
            navigation.clear();
            navigation.add_field(std::make_unique<fields::LinearField>(
                me + dir, 0, nav_radius, NAV_POTENTIAL - 5
            ));
        }
    }

    dir = potential_vector(me, {&m_danger_map, &navigation});
    if (dir.len() > EPS) {
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

    if (!m_i.ew->check_no_collision(me + dir, self.getRadius())) {
        auto dest = me + dir;
        LOG("ERROR: Check no collision failed. Destination (%3.1lf, %3.1lf) will lead to collision. "
                "Vector (%3.1lf, %3.1lf); Me (%3.1lf, %3.1lf); Angle %3.5lf\n",
            dest.x,
            dest.y,
            dir.x,
            dir.y,
            me.x,
            me.y,
            self.getAngle());
    }

    //Try to destroy enemy, while going to another targets
    if ((current_action == BH_MINIMIZE_DANGER || current_action == BH_EARN_BONUS) && have_target) {
        m_ev->destroy(move);
    }

    //TODO: Rewrite it
    const auto &skills = self.getSkills();
    const auto &statuses = self.getStatuses();
    const bool has_shield = std::find(skills.cbegin(), skills.cend(), model::SKILL_SHIELD) != skills.cend();
    const bool has_haste = std::find(skills.cbegin(), skills.cend(), model::SKILL_HASTE) != skills.cend();
    bool shielded = false;
    bool hastened = false;
    for (const auto &st : statuses) {
        shielded = shielded || st.getType() == STATUS_SHIELDED;
        hastened = hastened || st.getType() == STATUS_HASTENED;
    }

    if (has_shield && !shielded && self.getMana() >= game.getShieldManacost()) {
        move.setAction(ACTION_SHIELD);
    } else if (has_haste && !hastened && self.getMana() >= game.getHasteManacost()) {
        move.setAction(ACTION_HASTE);
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

void MyStrategy::initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game) {
    m_i.s = &self;
    m_i.w = &world;
    m_i.g = &game;
}

bool MyStrategy::initialize_strategy(const model::Wizard &self, const model::World &world, const model::Game &game) {

    m_pf = make_unique<PathFinder>(m_i);
    m_ev = make_unique<Eviscerator>(m_i);
    m_sb = make_unique<SkillBuilder>();

    m_i.ew = make_unique<ExWorld>(world, game);

    m_bhs[BH_MINIMIZE_DANGER].PATH_SPOIL_TIME = 100;

    m_bns_top.pt = {1200, 1200};
    m_bns_top.time = 2501;
    m_bns_bottom.pt = {2800, 2800};
    m_bns_bottom.time = m_bns_top.time;

    return true;
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

    m_i.ew->update_world(world, game);

    //Calculate danger map
    update_danger_map();
    //Update bonuses info
    //TODO: Move to ExWorld
    update_bonuses();

    m_pf->update_info(m_i, m_danger_map);
    m_ev->update_info(m_i);
    m_sb->update_info(m_i);
    for (int i = 0; i < BH_COUNT; ++i) {
        m_bhs[i].update_clock(world.getTickIndex());
    }
}

geom::Vec2D MyStrategy::potential_vector(const Point2D &pt, const std::vector<const fields::FieldMap*> &fmaps) const {
    constexpr double ANGLE_STEP = 1;
    constexpr int all_steps = static_cast<int>((360 + ANGLE_STEP) / ANGLE_STEP);
    std::array<double, all_steps + 1> forces;
    forces.fill(0);

    static std::array<double, all_steps> sin_table;
    static std::array<double, all_steps> cos_table;
    static std::array<double, all_steps> rad;
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        for (int i = 0; i < all_steps; ++i) {
            rad[i] = deg_to_rad(i * ANGLE_STEP);
            sin_table[i] = sin(rad[i]);
            cos_table[i] = cos(rad[i]);
        }
    }

    const double factor = m_i.ew->get_wizard_movement_factor(*m_i.s);

    const double strafe = m_i.g->getWizardStrafeSpeed() * factor;
    const double fwd = m_i.g->getWizardForwardSpeed() * factor;
    const double back = m_i.g->getWizardBackwardSpeed() * factor;


    const int steps = static_cast<int>(360.0 / ANGLE_STEP);
    double b = fwd;
    double a = strafe;

    const double my_angle = m_i.s->getAngle() + pi / 2;
    const double turn_cos = cos(my_angle);
    const double turn_sin = sin(my_angle);

    for (int i = 0; i <= steps; ++i) {
        double angle = rad[i];
        if (angle > pi) {
            b = back;
        }

        Vec2D cur{a * cos_table[i], -b * sin_table[i]};

        //Rotate
        double x1 = cur.x * turn_cos - cur.y * turn_sin;
        double y1 = cur.y * turn_cos + cur.x * turn_sin;

        cur.x = x1 + pt.x;
        cur.y = y1 + pt.y;

        if (m_i.ew->check_no_collision(cur, m_i.s->getRadius())) {
            for (const auto &fmap : fmaps) {
                forces[i] += fmap->get_value(cur);
            }
        } else {
            forces[i] = 1e5;
        }
    }
    for (const auto &fmap : fmaps) {
        forces[all_steps] += fmap->get_value(pt);
    }
    int min_idx = all_steps;
    for (int i = 0; i < forces.size(); ++i) {
        if (forces[i] < forces[min_idx]) {
            min_idx = i;
        }
    }

    if (min_idx == all_steps) {
        //Local minimum found, so best to stay here
        return {0, 0};
    }

    double angle = rad[min_idx];
    if (angle > pi) {
        b = back;
    } else {
        b = fwd;
    }
    double x = a * cos_table[min_idx];
    double y = -b * sin_table[min_idx];

    double x1 = x * turn_cos - y * turn_sin;
    double y1 = y * turn_cos + x * turn_sin;

    return {x1, y1};
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
            damage_fields.add_field(std::make_unique<fields::ConstField>(
                Point2D{minion.getX(), minion.getY()},
                0, minion.getRadius() + m_i.g->getStaffRange(),
                70
            ));
        } else {
            damage_fields.add_field(std::make_unique<fields::ConstField>(
                Point2D{minion.getX(), minion.getY()},
                0, m_i.g->getFetishBlowdartAttackRange() + m_i.s->getRadius(),
                10
            ));
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

    for (const auto &tower : m_i.ew->get_hostile_towers()) {
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
                tower.attack_range + 1,
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

    int bonus_interval = m_i.g->getBonusAppearanceIntervalTicks();
    if (m_i.ew->check_in_team_vision(m_bns_top.pt) && !top_up && m_bns_top.time == 0) {
        //In vision but not updated, so it was taken
        int next_tick = ((m_i.w->getTickIndex() + bonus_interval - 1) / bonus_interval) * bonus_interval;
        m_bns_top.time = next_tick - m_i.w->getTickIndex() + 2;
        LOG("Top bonus was taken!\n");
    }

    if (m_i.ew->check_in_team_vision(m_bns_bottom.pt) && !bottom_up && m_bns_bottom.time == 0) {
        //In vision but not updated, so it was taken
        int next_tick = ((m_i.w->getTickIndex() + bonus_interval - 1) / bonus_interval) * bonus_interval;
        m_bns_bottom.time = next_tick - m_i.w->getTickIndex() + 2;
        LOG("Bottom bonus was taken! next time = %d\n", m_bns_bottom.time);
    }

    //Decrease remaining time
    if (m_bns_bottom.time > 0) {
        --m_bns_bottom.time;
    }
    if (m_bns_top.time > 0) {
        --m_bns_top.time;
    }
}
