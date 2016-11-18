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

const double pi = 3.14159265358979323846;
const double EPS = 0.00003;
InfoPack g_info;


void MyStrategy::move(const Wizard &self, const World &world, const Game &game, Move &move) {
    //Initialize common used variables, on each tick start
    initialize_info_pack(self, world, game);

    static bool initialized = initialize_strategy(self, world, game);
    assert(initialized);
    /*
     * Per tick initialization
     */
    m_pf->update_info_pack(m_i);
    m_ev->update_info_pack(m_i);

    //Update towers info
    update_shadow_towers(m_enemy_towers, world, self.getFaction());

    //Calculate danger map
    update_danger_map();

    //Pre actions
    VISUAL(beginPre());

    visualise_danger_map(m_danger_map, {self.getX(), self.getY()});

    VISUAL(endPre());
    VISUAL(beginPost());

    //Check about our death
    static int last_tick = 0;
    int cur_tick = world.getTickIndex();
    if (cur_tick - last_tick >= game.getWizardMinResurrectionDelayTicks()) {
        //Seems resurrection
        m_pf = make_unique<PathFinder>(m_i);

    }
    last_tick = cur_tick;

    //Handle shadow towers


    //Summary movement vector
    Vec2D sum{0, 0};

    //Avoid obstacles
    Vec2D obs_avoid = repelling_obs_avoidance_vector();
    Vec2D damage_avoidance = repelling_damage_avoidance_vector();

    char buf[10];
    sprintf(buf, "%3.1lf%%", damage_avoidance.len());
    VISUAL(text(self.getX() - 70, self.getY() - 60, buf, 0xFF0000));


    const bool have_target = m_ev->choose_enemy();
    Vec2D enemy_attraction{0, 0};

    Vec2D waypoints{0, 0};
    if (have_target) {
        //Enemy choosen
        m_ev->destroy(move);
        enemy_attraction = m_ev->apply_enemy_attract_field(self);
    } else {
        //Move by waypoints
        Vec2D wp_next = m_pf->get_next_waypoint();
        fields::ConstRingField nwp_field(wp_next, 0, 8000, config::NEXT_WAYPOINT_ATTRACTION);
        waypoints += nwp_field.apply_force(self.getX(), self.getY());
    }
    Vec2D wp_prev = m_pf->get_previous_waypoint();
    fields::ConstRingField pwp_field(wp_prev, 0, 8000, config::PREV_WAYPOINT_ATTRACTION);
    waypoints += pwp_field.apply_force(self.getX(), self.getY());

    Vec2D dir = obs_avoid + waypoints + damage_avoidance + enemy_attraction;
    if (dir.len() > EPS) {
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

geom::Vec2D MyStrategy::repelling_obs_avoidance_vector() {
    Vec2D ret{0, 0};


    const auto &buildings = m_i.w->getBuildings();
    const auto &wizards = m_i.w->getWizards();
    const auto &trees = m_i.w->getTrees();
    const auto &creeps = m_i.w->getMinions();

    const auto &avoid = [](double center_x, double center_y, double radius, const model::CircularUnit *who) -> Vec2D {
        double dist = radius + who->getRadius() + config::OBS_AVOID_EXTRA_DISTANCE;
        /*
         * В середине большое значение, зависящее от радиуса
         * (чтобы объекты с разным радиусам толкали с одинаковой силой)
         * Экспоненциальное угасание к концу
         */
        //auto cfg = fields::ExpConfig::from_two_points(config::OBS_AVOID_FORCE, dist, 0);
        auto field = fields::ConstRingField({center_x, center_y}, 0, dist, -config::OBS_AVOID_FORCE);
        auto vec = field.apply_force(who->getX(), who->getY());
        //if (vec.len() > EPS) {
        //    printf("Avoid: (%lf; %lf)\n", vec.x, vec.y);
        //}
        return vec;
    };

    for (const auto &item : buildings) {
        ret += avoid(item.getX(), item.getY(), item.getRadius(), m_i.s);
    }
    for (const auto &item : wizards) {
        if (item.isMe()) {
            continue;
        }
        ret += avoid(item.getX(), item.getY(), item.getRadius(), m_i.s);
    }
    for (const auto &item : trees) {
        ret += avoid(item.getX(), item.getY(), item.getRadius(), m_i.s);
    }
    for (const auto &item : creeps) {
        ret += avoid(item.getX(), item.getY(), item.getRadius(), m_i.s);
    }

    std::initializer_list<Vec2D> walls{
        {m_i.s->getX(),      0},
        {m_i.s->getX(),      m_i.w->getWidth()},
        {0,                  m_i.s->getY()},
        {m_i.w->getHeight(), m_i.s->getY()}
    };
    for (const auto &wall : walls) {
        ret += avoid(wall.x, wall.y, 0, m_i.s);
    }

    return ret;
}

geom::Vec2D MyStrategy::repelling_damage_avoidance_vector() {
    Vec2D ret{0, 0};
    double force = 0;
    Vec2D tmp;
    for (const auto &field : m_danger_map) {
        tmp = field->apply_force(m_i.s->getX(), m_i.s->getY());
        force += tmp.len();
        ret += tmp;
    }
    force = std::min(force, config::DAMAGE_MAX_FEAR);
    ret = normalize(ret) * force;
    return ret;
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
        damage_fields.emplace_back(
            std::make_unique<fields::ExpRingField>(
                unit_center,
                0,
                8000,
                false,
                fields::ExpConfig::from_two_points(config::DAMAGE_ZONE_CURVATURE, config::DAMAGE_MAX_FEAR, dead_zone_r)
            )
        );
    };
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

        int me_kill_time = m_ev->get_myself_death_time(*m_i.s, minion);
        double dead_zone_r = attack_range - me_kill_time * ME_RETREAT_SPEED;
        add_unit_damage_fields({minion.getX(), minion.getY()}, attack_range, dead_zone_r);
    }
    for (const auto &wizard : wizards) {
        if (wizard.getFaction() == FACTION_NEUTRAL || wizard.getFaction() == m_i.s->getFaction()) {
            continue;
        }

        double attack_range = wizard.getCastRange() + m_i.s->getRadius();
        int me_kill_time = m_ev->get_myself_death_time(*m_i.s, wizard);
        double dead_zone_r = attack_range - me_kill_time * ME_RETREAT_SPEED;
        add_unit_damage_fields({wizard.getX(), wizard.getY()}, attack_range, dead_zone_r);
    }

    for (const auto &tower : m_enemy_towers) {
        double attack_range = tower.attack_range;
        attack_range += m_i.s->getRadius();

        int me_kill_time = m_ev->get_myself_death_time(*m_i.s, tower);
        double dead_zone_r = attack_range - me_kill_time * ME_RETREAT_SPEED;
        add_unit_damage_fields({tower.x, tower.y}, attack_range, dead_zone_r);
        VISUAL(circle(tower.x, tower.y, tower.attack_range, 0x440000));
        char buf[10];
        sprintf(buf, "%3d", tower.rem_cooldown);
        VISUAL(text(tower.x, tower.y + 30, buf, 0x004400));
    }
}

void MyStrategy::visualise_danger_map(const MyStrategy::DangerMap &danger, const geom::Point2D &center) {
#ifdef RUNNING_LOCAL
    const int GRID_SIZE = 30;
    const int half = GRID_SIZE / 2;
    Vec2D tmp;
    for (int x = -600; x <= 600; x += GRID_SIZE) {
        for (int y = -600; y <= 600; y += GRID_SIZE) {
            double x_a = center.x + x;
            double y_a = center.y + y;
            double force = 0;
            for (const auto &fld : danger) {
                tmp = fld->apply_force(x_a, y_a);
                force += tmp.len();
            }
            force = std::min(force, config::DAMAGE_MAX_FEAR);
            int color = static_cast<uint8_t>((255.0 / config::DAMAGE_MAX_FEAR) * force);
            color = 255 - color;
            color = (color << 16) | (color << 8) | color;
            VISUAL(fillRect(x_a - half - 1, y_a - half - 1, x_a + half, y_a + half, color));
        }
    }
#endif
}