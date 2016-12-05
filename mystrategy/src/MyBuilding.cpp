//
// Created by valdemar on 05.12.16.
//

#include "Vec2D.h"
#include "MyBuilding.h"

MyBuilding::MyBuilding(double x,
                       double y,
                       double radius,
                       int damage,
                       double attack_range,
                       int max_life,
                       int cooldown)
    : m_x(x),
      m_y(y),
      m_radius(radius),
      m_damage(damage),
      m_attack_range(attack_range),
      m_life(max_life),
      m_max_life(max_life),
      m_cooldown(cooldown) {

    m_last_update = 0;
    m_last_shoot_tick = -m_cooldown;
}

void MyBuilding::update(const model::Building &building) {
    m_life = building.getLife();
    m_last_shoot_tick = m_last_update + building.getRemainingActionCooldownTicks() - m_cooldown;
}

void MyBuilding::updateClock(int current_tick) {
    m_last_update = current_tick;
}

bool MyBuilding::isSame(const model::Building &building) {
    return eps_equal(building.getX(), m_x) && eps_equal(building.getY(), m_y);
}

int MyBuilding::getRemainingActionCooldownTicks() const {
    return std::max(m_last_shoot_tick + m_cooldown - m_last_update, 0);
}

double MyBuilding::getX() const {
    return m_x;
}

double MyBuilding::getY() const {
    return m_y;
}

double MyBuilding::getRadius() const {
    return m_radius;
}

int MyBuilding::getDamage() const {
    return m_damage;
}

double MyBuilding::getAttackRange() const {
    return m_attack_range;
}

int MyBuilding::getLife() const {
    return m_life;
}

int MyBuilding::getMaxLife() const {
    return m_max_life;
}

int MyBuilding::getCooldown() const {
    return m_cooldown;
}

geom::Point2D MyBuilding::getPoint() const {
    return {m_x, m_y};
}
