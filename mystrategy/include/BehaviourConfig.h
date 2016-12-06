//
// Created by valdemar on 05.12.16.
//

#pragma once

struct BehaviourConfig {
    //How long enemy wizard can pursuit us. More value - more fearing of enemy wizards
    static constexpr int max_runaway_time = 100;
    //General decision values
    static constexpr double danger_attack_t = 0.7; ///< Maximum danger value to attack
    static constexpr double danger_bonus_earn_t = 0.6;
    static constexpr double danger_scout_t = 0.6;

    //Normalized from 0 to 1
    static struct Damage {

        double center_mult = 2;

        double orc = 0.01;
        double fetish = 0.01;
        double tower = 0.01;
        double tower_active = 0.5;
        double wizard = 0.02;
    } damage;

    //Normalized from 0 to 1
    static struct NavK {
        /*
         * Coefficient k in linear field formula y = k*x + c
         */
        double next_wp = 0.0005;
        double prev_wp = 0.0005;
        double bonus = 0.0005;

        //TODO: Use different values for each kind of enemy
        //TODO: Force should depend on enemy HP
        double attack = 0.0008;
    } navigation_k;
};
