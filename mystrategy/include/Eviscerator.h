//
// Created by valdemar on 12.11.16.
//

#pragma once

#include "model/Move.h"
#include "model/Building.h"
#include "model/Wizard.h"
#include "model/Minion.h"
#include "Vec2D.h"
#include "InfoPack.h"
#include "VectorField.h"

#include <memory>

//struct EnemyInfo {
//public:
//
//    geom::Point2D pos;
//
//};

class Eviscerator {
public:

    Eviscerator(const InfoPack &info);

    /*
     * Should be called at each tick start
     */
    void update_info_pack(const InfoPack &info);

    int get_incoming_damage(const geom::Point2D &me, double me_radius, const model::Minion &enemy);
    int get_incoming_damage(const geom::Point2D &me, double me_radius, const model::Building &enemy);
    int get_incoming_damage(const geom::Point2D &me, double me_radius, const model::Wizard &enemy);

    int get_myself_death_time(const model::Wizard &me, const model::Minion &enemy);
    int get_myself_death_time(const model::Wizard &me, const model::Building &enemy);
    int get_myself_death_time(const model::Wizard &me, const model::Wizard &enemy);

    /**
     * Check all enemies and choose best one.
     * Enemy stored in internal variable
     * @return true, if killing candidate found
     */
    bool choose_enemy();

    void destroy(model::Move &move);

    geom::Vec2D apply_enemy_attract_field(const model::Wizard &me);

private:
    const InfoPack *m_i;
    //Set up in choose enemy
    const model::Wizard *m_en_wz;
    const model::Building *m_en_building;
    const model::Minion *m_en_minion;
    //Attraction to choosen enemy
    std::unique_ptr<fields::IVectorField> m_attract_field;
};
