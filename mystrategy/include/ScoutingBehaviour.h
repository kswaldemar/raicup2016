//
// Created by valdemar on 08.11.16.
//

#include "Behaviour.h"

class ScoutingBehaviour : public Behaviour {
public:
    void behave(model::Move &move) override;
    int get_score() override;
};