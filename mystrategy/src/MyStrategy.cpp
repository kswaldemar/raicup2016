#include "MyStrategy.h"

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES

#include "Logger.h"
#include "PotentialField.h"


#include <cmath>
#include <cstdlib>
#include <array>
#include <cassert>

using namespace model;
using namespace std;

static const int FIELD_SCALE_FACTOR = 10;
static const int FORWARD_SPEED = 100500;
static const int BACKWARD_SPEED = -100500;

void MyStrategy::move(const Wizard& self, const World& world, const Game& game, Move& move) {
    //Initialize common used variables, on each tick start
    initialize_info_pack(self, world, game);

    static bool first_call_ever = true;
    if (first_call_ever) {
        first_call_ever = false;
        m_field = std::make_unique<PotentialField>(world.getWidth(), world.getHeight(), FIELD_SCALE_FACTOR);
    }

    //PotentialField relaxation
    m_field->relax();

    //Add friendly and enemy objects to field
    m_field->add_waypoint({200, 200});

    Point2D next_pt = m_field->potential_move({self.getX(), self.getY()});

    const double turn_angle = self.getAngleTo(next_pt.x, next_pt.y);
    const double half_sector = PI / 4;
    if (turn_angle >= -half_sector && turn_angle <= half_sector) {
        move.setSpeed(FORWARD_SPEED);
    }
    move.setTurn(turn_angle);

    printf("My position = (%d; %d), go to (%d; %d)\n", (int) self.getX(), (int) self.getY(), next_pt.x, next_pt.y);
}

MyStrategy::MyStrategy() { }

void MyStrategy::initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game) {
    m_i.s = &self;
    m_i.w = &world;
    m_i.g = &game;
}