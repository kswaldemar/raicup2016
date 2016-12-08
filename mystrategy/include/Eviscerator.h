//
// Created by valdemar on 12.11.16.
//

#pragma once

#include "Vec2D.h"
#include "InfoPack.h"
#include "PotentialField.h"
#include "UnitDesc.h"
#include "BulletHandler.h"
#include "FieldMap.h"
#include "model/Move.h"
#include "model/Building.h"
#include "model/Wizard.h"
#include "model/Minion.h"


#include <memory>
#include <map>

struct EnemyDesc {
    enum class Type {
        MINION_ORC,
        MINION_FETISH,
        WIZARD,
        TOWER
    };

    EnemyDesc(const model::LivingUnit *unit, Type type, double score)
        : unit(unit),
          type(type),
          score(score) {
    }

    const model::LivingUnit *unit;
    Type type;
    double score;
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

    /**
     * Check all enemies and choose best one.
     * Enemy stored in internal variable
     * @return true, if killing candidate found
     */
    double choose_enemy();

    DestroyDesc destroy(model::Move &move);

    void destroy(model::Move &move, const geom::Point2D &center, double radius) const;

    bool tower_maybe_attack_me(const MyBuilding &tower);

    bool can_shoot_to_target() const;

    double get_minion_aggression_radius(const model::Minion &creep) const;

    bool is_minion_attack_me(const model::Minion &creep) const;

    bool can_leave_battlefield() const;

    const fields::FieldMap &get_bullet_damage_map() const;

    std::pair<geom::Point2D, double> get_fireball_best_point() const;

    double get_fireball_damage(const geom::Point2D &center) const;

private:
    BulletHandler m_bhandler;

    const InfoPack *m_i;
    //Set up in choose enemy
    const EnemyDesc *m_target;
    //Minion target and distance to target
    std::map<long long int, std::pair<const model::LivingUnit *, double>> m_minion_target_by_id;
    std::vector<EnemyDesc> m_possible_targets;
    fields::FieldMap m_bullet_map = fields::FieldMap(fields::FieldMap::ADD);

};
