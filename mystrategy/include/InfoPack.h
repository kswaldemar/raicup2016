//
// Created by valdemar on 08.11.16.
//

#pragma once

namespace model {

class Wizard;
class World;
class Game;

}

struct InfoPack {
    const model::Wizard* s;
    const model::World* w;
    const model::Game* g;
};