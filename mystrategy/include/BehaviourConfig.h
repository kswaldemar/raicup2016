//
// Created by valdemar on 05.12.16.
//

#pragma once

struct BehaviourConfig {
    //How long enemy wizard can pursuit us. More value - more fearing of enemy wizards
    static constexpr int max_runaway_time = 100;
    //General decision values
    static constexpr double danger_attack_t = 0.8; ///< Maximum danger value to attack
    static constexpr double danger_bonus_earn_t = 0.6;
    static constexpr double danger_scout_t = 0.6;
};
