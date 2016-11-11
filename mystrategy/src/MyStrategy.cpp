#include <cassert>
#include "FieldsDescription.h"
#include "FieldsConfig.h"
#include "MyStrategy.h"
#include "PathFinder.h"
#include "PotentialConfig.h"

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
    }

    /*
     * Per tick initialization
     */




    /*
     * Use new api
     */
    Vec2D wp = m_pf->get_next_waypoint();
    fields::LinearRingField wp_field(wp, 0, 8000, FieldsConfig::WAYPOINT_ATTRACTION);
    Vec2D attractive = wp_field.apply_force(self.getX(), self.getY());
    Vec2D obs_avoid = repelling_obs_avoidance_vector();

    printf("Obstacle vector: (%3.1lf; %3.1lf); Attractive vector: (%3.1lf; %3.1lf)\n",
           obs_avoid.x, obs_avoid.y, attractive.x, attractive.y);

    Vec2D dir = obs_avoid + attractive;
    if (dir.len() > EPS) {
        m_pf->move_along(dir, move, false);
    }

    return;



    //enum Behaviour {
    //    DO_RUNAWAY,
    //    DO_ASSAULT,
    //    DO_SCOUTING
    //};
    //Behaviour behaviour;
    /*
     * Calculate vectors
     */
    //Vec2D obs_avoidance = repelling_obs_avoidance_vector();
    //const CircularUnit *best_enemy = nullptr;
    //bool assault = false;
    //Vec2D en_attraction = potential::most_attractive_enemy(m_i, best_enemy, assault);
    //const bool is_enemy_near = assault;
    //
    //Vec2D attraction;
    //
    //behaviour = DO_SCOUTING;
    ////Simplest way to leave from danger
    //const int LOW_HP = 37;
    //if (self.getLife() <= LOW_HP) {
    //    behaviour = DO_RUNAWAY;
    //} else if (is_enemy_near) {
    //    assert(best_enemy);
    //    behaviour = DO_ASSAULT;
    //}
    //
    //switch (behaviour) {
    //    case DO_RUNAWAY: {
    //        Vec2D wp = m_pf->get_previous_waypoint();
    //        attraction = potential::waypoint_attraction(self, wp);
    //    }
    //        break;
    //    case DO_SCOUTING: {
    //        Vec2D wp = m_pf->get_next_waypoint();
    //        attraction = potential::waypoint_attraction(self, wp);
    //    }
    //        break;
    //    case DO_ASSAULT: {
    //        attraction += en_attraction;
    //        double distance = self.getDistanceTo(*best_enemy);
    //        if (distance <= self.getCastRange()) {
    //            //Target near
    //            double angle = self.getAngleTo(*best_enemy);
    //            const double very_small = 1 * (pi / 180.0);
    //            if (std::abs(angle) > very_small) {
    //                move.setTurn(angle);
    //            }
    //            if (std::abs(angle) < game.getStaffSector() / 2.0) {
    //                //Attack
    //                move.setAction(ACTION_MAGIC_MISSILE);
    //                move.setCastAngle(angle);
    //                move.setMinCastDistance(distance - best_enemy->getRadius() + game.getMagicMissileRadius());
    //            }
    //        }
    //    }
    //            break;
    //}
    //
    //printf("[%d] Obstacle vector: (%3.1lf; %3.1lf); Attractive vector: (%3.1lf; %3.1lf)\n",
    //       behaviour,
    //       obs_avoidance.x,
    //       obs_avoidance.y,
    //       attraction.x,
    //       attraction.y);
    //
    //Vec2D dir = obs_avoidance + attraction;
    //if (dir.len() > EPS) {
    //    move_along(dir, move, behaviour == DO_ASSAULT);
    //}

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
        double dist = radius + who->getRadius() + FieldsConfig::OBS_AVOID_EXTRA_DISTANCE;
        /*
         * В середине большое значение, зависящее от радиуса
         * (чтобы объекты с разным радиусам толкали с одинаковой силой)
         * Экспоненциальное угасание к концу
         */
        auto cfg = fields::ExpConfig::from_two_points(FieldsConfig::OBS_AVOID_PEEK_KOEF * dist, 1, dist);
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
