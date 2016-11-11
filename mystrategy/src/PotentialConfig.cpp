//
// Created by valdemar on 09.11.16.
//

#include "PotentialConfig.h"

#include <cstdio>
#include <initializer_list>
#include <cassert>

namespace potential {

struct FieldDesc {
    double v;
    double r;
};

/**
 * Config values
 */

/*
 * Vector field speed (value of each vector in field)
 */
const int WAYPOINT = 200;

const int ENEMY_ORC = 300;
const int ENEMY_FETISH = 310;
const int ENEMY_BUILDING = 350;
const int ENEMY_WIZARD = 380;

const int RP_ENEMY_ORC = 1000;
const int RP_ENEMY_FETISH = 1000;

/*
 * Other config
 */
//TODO: Вместо этого считать, кол-во ходов, которое нужно сделать, чтобы дойти до врага (с учетом деревьев)
const double DETECT_DISTANCE = 600; // Distance from which start to attract at all

/**
 * Get equivalent potential field value in particular point in vector field
 */
inline double get_value(double field_radius, double distance_to_center, double field_speed) {
    if (field_radius < distance_to_center) {
        return 0;
    }
    return (field_radius - distance_to_center) * field_speed;
}

/**
 * Functions implementations
 */
geom::Vec2D waypoint_attraction(const model::Wizard &who, const geom::Vec2D &waypoint_vector) {
    geom::Vec2D s{waypoint_vector.x - who.getX(), waypoint_vector.y - who.getY()};
    s = geom::normalize(s) * WAYPOINT;
    return s;
}

geom::Vec2D obstacle_avoidance(double radius, const geom::Vec2D &rp) {
    double x = rp.len();
    x -= radius;
    double y = 0;
    static const std::initializer_list<geom::Vec2D> fn {
        { 5, 20000 },
        { 10, 2000 },
        { 15, 100 },
        { 20, 50 },
        { 25, 25 },
        { 30, 15 },
        { 35, 10 }
    };

    for (const auto &i : fn) {
        if (x <= i.x) {
            y = i.y;
            break;
        }
    }

    return geom::normalize(rp) *= y;
}

geom::Vec2D most_attractive_enemy(InfoPack info, const model::CircularUnit *&enemy, bool &is_assault) {
    const auto &creeps = info.w->getMinions();
    const auto &buildings = info.w->getBuildings();
    const auto &wizards = info.w->getWizards();

    double max_pf_value = 0;
    double pf_value = 0;
    geom::Vec2D most_att{0, 0};
    geom::Vec2D att{0, 0};

    bool first_time = true;
    for (const auto &i : creeps) {
        if (i.getFaction() == info.s->getFaction()  || i.getFaction() == model::FACTION_NEUTRAL) {
            //Friendly
            continue;
        }
        pf_value = 0; //Potential field force in that point, if work with only that vector
        att = enemy_minion_attraction(*info.s, i, &pf_value);
        if (pf_value > max_pf_value || first_time) {
            first_time = false;
            max_pf_value = pf_value;
            most_att = att;
            enemy = reinterpret_cast<const model::CircularUnit *>(&i);
        }
    }

    for (const auto &i : buildings) {
        if (i.getFaction() == info.s->getFaction() || i.getFaction() == model::FACTION_NEUTRAL) {
            //Friendly
            continue;
        }
        pf_value = 0; //Potential field force in that point, if work with only that vector
        att = enemy_building_attraction(*info.s, i, &pf_value);
        if (pf_value > max_pf_value || first_time) {
            first_time = false;
            max_pf_value = pf_value;
            most_att = att;
            enemy = reinterpret_cast<const model::CircularUnit *>(&i);
        }
    }

    for (const auto &i : wizards) {
        if (i.getFaction() == info.s->getFaction() || i.getFaction() == model::FACTION_NEUTRAL) {
            //Friendly
            continue;
        }
        pf_value = 0; //Potential field force in that point, if work with only that vector
        att = enemy_wizard_attraction(*info.s, i, &pf_value);
        if (pf_value > max_pf_value || first_time) {
            first_time = false;
            max_pf_value = pf_value;
            most_att = att;
            enemy = reinterpret_cast<const model::CircularUnit *>(&i);
        }
    }

    printf("Most powerful enemy attraction = (%3.1lf, %3.1lf); max_pf = %lf\n", most_att.x, most_att.y, max_pf_value);
    if (max_pf_value > 0) {
        is_assault = true;
    }
    return most_att;
}

geom::Vec2D enemy_minion_attraction(const model::Wizard &who, const model::Minion &target, double *pf_value) {
    double dist = who.getDistanceTo(target);
    double shoot_range = g_info.g->getWizardCastRange();
    if (dist > DETECT_DISTANCE) {
        return {0, 0};
    }

    //Attractive to our shooting range field
    FieldDesc att_to_range{0, DETECT_DISTANCE};
    //Repelling, already at shooting range
    FieldDesc rep_to_range{0, shoot_range - 30};
    //Repelling from enemy attack range field
    FieldDesc rep_from_attack{0, 0};

    switch (target.getType()) {
        case model::MINION_ORC_WOODCUTTER:
            att_to_range.v = ENEMY_ORC;
            rep_from_attack.v = -RP_ENEMY_ORC;
            rep_from_attack.r = g_info.g->getOrcWoodcutterAttackRange() + (g_info.g->getStaffRange() / 2.0);
            break;
        case model::MINION_FETISH_BLOWDART:
            att_to_range.v = ENEMY_FETISH;
            rep_from_attack.v = -RP_ENEMY_FETISH;
            rep_from_attack.r = g_info.g->getFetishBlowdartAttackRange();
            break;
        default:
            assert(false);
            break;
    }

    rep_to_range.v = -att_to_range.v;

    const std::initializer_list<FieldDesc> enabled_fields = {
        att_to_range,
        rep_to_range,
        rep_from_attack
    };


    double sum_v = 0;
    double s_pf = 0;
    for (const auto &fld : enabled_fields) {
        if (dist > fld.r) {
            //Too far, not affected at all
            continue;
        }
        sum_v += fld.v; // Just sum vectors
        // Sum corresponding potential field value, will return negative values from repelling fields (fld.v < 0)
        s_pf += get_value(fld.r, dist, fld.v);
    }
    if (pf_value) {
        *pf_value = s_pf;
    }

    //Decision
    geom::Vec2D a{target.getX() - who.getX(), target.getY() - who.getY()};
    return geom::normalize(a) * sum_v;
}

geom::Vec2D enemy_wizard_attraction(const model::Wizard &who, const model::Wizard &target, double *pf_value) {
    double dist = who.getDistanceTo(target);
    double shoot_range = g_info.g->getWizardCastRange();
    if (dist > DETECT_DISTANCE) {
        return {0, 0};
    }

    //Attractive to our shooting range field
    std::initializer_list<FieldDesc> enabled_fields = {
        {ENEMY_WIZARD, DETECT_DISTANCE},
        {-ENEMY_WIZARD, shoot_range - 5}
    };

    double sum_v = 0;
    double s_pf = 0;
    for (const auto &fld : enabled_fields) {
        if (dist > fld.r) {
            //Too far, not affected at all
            continue;
        }
        sum_v += fld.v; // Just sum vectors
        // Sum corresponding potential field value, will return negative values from repelling fields (fld.v < 0)
        s_pf += get_value(fld.r, dist, fld.v);
    }
    if (pf_value) {
        *pf_value = s_pf;
    }

    //Decision
    geom::Vec2D a{target.getX() - who.getX(), target.getY() - who.getY()};
    return geom::normalize(a) * sum_v;
}

geom::Vec2D enemy_building_attraction(const model::Wizard &who, const model::Building &target, double *pf_value) {
    double dist = who.getDistanceTo(target);
    double shoot_range = g_info.g->getWizardCastRange();
    if (dist > DETECT_DISTANCE) {
        return {0, 0};
    }

    //Attractive to our shooting range field
    std::initializer_list<FieldDesc> enabled_fields = {
        {ENEMY_BUILDING, DETECT_DISTANCE},
        {-ENEMY_BUILDING, shoot_range}
    };

    double sum_v = 0;
    double s_pf = 0;
    for (const auto &fld : enabled_fields) {
        if (dist > fld.r) {
            //Too far, not affected at all
            continue;
        }
        sum_v += fld.v; // Just sum vectors
        // Sum corresponding potential field value, will return negative values from repelling fields (fld.v < 0)
        s_pf += get_value(fld.r, dist, fld.v);
    }
    if (pf_value) {
        *pf_value = s_pf;
    }

    //Decision
    geom::Vec2D a{target.getX() - who.getX(), target.getY() - who.getY()};
    return geom::normalize(a) * sum_v;

}

}