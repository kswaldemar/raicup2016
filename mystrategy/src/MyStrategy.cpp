#include "FieldsDescription.h"
#include "MyStrategy.h"
#include "PathFinder.h"
#include "Logger.h"
#include "VisualDebug.h"
#include "BehaviourConfig.h"

#include <cassert>
#include <list>
#include <algorithm>

using namespace model;
using namespace std;
using namespace geom;

BehaviourConfig::Damage BehaviourConfig::damage = BehaviourConfig::Damage();
BehaviourConfig::NavK BehaviourConfig::navigation_k = BehaviourConfig::NavK();
BehaviourConfig::BulletF BehaviourConfig::bullet = BehaviourConfig::BulletF();

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

    fields::FieldMap strategic_map = create_danger_map();

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

    const Point2D me{self.getX(), self.getY()};

    //Pre actions
    VISUAL(beginPre());

    Behaviour current_action = BH_COUNT;
    static Behaviour prev_action = BH_COUNT;

    Vec2D dir;
    double danger_level = strategic_map.get_value(me);

    fields::FieldMap navigation(fields::FieldMap::MIN);
    int nav_radius = 2000;

    double best_enemy = m_ev->choose_enemy();
    const bool have_target = best_enemy > 0;
    const bool can_shoot = m_ev->can_shoot_to_target();
    bool hold_face = have_target;
    if (danger_level <= BehaviourConfig::danger_attack_t && current_action == BH_COUNT && have_target) {
        //Danger is ok to attack
        //Enemy choosen
        prev_action = current_action;
        current_action = BH_ATTACK;
        auto description = m_ev->destroy(move);
        if (self.getDistanceTo(*description.unit) >= description.att_range) {
            m_bhs[BH_ATTACK].update_target(*description.unit);
            if (m_bhs[BH_ATTACK].is_path_spoiled()) {
                auto way = m_pf->find_way({description.unit->getX(), description.unit->getY()},
                                          description.att_range - PathFinder::GRID_SIZE);
                smooth_path(me, way);
                m_bhs[BH_ATTACK].load_path(
                    std::move(way),
                    *description.unit
                );
            }
            dir = m_bhs[BH_ATTACK].get_next_direction(self);
            navigation.add_field(std::make_unique<fields::LinearField>(
                me + dir,
                0,
                nav_radius,
                BehaviourConfig::navigation_k.attack,
                -BehaviourConfig::navigation_k.attack * nav_radius
            ));
        } else if (self.getDistanceTo(*description.unit) <= description.att_range) {
            current_action = BH_MINIMIZE_DANGER;
        }

    }

    if (danger_level <= BehaviourConfig::danger_bonus_earn_t
        && (m_ev->can_leave_battlefield())
        && m_pf->bonuses_is_under_control()) {
        //Here check for bonuses time and go to one of them
        bool will_go = false;
        if (m_bhs[BH_EARN_BONUS].is_path_spoiled()) {
            const double my_speed = game.getWizardForwardSpeed() * m_i.ew->get_wizard_movement_factor(self);
            const double max_dist = 1600;
            const int bonus_overtime = 100;
            const MyBonus *best = nullptr;
            double min_dist = 1e5;
            for (const auto &b : m_i.ew->get_bonuses()) {
                const double dist = self.getDistanceTo(b.getX(), b.getY()) - game.getBonusRadius();
                if (dist >= max_dist || dist >= min_dist) {
                    continue;
                }
                int reach_time = static_cast<int>(ceil(dist / my_speed));
                int diff = b.getRemainingTime() - reach_time;
                if (diff <= bonus_overtime) {
                    best = &b;
                    min_dist = dist;
                }
            }

            if (best != nullptr) {
                m_bhs[BH_EARN_BONUS].update_target(best->getPoint(), game.getBonusRadius() + 5);
                auto way = m_pf->find_way(best->getPoint(), game.getBonusRadius() + 5);
                smooth_path(me, way);
                m_bhs[BH_EARN_BONUS].load_path(
                    std::move(way),
                    best->getPoint(),
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
                me + dir,
                0,
                nav_radius,
                BehaviourConfig::navigation_k.bonus,
                -BehaviourConfig::navigation_k.bonus * nav_radius
            ));
        }

        if (will_go) {
            prev_action = current_action;
            current_action = BH_EARN_BONUS;
            hold_face = can_shoot;
        }
    }

    //Prevent bonus denying
    for (const auto &b : m_i.ew->get_bonuses()) {
        int rem_time = b.getRemainingTime();
        if (rem_time > 0 && rem_time < 26) {
            const double d1 = game.getBonusRadius() + self.getRadius();
            m_damage_map.add_field(std::make_unique<fields::LinearField>(
                b.getPoint(), 0, d1, -0.1 / d1, 1
            ));
        }
    }

    geom::Point2D waypoint{0, 0};

    if (danger_level <= BehaviourConfig::danger_scout_t && current_action == BH_COUNT) {
        //Move by waypoints
        prev_action = current_action;
        current_action = BH_SCOUT;
        Vec2D wp_next = m_pf->get_next_waypoint();
        VISUAL(circle(wp_next.x, wp_next.y, PathFinder::WAYPOINT_RADIUS, 0x001111));
        m_bhs[BH_SCOUT].update_target(wp_next, PathFinder::WAYPOINT_RADIUS);
        if (m_bhs[BH_SCOUT].is_path_spoiled()) {
            auto way = m_pf->find_way(wp_next, PathFinder::WAYPOINT_RADIUS - PathFinder::GRID_SIZE);
            smooth_path(me, way);
            m_bhs[BH_SCOUT].load_path(
                std::move(way),
                wp_next,
                PathFinder::WAYPOINT_RADIUS
            );
        }
        dir = m_bhs[BH_SCOUT].get_next_direction(self);
        navigation.clear();
        navigation.add_field(std::make_unique<fields::LinearField>(
            me + dir,
            0,
            nav_radius,
            BehaviourConfig::navigation_k.next_wp,
            -BehaviourConfig::navigation_k.next_wp * nav_radius
        ));

        waypoint = me + dir;

        VISUAL(line(self.getX(), self.getY(), self.getX() + dir.x, self.getY() + dir.y, 0xFF0000));
    }

    if (current_action == BH_COUNT || current_action == BH_MINIMIZE_DANGER) {
        prev_action = current_action;
        current_action = BH_MINIMIZE_DANGER;


        auto wp_prev = m_pf->get_previous_waypoint();
        if (wp_prev.x > 0 && wp_prev.y > 0) {
            VISUAL(circle(wp_prev.x, wp_prev.y, PathFinder::WAYPOINT_RADIUS, 0x002211));

            navigation.add_field(
                std::make_unique<fields::LinearField>(
                    wp_prev,
                    0,
                    nav_radius,
                    BehaviourConfig::navigation_k.prev_wp,
                    -BehaviourConfig::navigation_k.prev_wp * 500
                )
            );

            if (m_bhs[BH_MINIMIZE_DANGER].is_path_spoiled() && not_moved) {
                //Possibly stuck in place
                auto way = m_pf->find_way(wp_prev, PathFinder::WAYPOINT_RADIUS - PathFinder::GRID_SIZE);
                smooth_path(me, way);
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
                    me + dir,
                    0,
                    nav_radius,
                    BehaviourConfig::navigation_k.prev_wp,
                    -BehaviourConfig::navigation_k.prev_wp * nav_radius
                ));
            }
        }
    }


    const auto &bullets_map = m_ev->get_bullet_damage_map();

    dir = potential_vector(me, {&m_damage_map, &navigation, &bullets_map});
    if (dir.len() > EPS) {
        m_pf->move_along(dir, move, hold_face);
    }

    //Check tree blocking way
    const MyLivingUnit *block_obstacle;
    bool destroy_obstacle = false;
    if ((waypoint.x > 0 && waypoint.y > 0
         && !m_i.ew->check_no_collision(waypoint, self.getRadius(), &block_obstacle)) // Obstacle blocking way
        || (!m_i.ew->check_no_collision(me, self.getRadius() + 4.5, &block_obstacle)
            && dir.len() <= EPS) // Local minimum because of obstacle
        ) {
        //We will meet collision soon
        VISUAL(fillCircle(waypoint.x, waypoint.y, 10, 0x990000));
        //TODO: Choose nearest obstacle
        if (block_obstacle && block_obstacle->getType() == MyLivingUnit::STATIC) {
            m_ev->destroy(move, {block_obstacle->getX(), block_obstacle->getY()}, block_obstacle->getRadius());
            destroy_obstacle = true;
        }
    }
    if ((current_action == BH_MINIMIZE_DANGER || current_action == BH_EARN_BONUS) && can_shoot && !destroy_obstacle) {
        //Shoot to enemy, when retreating
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

    visualise_field_maps(
        {
            //&m_damage_map,
            //&navigation,
            &bullets_map,
        },
        me
    );

    VISUAL(endPre());

    VISUAL(beginPost());

    //m_ev->m_bhandler.visualise_projectiles();
    ////
    //m_ev->m_bhandler.check_projectiles_hit(me, self.getRadius());

    //for (const auto &i : m_i.ew->get_obstacles()) {
    //    VISUAL(fillCircle(i.getX(), i.getY(), 5, 0x550011));
    //}

#ifdef RUNNING_LOCAL
    {
        char buf[20];
        auto bonuses = m_i.ew->get_bonuses();
        sprintf(buf, "%d", bonuses[0].getRemainingTime());
        VISUAL(text(bonuses[0].getX(), bonuses[0].getY() + 50, buf, 0x111199));
        sprintf(buf, "%d", bonuses[1].getRemainingTime());
        VISUAL(text(bonuses[1].getX(), bonuses[1].getY() + 50, buf, 0x111199));
    }
#endif

#ifdef RUNNING_LOCAL
    for (const auto &i : m_i.ew->get_hostile_towers()) {
        char buf[20];
        sprintf(buf, "%d", i.getRemainingActionCooldownTicks());
        VISUAL(text(i.getX(), i.getY() + 50, buf, 0x00CC00));
    }
#endif

//
//#ifdef RUNNING_LOCAL
//    dir *= 3;
//    VISUAL(line(self.getX(), self.getY(), self.getX() + dir.x, self.getY() + dir.y, 0xB325DA));
//    char buf[10];
//    char c = 'N';
//    assert(current_action != BH_COUNT);
//    switch (current_action) {
//        case BH_MINIMIZE_DANGER:c = 'M';
//            break;
//        case BH_ATTACK:c = 'A';
//            break;
//        case BH_SCOUT:c = 'R';
//            break;
//        case BH_EARN_BONUS: c = 'B';
//            break;
//    }
//    sprintf(buf, "%c %3.1lf%%", c, m_damage_map.get_value(self.getX(), self.getY()));
//    VISUAL(text(self.getX() - 70, self.getY() - 60, buf, 0xFF0000));
//    sprintf(buf, "[%5.5lf]", strategic_map.get_value(me));
//    VISUAL(text(me.x - 70, me.y - 80, buf, 0xFF9900));
//    //Vec2D spd{self.getSpeedX(), self.getSpeedY()};
//    //sprintf(buf, "spd %3.1lf", spd.len());
//    //VISUAL(text(self.getX(), self.getY() - 70, buf, 0x009911));
//#endif


    VISUAL(endPost());

    //if (world.getTickIndex() % 100 == 0) {
    //    LOG("Try to learn %d\n", move.getSkillToLearn());
    //}
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

    m_i.ew->update_world(self, world, game);

    //Calculate danger map
    update_damage_map();

    m_pf->update_info(m_i, m_damage_map);
    m_ev->update_info(m_i);
    m_sb->update_info(m_i);
    for (int i = 0; i < BH_COUNT; ++i) {
        m_bhs[i].update_clock(world.getTickIndex());
    }
}

geom::Vec2D MyStrategy::potential_vector(const Point2D &pt, const std::vector<const fields::FieldMap *> &fmaps) const {
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

void MyStrategy::update_damage_map() {

    m_damage_map.clear();
    auto &damage_fields = m_damage_map;

    const double my_speed = m_i.g->getWizardBackwardSpeed() * m_i.ew->get_wizard_movement_factor(*m_i.s);
    const double my_radius = m_i.s->getRadius();

    const double life_factor = m_i.s->getLife() / m_i.s->getMaxLife();
    const double klife = 1.0 + life_factor;

    for (const auto &i : m_i.ew->get_hostile_creeps()) {
        const Point2D unit_center{i->getX(), i->getY()};
        double at_range;
        double force;
        if (i->getType() == MINION_FETISH_BLOWDART) {
            at_range = m_i.g->getFetishBlowdartAttackRange();
            force = BehaviourConfig::damage.fetish * klife;
        } else {
            at_range = m_i.g->getOrcWoodcutterAttackRange();
            force = BehaviourConfig::damage.orc * klife;
        }
        at_range += my_radius;

        const double narrow_attack_range = m_ev->get_minion_aggression_radius(*i);
        if (narrow_attack_range < at_range) {
            at_range = narrow_attack_range;
        }

        double d1 = at_range - i->getRemainingActionCooldownTicks() * my_speed;

        if (d1 >= 1) {
            damage_fields.add_field(std::make_unique<fields::LinearField>(
                unit_center, 0, d1, -force / d1, BehaviourConfig::damage.center_mult * force
            ));
        } else {
            d1 = 0;
        }



        if (!eps_equal(d1, at_range)) {
            damage_fields.add_field(std::make_unique<fields::LinearField>(
                unit_center, d1, at_range,
                -force / (at_range - d1),
                force + (force / (at_range - d1)) * d1
            ));
        }
    }

    for (const auto &i : m_i.ew->get_hostile_wizards()) {
        const Point2D unit_center{i->getX(), i->getY()};
        const double at_range = i->getCastRange() + my_radius;
        //TODO: Respect mana
        //TODO: Respect skills (fireball, frostbolt, ultimate magic missile)
        //TODO: Respect haste increased turn speed
        int rem_cooldown = i->getRemainingActionCooldownTicks();
        const double angle_to_me = std::abs(i->getAngleTo(*m_i.s));
        rem_cooldown += static_cast<int>(ceil(angle_to_me / m_i.g->getWizardMaxTurnAngle()));

        double d1 = at_range - rem_cooldown * my_speed;
        const double force = BehaviourConfig::damage.wizard * klife;
        if (d1 >= 1) {
            damage_fields.add_field(std::make_unique<fields::LinearField>(
                unit_center, 0, d1, -force / d1, BehaviourConfig::damage.center_mult * force
            ));
        } else {
            d1 = 0;
        }

        if (!eps_equal(d1, at_range)) {
            damage_fields.add_field(std::make_unique<fields::LinearField>(
                unit_center, d1, at_range,
                -force / (at_range - d1),
                force + (force / (at_range - d1)) * d1
            ));
        }
    }


    for (const auto &i : m_i.ew->get_hostile_towers()) {
        const Point2D unit_center{i.getX(), i.getY()};
        const double at_range = i.getAttackRange() + 1;

        double force;
        if (m_ev->tower_maybe_attack_me(i)) {
            force = BehaviourConfig::damage.tower_active;
        } else {
            force = BehaviourConfig::damage.tower;
        }
        force *= klife;

        const double retr_diff = i.getRemainingActionCooldownTicks() * my_speed;
        double d1 = at_range - retr_diff;

        if (d1 >= 1) {
            damage_fields.add_field(std::make_unique<fields::LinearField>(
                unit_center, 0, d1, -force / d1, BehaviourConfig::damage.center_mult * force
            ));
        } else {
            d1 = 0;
        }

        damage_fields.add_field(std::make_unique<fields::LinearField>(
            unit_center, d1, at_range,
            -force / retr_diff,
            force + (force / retr_diff) * d1
        ));


#ifdef RUNNING_LOCAL
        {
            char buf[10];
            sprintf(buf, "%3d", i.getRemainingActionCooldownTicks());
            VISUAL(text(i.getX(), i.getY() + 30, buf, 0x004400));
        }
#endif
    }


}

void MyStrategy::visualise_field_maps(const std::vector<const fields::FieldMap *> &fmaps, const geom::Point2D &center) {
#ifdef RUNNING_LOCAL
    const int GRID_SIZE = 30;
    const int half = GRID_SIZE / 2;
    Vec2D tmp;
    for (int x = -600; x <= 600; x += GRID_SIZE) {
        for (int y = -600; y <= 600; y += GRID_SIZE) {
            double x_a = center.x + x;
            double y_a = center.y + y;
            double force = 0;
            for (const auto &fmap : fmaps) {
                force += fmap->get_value(x_a, y_a);
            }

            force *= 255;

            force = std::min(force, 255.0);
            force = std::max(force, -255.0);

            if (eps_equal(force, 0)) {
                continue;
            }

            int color = static_cast<uint8_t>(std::abs(force));
            color = 255 - color;
            if (force < 0) {
                color = (color << 16) | 0xff00 | color;
            } else {
                //color = (color << 16) | (color << 8) | color;
                color = (color << 16) | (color << 8) | 0xff;
            }


            VISUAL(fillRect(x_a - half - 1, y_a - half - 1, x_a + half, y_a + half, color));
        }
    }
#endif
}

void MyStrategy::smooth_path(const geom::Point2D &me, std::list<geom::Point2D> &path) {
    auto almost_last = path.cend();
    --almost_last;
    for (auto it = path.cbegin(); it != almost_last;) {
        if (m_i.ew->line_of_sight(me.x, me.y, it->x, it->y)) {
            it = path.erase(it);
        } else {
            break;
        }
    }
}

fields::FieldMap MyStrategy::create_danger_map() {
    using namespace fields;
    FieldMap ret(FieldMap::ADD);

    const double my_speed = m_i.g->getWizardBackwardSpeed() * m_i.ew->get_wizard_movement_factor(*m_i.s);
    const int my_life = m_i.s->getLife();
    const double my_max_life = m_i.s->getMaxLife();
    //TODO: Respect self shield

    for (const auto &i : m_i.ew->get_hostile_towers()) {
        DangerField::Config conf = {
            i.getDamage(),
            i.getAttackRange(),
            i.getRemainingActionCooldownTicks(),
            i.getCooldown(),
            0 // Tower cannot move
        };

        ret.add_field(std::make_unique<DangerField>(
            i.getPoint(),
            0,
            i.getAttackRange() + 1,
            my_speed,
            my_life,
            my_max_life,
            conf
        ));
    }

    for (const auto &i : m_i.ew->get_hostile_creeps()) {
        const double attack_range = i->getType() == MINION_ORC_WOODCUTTER ? m_i.g->getOrcWoodcutterAttackRange()
                                                                          : m_i.g->getFetishBlowdartAttackRange();
        //TODO: Respect minion angle as increased remaining cooldown
        DangerField::Config conf = {
            i->getDamage(),
            attack_range + m_i.s->getRadius(),
            i->getRemainingActionCooldownTicks(),
            i->getCooldownTicks(),
            m_i.g->getMinionSpeed()
        };

        //TODO: Check that minion will attack us and narrow field if not
        //TODO: Adjust to emulate going away
        ret.add_field(std::make_unique<DangerField>(
            geom::Point2D(i->getX(), i->getY()),
            0,
            i->getVisionRange(),
            my_speed,
            my_life,
            my_max_life,
            conf
        ));
    }

    for (const auto &i : m_i.ew->get_hostile_wizards()) {

        const auto cooldowns = i->getRemainingCooldownTicksByAction();
        const double speed = m_i.g->getWizardForwardSpeed() * m_i.ew->get_wizard_movement_factor(*i);

        int overtime = 0;
        const double angle = std::abs(i->getAngleTo(*m_i.s));
        //TODO: Respect haste increased turn speed
        overtime += static_cast<int>(ceil(angle / m_i.g->getWizardMaxTurnAngle()));
        //TODO: Respect damage increase auras
        //TODO: Honor fireball and frostbolt ultimates
        DangerField::Config conf = {
            m_i.g->getMagicMissileDirectDamage(),
            i->getCastRange() + m_i.s->getRadius(),
            cooldowns[ACTION_MAGIC_MISSILE],
            m_i.g->getMagicMissileCooldownTicks() + overtime,
            speed,
        };

        ret.add_field(std::make_unique<DangerField>(
            geom::Point2D(i->getX(), i->getY()),
            0,
            i->getVisionRange(),
            my_speed,
            my_life,
            my_max_life,
            conf
        ));
    }

    return ret;
}
