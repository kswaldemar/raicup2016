#include "FieldsDescription.h"
#include "Config.h"
#include "MyStrategy.h"
#include "PathFinder.h"
#include "Logger.h"
#include "VisualDebug.h"

#include <cassert>

using namespace model;
using namespace std;
using namespace geom;

const double pi = 3.14159265358979323846;
const double EPS = 0.00003;
InfoPack g_info;


void MyStrategy::move(const Wizard &self, const World &world, const Game &game, Move &move) {
    //Initialize common used variables, on each tick start
    initialize_info_pack(self, world, game);

    static bool first_call_ever = true;
    if (first_call_ever) {
        first_call_ever = false;
        m_pf = make_unique<PathFinder>(m_i);
        m_ev = make_unique<Eviscerator>(m_i);
    }
    /*
     * Per tick initialization
     */
    m_pf->update_info_pack(m_i);
    m_ev->update_info_pack(m_i);

    VISUAL(beginPre);
    VISUAL(circle, self.getX(), self.getY(), self.getRadius() * 2, 0x00ff00);
    VISUAL(endPre);

    //Check about our death
    static int last_tick = 0;
    int cur_tick = world.getTickIndex();
    if (cur_tick - last_tick >= game.getWizardMinResurrectionDelayTicks()) {
        //Seems ressurection
        m_pf = make_unique<PathFinder>(m_i);

    }
    last_tick = cur_tick;

    //Summary movement vector
    Vec2D sum{0, 0};

    //Avoid obstacles
    Vec2D obs_avoid = repelling_obs_avoidance_vector();
    Vec2D damage_avoidance = repelling_damage_avoidance_vector();


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


    LOG(
        "[Danger %3.1lf%%] Dir (%3.1lf; %3.1lf); "
            "Obstacle vector: (%3.1lf; %3.1lf); "
            "Waypoint vector: (%3.1lf; %3.1lf); "
            "Enemy attraction vector (%3.1lf; %3.1lf); "
            "Damage avoid vector (%3.1lf; %3.1lf);\n",
        damage_avoidance.len(),
        dir.x, dir.y,
        obs_avoid.x, obs_avoid.y,
        waypoints.x, waypoints.y,
        enemy_attraction.x, enemy_attraction.y,
        damage_avoidance.x, damage_avoidance.y);
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
    std::vector<std::unique_ptr<fields::IVectorField>> damage_fields;
    const auto &creeps = m_i.w->getMinions();
    const auto &wizards = m_i.w->getWizards();
    const auto &buildings = m_i.w->getBuildings();
    const double ME_RETREAT_SPEED = 4.0;

    const auto &add_unit_damage_fields = [&damage_fields](const Point2D &unit_center,
                                                          double attack_range,
                                                          double dead_zone_r) {
        //So, there is some radius from which I cannot retreat

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
    for (const auto &tower : buildings) {
        if (tower.getFaction() == m_i.s->getFaction()) {
            continue;
        }

        double attack_range;
        if (tower.getType() == BUILDING_GUARDIAN_TOWER) {
            attack_range = m_i.g->getGuardianTowerAttackRange();
        } else {
            attack_range = m_i.g->getFactionBaseAttackRange();
        }
        attack_range += m_i.s->getRadius();

        int me_kill_time = m_ev->get_myself_death_time(*m_i.s, tower);
        double dead_zone_r = attack_range - me_kill_time * ME_RETREAT_SPEED;
        add_unit_damage_fields({tower.getX(), tower.getY()}, attack_range, dead_zone_r);
    }

    Vec2D ret{0, 0};
    for (const auto &field : damage_fields) {
        ret += field->apply_force(m_i.s->getX(), m_i.s->getY());
    }
    if (ret.len() > config::DAMAGE_MAX_FEAR) {
        ret = normalize(ret) * config::DAMAGE_MAX_FEAR;
    }
    return ret;
}
