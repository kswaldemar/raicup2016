//
// Created by valdemar on 08.11.16.
//

#pragma once

#include "ExWorld.h"
#include "model/Wizard.h"
#include "model/World.h"
#include "model/Game.h"

#include <memory>

struct InfoPack {
    const model::Wizard* s;
    const model::World* w;
    const model::Game* g;

    std::unique_ptr<ExWorld> ew;
};