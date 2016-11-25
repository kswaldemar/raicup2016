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
#include "UnitDesc.h"

#include <memory>

//struct EnemyInfo {
//public:
//
//    geom::Point2D pos;
//
//};

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
        const double att_range;
    };

    Eviscerator(const InfoPack &info);

    /*
     * Should be called at each tick start
     */
    void update_info(const InfoPack &info);

    int get_myself_death_time(const model::Wizard &me, const model::Minion &enemy);
    int get_myself_death_time(const model::Wizard &me, const model::Building &enemy);
    int get_myself_death_time(const model::Wizard &me, const model::Wizard &enemy);
    int get_myself_death_time(const model::Wizard &me, const TowerDesc &enemy);

    static double calc_dead_zone(const RunawayUnit &me, const AttackUnit &enemy);

    /**
     * Check all enemies and choose best one.
     * Enemy stored in internal variable
     * @return true, if killing candidate found
     */
    int choose_enemy();

    DestroyDesc destroy(model::Move &move);

    geom::Vec2D apply_enemy_attract_field(const model::Wizard &me);

    bool tower_maybe_attack_me(const TowerDesc &tower);

    const std::vector<EnemyDesc> &get_enemies() const;

    bool can_shoot_to_target() const;
private:
    const InfoPack *m_i;
    //Set up in choose enemy
    const EnemyDesc *m_target;
    //Attraction to choosen enemy
    std::unique_ptr<fields::IField> m_attract_field;
    std::vector<EnemyDesc> m_enemies;
};
