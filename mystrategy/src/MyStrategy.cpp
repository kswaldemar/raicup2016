#include "MyStrategy.h"
#include "PotentialConfig.h"


using namespace model;
using namespace std;
using namespace geom;

const double pi = 3.14159265358979323846;
static const double OBSTACLE_AVOIDANCE_EXTRA_RADIUS = 0;
static const double MOVE_VECTOR_EPS = 0.00003;


void MyStrategy::move(const Wizard &self, const World &world, const Game &game, Move &move) {
    //Initialize common used variables, on each tick start
    initialize_info_pack(self, world, game);

    static bool first_call_ever = true;
    if (first_call_ever) {
        first_call_ever = false;
    }

    /*
     * Per tick initialization
     */

    /*
     * Calulate vectors
     */

    Vec2D repelling = recalculate_repelling_vector();
    Vec2D attraction = recalculate_attraction_vector();

    printf("Repelling vector: (%3.1lf; %3.1lf); Attractive vector: (%3.1lf; %3.1lf)\n",
           repelling.x,
           repelling.y,
           attraction.x,
           attraction.y);

    Vec2D dir = repelling + attraction;
    if (dir.len() > MOVE_VECTOR_EPS) {
        move_along(dir, move, true);
    }

}

MyStrategy::MyStrategy() {
}

void MyStrategy::initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game) {
    m_i.s = &self;
    m_i.w = &world;
    m_i.g = &game;
}

geom::Vec2D MyStrategy::recalculate_repelling_vector() {
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
        ret += potential::obstacle_avoidance(item.getRadius() + self_r + OBSTACLE_AVOIDANCE_EXTRA_RADIUS, rp);
    }
    for (const auto &item : wizards) {
        if (item.isMe()) {
            continue;
        }
        Vec2D rp(self_x - item.getX(), self_y - item.getY());
        ret += potential::obstacle_avoidance(item.getRadius() + self_r + OBSTACLE_AVOIDANCE_EXTRA_RADIUS, rp);
    }
    for (const auto &item : trees) {
        Vec2D rp(self_x - item.getX(), self_y - item.getY());
        ret += potential::obstacle_avoidance(item.getRadius() + self_r + OBSTACLE_AVOIDANCE_EXTRA_RADIUS, rp);
    }
    for (const auto &item : creeps) {
        Vec2D rp(self_x - item.getX(), self_y - item.getY());
        ret += potential::obstacle_avoidance(item.getRadius() + self_r + OBSTACLE_AVOIDANCE_EXTRA_RADIUS, rp);
    }

    std::initializer_list<Vec2D> walls{
        {m_i.s->getX(),      0},
        {m_i.s->getX(),      m_i.w->getWidth()},
        {0,                  m_i.s->getY()},
        {m_i.w->getHeight(), m_i.s->getY()}
    };
    for (const auto &wall : walls) {
        Vec2D rp{self_x - wall.x, self_y - wall.y};
        ret += potential::obstacle_avoidance(self_r + OBSTACLE_AVOIDANCE_EXTRA_RADIUS, rp);
    }

    return ret;
}

geom::Vec2D MyStrategy::recalculate_attraction_vector() {
    Vec2D waypoint{200, 200};
    Vec2D wp_a{waypoint.x - m_i.s->getX(), waypoint.y - m_i.s->getY()};
    Vec2D attraction = potential::waypoint_attraction(wp_a);
    return attraction;
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

    printf("Move like %lf; %lf\n", strafe, f_b);

    if (!hold_face) {
        move.setTurn(turn_angle);
    }
}
