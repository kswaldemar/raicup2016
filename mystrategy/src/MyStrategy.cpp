#include "FieldsDescription.h"
#include "Config.h"
#include "MyStrategy.h"
#include "PathFinder.h"
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


    //Move by waypoints
    Vec2D wp_next = m_pf->get_next_waypoint();
    Vec2D wp_prev = m_pf->get_previous_waypoint();
    fields::ConstRingField nwp_field(wp_next, 0, 8000, config::NEXT_WAYPOINT_ATTRACTION);
    fields::ConstRingField pwp_field(wp_prev, 0, 8000, config::PREV_WAYPOINT_ATTRACTION);
    Vec2D waypoints{0, 0};
    waypoints += nwp_field.apply_force(self.getX(), self.getY());
    waypoints += pwp_field.apply_force(self.getX(), self.getY());

    //Avoid obstacles
    Vec2D obs_avoid = repelling_obs_avoidance_vector();

    //Just for test, it should depend on our hp level
    std::vector<fields::ConstRingField> damage_fields;
    const auto &creeps = world.getMinions();
    const auto &wizards = world.getWizards();
    const auto &buildings = world.getBuildings();
    for (const auto &minion : creeps) {

        if (minion.getFaction() == FACTION_NEUTRAL || minion.getFaction() == self.getFaction()) {
            continue;
        }
        double attack_range;

        if (minion.getType() == MINION_ORC_WOODCUTTER) {
            attack_range = game.getOrcWoodcutterAttackRange();
        } else {
            attack_range = game.getFetishBlowdartAttackRange();
        }
        attack_range += self.getRadius() + config::DMG_AVOID_EXTRA_DISTANCE;

        int incoming_damage = m_ev->get_incoming_damage({self.getX(), self.getY()}, self.getRadius(), minion);
        damage_fields.emplace_back(Point2D{minion.getX(), minion.getY()}, 0, attack_range, -incoming_damage);
    }
    for (const auto &wizard : wizards) {
        if (wizard.getFaction() == FACTION_NEUTRAL || wizard.getFaction() == self.getFaction()) {
            continue;
        }

        double attack_range = wizard.getCastRange();
        attack_range += self.getRadius() + config::DMG_AVOID_EXTRA_DISTANCE;

        int incoming_damage = m_ev->get_incoming_damage({self.getX(), self.getY()}, self.getRadius(), wizard);
        damage_fields.emplace_back(Point2D{wizard.getX(), wizard.getY()}, 0, attack_range, -incoming_damage);
    }
    for (const auto &tower : buildings) {
        if (tower.getFaction() == self.getFaction()) {
            continue;
        }

        double attack_range = tower.getAttackRange();
        attack_range += self.getRadius() + config::DMG_AVOID_EXTRA_DISTANCE;

        int incoming_damage = m_ev->get_incoming_damage({self.getX(), self.getY()}, self.getRadius(), tower);
        damage_fields.emplace_back(Point2D{tower.getX(), tower.getY()}, 0, attack_range, -incoming_damage);
    }

    Vec2D damage_avoidance{0, 0};
    for (const auto &field : damage_fields) {
        damage_avoidance += field.apply_force(self.getX(), self.getY());
    }

    const bool have_target = m_ev->choose_enemy();
    Vec2D enemy_attraction{0, 0};
    if (have_target) {
        //Enemy choosen
        m_ev->destroy(move);
        enemy_attraction = m_ev->apply_enemy_attract_field(self);
    }

    Vec2D dir = obs_avoid + waypoints + damage_avoidance + enemy_attraction;
    if (dir.len() > EPS) {
        m_pf->move_along(dir, move, have_target);
    }


    printf("Obstacle vector: (%3.1lf; %3.1lf); Attractive vector: (%3.1lf; %3.1lf); Damage avoid vector (%3.1lf; %3.1lf);\n",
           obs_avoid.x, obs_avoid.y, waypoints.x, waypoints.y, damage_avoidance.x, damage_avoidance.y);
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
        auto cfg = fields::ExpConfig::from_two_points(config::OBS_AVOID_PEEK_KOEF * dist, 1, dist);
        auto field = fields::ExpRingField({center_x, center_y}, 0, dist, false, cfg);
        auto vec = field.apply_force(who->getX(), who->getY());
        if (vec.len() > EPS) {
            printf("Avoid: (%lf; %lf)\n", vec.x, vec.y);
        }
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
    printf("wall ");
    for (const auto &wall : walls) {
        ret += avoid(wall.x, wall.y, 0, m_i.s);
    }

    return ret;
}
