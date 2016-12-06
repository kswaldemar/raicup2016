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
    : MyLivingUnit(MyLivingUnit::STATIC, x, y, radius, max_life, max_life),
      m_damage(damage),
      m_attack_range(attack_range),
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

int MyBuilding::getDamage() const {
    return m_damage;
}

double MyBuilding::getAttackRange() const {
    return m_attack_range;
}

int MyBuilding::getCooldown() const {
    return m_cooldown;
}