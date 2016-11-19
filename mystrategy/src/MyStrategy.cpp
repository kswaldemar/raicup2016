#include "FieldsDescription.h"
#include "Config.h"
#include "MyStrategy.h"
#include "PathFinder.h"
#include "Logger.h"
#include "VisualDebug.h"

#include <cassert>
#include <list>

using namespace model;
using namespace std;
using namespace geom;

const double EPS = 0.00003;
InfoPack g_info;

enum Behaviour {
    BH_ATTACK,
    BH_SCOUT,
    BH_MINIMIZE_DANGER,
    BH_NOTHING
};


void MyStrategy::move(const Wizard &self, const World &world, const Game &game, Move &move) {
    //Initialize common used variables, on each tick start
    initialize_info_pack(self, world, game);

    static bool initialized = initialize_strategy(self, world, game);
    assert(initialized);

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

    /*
     * Per tick initialization
     */
    m_pf->update_info_pack(m_i);
    m_ev->update_info_pack(m_i);
    m_pf->update_reference(m_danger_map);

    //Pre actions
    VISUAL(beginPre());

    visualise_danger_map(m_danger_map, {self.getX(), self.getY()});

    VISUAL(endPre());

    VISUAL(beginPost());


    Behaviour current_action = BH_NOTHING;
    static Behaviour prev_action = BH_NOTHING;
    static Point2D prev_point{0, 0};

    Vec2D dir;
    double danger_level = m_danger_map.get_value(self.getX(), self.getY());

    if (danger_level <= config::ATTACK_THRESH && current_action == BH_NOTHING) {
        //Danger is ok to attack
        //TODO: Check many enemies and attack if not too dangerous

        double att_range = 0;
        const CircularUnit *enemy;
        const bool have_target = m_ev->choose_enemy();
        Vec2D enemy_attraction{0, 0};
        if (have_target) {
            //Enemy choosen
            prev_action = current_action;
            current_action = BH_ATTACK;
            auto description = m_ev->destroy(move);
            if (self.getDistanceTo(*description.unit) >= description.att_range) {
                if (prev_action != current_action || self.getDistanceTo(prev_point.x, prev_point.y) <= 1) {
                    dir = m_pf->find_way({description.unit->getX(), description.unit->getY()}, description.att_range, 0);
                    prev_point = {self.getX() + dir.x, self.getY() + dir.y};
                } else {
                    dir = {prev_point.x - self.getX(), prev_point.y - self.getY()};
                }
                m_pf->move_along(dir, move, true);
            } else {
                current_action = BH_MINIMIZE_DANGER;
            }
        }
    }

    if (danger_level <= config::SCOUT_THRESH && current_action == BH_NOTHING) {
        //Move by waypoints
        prev_action = current_action;
        current_action = BH_SCOUT;
        Vec2D wp_next = m_pf->get_next_waypoint();
        VISUAL(circle(wp_next.x, wp_next.y, PathFinder::WAYPOINT_RADIUS, 0x001111));
        if (prev_action != current_action || self.getDistanceTo(prev_point.x, prev_point.y) <= 4) {
            dir = m_pf->find_way(wp_next, PathFinder::WAYPOINT_RADIUS, 10);
            if (dir.len() < EPS) {
                //Seems no way
                //Then just go along direction
                Vec2D dwp{wp_next.x - self.getX(), wp_next.y - self.getY()};
                dwp = normalize(dwp);
                dwp *= 600 - PathFinder::GRID_SIZE;
                dir = m_pf->find_way(wp_next, PathFinder::WAYPOINT_RADIUS, 10);
            }
            auto my_cell = PathFinder::world_to_cell({self.getX(), self.getY()});
            auto centered = PathFinder::cell_to_world(my_cell);
            //VISUAL(fillCircle(centered.x, centered.y, 4, 0xFF0000));
            centered += dir;
            prev_point = centered;
            //prev_point = {dir.x + self.getX(), dir.y + self.getY()};
        }
        VISUAL(line(self.getX(), self.getY(), prev_point.x, prev_point.y, 0xFF0000));
        dir = {prev_point.x - self.getX(), prev_point.y - self.getY()};

        if (dir.len() > EPS) {
            m_pf->move_along(dir, move, false);
        }
    }

    if (current_action == BH_NOTHING || current_action == BH_MINIMIZE_DANGER) {
        prev_action = current_action;
        current_action = BH_MINIMIZE_DANGER;

        m_pf->get_previous_waypoint();
        //VISUAL(circle(wp_prev.x, wp_prev.y, 100, 0x000077));
        //Shoot to enemy, while retreat
        const bool have_target = m_ev->choose_enemy();
        if (have_target) {
            m_ev->destroy(move);
        }
        Point2D from{self.getX(), self.getY()};
        for (int i = 0; i < 100; ++i) {
            dir = damage_avoid_vector(from);
            from += dir;
        }
        //if (prev_action != current_action || self.getDistanceTo(prev_point.x, prev_point.y) <= 1) {
            dir = m_pf->find_way(from, PathFinder::GRID_SIZE * 2, 10);
        //    prev_point = {self.getX() + dir.x, self.getY() + dir.y};
        //} else {
        //    dir = {prev_point.x - self.getX(), prev_point.y - self.getY()};
        //}
        m_pf->move_along(dir, move, have_target);
    }

    //LOG(
    //    "[Danger %3.1lf%%] Dir (%3.1lf; %3.1lf); "
    //        "Obstacle vector: (%3.1lf; %3.1lf); "
    //        "Waypoint vector: (%3.1lf; %3.1lf); "
    //        "Enemy attraction vector (%3.1lf; %3.1lf); "
    //        "Damage avoid vector (%3.1lf; %3.1lf);\n",
    //    damage_avoidance.len(),
    //    dir.x, dir.y,
    //    obs_avoid.x, obs_avoid.y,
    //    waypoints.x, waypoints.y,
    //    enemy_attraction.x, enemy_attraction.y,
    //    damage_avoidance.x, damage_avoidance.y);

#ifdef RUNNING_LOCAL
    char buf[10];
    char c = 'N';
    switch (current_action) {
        case BH_MINIMIZE_DANGER:
            c = 'M';
            break;
        case BH_ATTACK:
            c = 'A';
            break;
        case BH_SCOUT:
            c = 'R';
            break;
        case BH_NOTHING:
            c = 'N';
            break;
    }
    sprintf(buf, "%c %3.1lf%%", c, m_danger_map.get_value(self.getX(), self.getY()));
    VISUAL(text(self.getX() - 70, self.getY() - 60, buf, 0xFF0000));
#endif


    VISUAL(endPost());
}

MyStrategy::MyStrategy() {
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
    std::array<double, all_steps> forces;
    forces.fill(config::DAMAGE_MAX_FEAR);

    Vec2D fwd{m_i.g->getWizardForwardSpeed(), m_i.g->getWizardStrafeSpeed()};
    double len = fwd.len();

    Vec2D back{-m_i.g->getWizardBackwardSpeed(), m_i.g->getWizardStrafeSpeed()};
    double len2 = back.len();


    const int steps = static_cast<int>(180.0 / ANGLE_STEP);
    for (int i = 0; i <= steps; ++i) {
        double angle = deg_to_rad(i * ANGLE_STEP);

        if (angle > pi) {
            len = len2;
        }

        Vec2D cur{len * cos(angle), len * sin(angle)};
        cur.x += from.x;
        cur.y += from.y;
        forces[i] = m_danger_map.get_value(cur.x, cur.y);
    }

    int min_idx = 0;
    for (int i = 0; i < forces.size(); ++i) {
        if (forces[i] < forces[min_idx]) {
            min_idx = i;
        }
    }

    double angle = deg_to_rad(min_idx * ANGLE_STEP);
    Vec2D best;
    if (angle > pi) {
        best.x += len2 * cos(angle);
        best.y += len2 * sin(angle);
    } else {
        best.x += len * cos(angle);
        best.y += len * sin(angle);
    }


    const double cur_danger = m_danger_map.get_value(m_i.s->getX(), m_i.s->getY());
//#ifdef RUNNING_LOCAL
//    Vec2D to_draw = normalize(best) * 50;
//    if (cur_danger > 0) {
//        VISUAL(line(from.x, from.y, from.x + to_draw.x, from.y + to_draw.y, 0x00CC00));
//    }
//#endif
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
            if (dist.len() <= vision.r && !updated[idx]) {
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
        add_unit_damage_fields({minion.getX(), minion.getY()}, attack_range, dead_zone_r);
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
        damage_fields.add_field(
            std::make_unique<fields::ExpRingField>(
                Point2D{tower.x, tower.y},
                0,
                tower.attack_range,
                false,
                fields::ExpConfig::from_two_points(config::DAMAGE_ZONE_CURVATURE, config::DAMAGE_MAX_FEAR, dead_zone_r)
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
            force = std::min(force, config::DAMAGE_MAX_FEAR);
            int color = static_cast<uint8_t>((255.0 / config::DAMAGE_MAX_FEAR) * force);
            color = 255 - color;
            color = (color << 16) | (color << 8) | color;
            VISUAL(fillRect(x_a - half - 1, y_a - half - 1, x_a + half, y_a + half, color));
        }
    }
#endif
}