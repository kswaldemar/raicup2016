#include "MyStrategy.h"
#include "PathFinder.h"
#include "PotentialConfig.h"

using namespace model;
using namespace std;
using namespace geom;

const double pi = 3.14159265358979323846;
static const double EPS = 0.00003;


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





    enum Behaviour {
        DO_RUNAWAY,
        DO_ASSAULT,
        DO_SCOUTING
    };
    Behaviour behaviour = DO_SCOUTING;

    /*
     * Calculate vectors
     */
    Vec2D obs_avoidance = repelling_obs_avoidance_vector();
    Vec2D en_attraction = enemy_attraction_vector();
    const bool is_enemy_near = en_attraction.len() > EPS;

    Vec2D attraction;

    //Simplest way to leave from danger
    const int LOW_HP = 75;
    if (self.getLife() <= LOW_HP) {
        behaviour = DO_RUNAWAY;
    }

    if (is_enemy_near) {
        behaviour = DO_ASSAULT;
    }

    switch (behaviour) {
        case DO_RUNAWAY: {
            Vec2D wp = m_pf->get_previous_waypoint();
            attraction = potential::waypoint_attraction(self, wp);
        }
            break;
        case DO_SCOUTING: {
            Vec2D wp = m_pf->get_next_waypoint();
            attraction = potential::waypoint_attraction(self, wp);
        }
            break;
        case DO_ASSAULT: {
        }
                break;
    }

    //printf("Repelling vector: (%3.1lf; %3.1lf); Attractive vector: (%3.1lf; %3.1lf)\n",
    //       repelling.x,
    //       repelling.y,
    //       attraction.x,
    //       attraction.y);

    Vec2D dir = obs_avoidance + attraction;
    if (dir.len() > EPS) {
        move_along(dir, move, is_enemy_near);
    }

}

MyStrategy::MyStrategy() {
}

void MyStrategy::initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game) {
    m_i.s = &self;
    m_i.w = &world;
    m_i.g = &game;
}

geom::Vec2D MyStrategy::repelling_obs_avoidance_vector() {
    Vec2D ret{0, 0};

    const double self_x = m_i.s->getX();
    const double self_y = m_i.s->getY();
    const double self_r = m_i.s->getRadius();

    const auto &buildings = m_i.w->getBuildings();
    const auto &wizards = m_i.w->getWizards();
    const auto &trees = m_i.w->getTrees();
    const auto &creeps = m_i.w->getMinions();

    for (const auto &item : buildings) {
        Vec2D rp(self_x - item.getX(), self_y - item.getY());
        ret += potential::obstacle_avoidance(item.getRadius() + self_r, rp);
    }
    for (const auto &item : wizards) {
        if (item.isMe()) {
            continue;
        }
        Vec2D rp(self_x - item.getX(), self_y - item.getY());
        ret += potential::obstacle_avoidance(item.getRadius() + self_r, rp);
    }
    for (const auto &item : trees) {
        Vec2D rp(self_x - item.getX(), self_y - item.getY());
        ret += potential::obstacle_avoidance(item.getRadius() + self_r, rp);
    }
    for (const auto &item : creeps) {
        Vec2D rp(self_x - item.getX(), self_y - item.getY());
        ret += potential::obstacle_avoidance(item.getRadius() + self_r, rp);
    }

    std::initializer_list<Vec2D> walls{
        {m_i.s->getX(),      0},
        {m_i.s->getX(),      m_i.w->getWidth()},
        {0,                  m_i.s->getY()},
        {m_i.w->getHeight(), m_i.s->getY()}
    };
    for (const auto &wall : walls) {
        Vec2D rp{self_x - wall.x, self_y - wall.y};
        ret += potential::obstacle_avoidance(self_r, rp);
    }

    return ret;
}

geom::Vec2D MyStrategy::enemy_attraction_vector() {
    return {0, 0};
}

void MyStrategy::move_along(const Vec2D &dir, model::Move &move, bool hold_face) {
    const double turn_angle = m_i.s->getAngleTo(m_i.s->getX() + dir.x, m_i.s->getY() + dir.y);
    double strafe = sin(turn_angle);
    double f_b = cos(turn_angle);

    //Checking for haste
    const auto &statuses = m_i.s->getStatuses();
    for (const auto &st : statuses) {
        if (st.getType() == STATUS_HASTENED) {
            strafe *= m_i.g->getHastenedMovementBonusFactor();
            f_b *= m_i.g->getHastenedMovementBonusFactor();
        }
    }
    strafe *= m_i.g->getWizardStrafeSpeed();

    if (f_b > 0) {
        f_b *= m_i.g->getWizardForwardSpeed();
    } else {
        f_b *= m_i.g->getWizardBackwardSpeed();
    }

    move.setStrafeSpeed(strafe);
    move.setSpeed(f_b);

    if (!hold_face) {
        move.setTurn(turn_angle);
    }
}
