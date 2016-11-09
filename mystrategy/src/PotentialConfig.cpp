//
// Created by valdemar on 09.11.16.
//

#include "PotentialConfig.h"

namespace potential {

/**
 * Config values
 */
const int WAYPOINT_COST = 4000;

/**
 * Functions implementations
 */

int calc_waypoint_cost(double distance_to_center) {
    //Simple linear function
    return static_cast<int>(WAYPOINT_COST - distance_to_center);
}

}