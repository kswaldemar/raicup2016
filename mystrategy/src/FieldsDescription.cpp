//
// Created by valdemar on 11.11.16.
//

#include "FieldsDescription.h"
#include "BehaviourConfig.h"
#include <cassert>

namespace fields {

ConstField::ConstField(const geom::Point2D &center, double r1, double r2, double force)
    : PotentialField(center, r1, r2),
      m_force(force) {
    assert(r1 >= 0);
    assert(r2 >= 0);
}

double ConstField::calc_force(double sqr_x) const {
    return m_force;
}


LinearField::LinearField(const geom::Point2D &center, double r1, double r2, double k, double c)
    : PotentialField(center, r1, r2),
      m_k(k), m_c(c) {

}

double LinearField::calc_force(double sqr_x) const {
    const double x = sqrt(sqr_x);
    return m_k * x + m_c;
}


DangerField::DangerField(const geom::Point2D &center,
                         double r1,
                         double r2,
                         double my_speed,
                         int my_life,
                         DangerField::Config enemy)
    : PotentialField(center, r1, r2),
      m_speed(my_speed),
      m_life(my_life),
      m_enemy(enemy) {

}

double DangerField::calc_force(double sqr_x) const {
    const double x = sqrt(sqr_x);

    int retreat_time;
    if (m_enemy.speed <= m_speed) {
        // We are faster or have same speed.
        // Think that we can go out from attack range and keep going without damage.
        // Assume that enemy will not go close while we are in attack range.
        if (x < m_enemy.attack_range) {
            retreat_time = static_cast<int>(ceil((m_enemy.attack_range - x) / m_speed));
        } else {
            retreat_time = 1;
        }
    } else {
        //Suppose enemy will go for us no longer than catch time
        retreat_time = BehaviourConfig::max_runaway_time;
    }
    if (retreat_time < 1) {
        retreat_time = 1;
    }

    int kill_time = m_enemy.rem_cooldown;
    //Enemy should come to attack range first
    if (x > m_enemy.attack_range && m_enemy.speed > 0) {
        kill_time += static_cast<int>(ceil((x - m_enemy.attack_range) / m_enemy.speed));
    }
    int atk_num = (m_life + m_enemy.damage - 1) / m_enemy.damage;
    kill_time += (atk_num - 1) * m_enemy.cooldown;
    if (kill_time < 1) {
        kill_time = 1;
    }

    return static_cast<double>(retreat_time) / kill_time;
}

}
