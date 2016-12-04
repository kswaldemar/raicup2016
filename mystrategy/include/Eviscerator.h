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
#include "PotentialField.h"
#include "UnitDesc.h"

#include <memory>

struct EnemyDesc {
    enum class Type {
        MINION_ORC,
        MINION_FETISH,
        WIZARD,
        TOWER
    };

    EnemyDesc(const model::LivingUnit *unit, Type type)
        : unit(unit),
          type(type) {
    }

    const model::LivingUnit *unit;
    Type type;
};


class Eviscerator {
public:

    struct DestroyDesc {
        const model::LivingUnit *unit;
        double att_range;
    };

    Eviscerator(const InfoPack &info);

    /*
     * Should be called at each tick start
     */
    void update_info(const InfoPack &info);

    static double calc_dead_zone(const RunawayUnit &me, const AttackUnit &enemy);

    /**
     * Check all enemies and choose best one.
     * Enemy stored in internal variable
     * @return true, if killing candidate found
     */
    int choose_enemy();

    DestroyDesc destroy(model::Move &move);

    void destroy(model::Move &move, const geom::Point2D &center, double radius) const;

    bool tower_maybe_attack_me(const TowerDesc &tower);

    bool can_shoot_to_target() const;
private:
    const InfoPack *m_i;
    //Set up in choose enemy
    const EnemyDesc *m_target;
    //Attraction to choosen enemy
    std::vector<EnemyDesc> m_enemies;
};
