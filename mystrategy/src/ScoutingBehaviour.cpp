//
// Created by valdemar on 08.11.16.
//

#include "ScoutingBehaviour.h"

#include "model/Game.h"

using namespace model;

void ScoutingBehaviour::behave(model::Move &move) {
    move.setAction(ACTION_MAGIC_MISSILE);
    move.setSpeed(m_i.g->getWizardForwardSpeed());
}

int ScoutingBehaviour::get_score() {
    return 1;

}
