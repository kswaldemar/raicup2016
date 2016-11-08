#include "MyStrategy.h"

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES

#include "Logger.h"
#include "Behaviour.h"
#include "ScoutingBehaviour.h"

#include <cmath>
#include <cstdlib>
#include <array>
#include <cassert>

using namespace model;
using namespace std;

void MyStrategy::move(const Wizard& self, const World& world, const Game& game, Move& move) {

    //Initialize common used variables, on each tick start
    initialize_info_pack(self, world, game);

    //Lookaround and choose behaviour
    Behaviour *now = choose_behaviour();
    assert(now);
    now->update_info_pack(m_i);
    now->behave(move);
}

MyStrategy::MyStrategy() { }

void MyStrategy::initialize_info_pack(const model::Wizard &self, const model::World &world, const model::Game &game) {
    m_i.s = &self;
    m_i.w = &world;
    m_i.g = &game;
}

Behaviour *MyStrategy::choose_behaviour() {
    //There can be only one strategy process, so use static

    /*
     * All possible behaviours
     */
    constexpr uint16_t NUMBER_OF_BEHAVIOURS = 1;
    static ScoutingBehaviour scouting;
    //static AssaultBehaviour assault;
    //static RetreatBehaviour retreat;

    static std::array<Behaviour *, NUMBER_OF_BEHAVIOURS> all_behaviours {
        {&scouting}
    };

    static bool only_one_time = true;
    if (only_one_time) {
        only_one_time = false;
        for (auto &i: all_behaviours) {
            i->init(m_i);
        }
    }

    //Here smart logic to choose right behaviour, using potential fields etc.
    int max_score = 0;
    Behaviour *ret = nullptr;
    for (const auto &i : all_behaviours) {
        int score = i->get_score();
        if (score > max_score) {
            ret = i;
        }
    }
    return ret;
}