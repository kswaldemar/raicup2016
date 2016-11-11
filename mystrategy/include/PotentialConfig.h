//
// Created by valdemar on 09.11.16.
//

#pragma once

#include "InfoPack.h"
#include "model/Wizard.h"
#include "Vec2D.h"

extern const double EPS;

namespace potential {

geom::Vec2D waypoint_attraction(const model::Wizard &who, const geom::Vec2D &waypoint_vector);

geom::Vec2D obstacle_avoidance(double r, const geom::Vec2D &rp);

/**
 * Check all enemy attraction vectors and determine best enemy
 * @param info[in] info pack with world, game, self
 * @param enemy[out] pointer to simplified enemy description
 * @return Most powerful enemy attraction vector
 */
geom::Vec2D most_attractive_enemy(InfoPack info, const model::CircularUnit *&enemy, bool &is_assault);

geom::Vec2D enemy_minion_attraction(const model::Wizard &who, const model::Minion &target, double *pf_value = nullptr);
geom::Vec2D enemy_wizard_attraction(const model::Wizard &who, const model::Wizard &target, double *pf_value = nullptr);
geom::Vec2D enemy_building_attraction(const model::Wizard &who, const model::Building &target, double *pf_value = nullptr);


}
