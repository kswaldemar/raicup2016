//
// Created by valdemar on 04.12.16.
//

#include "MyLivingUnit.h"

MyLivingUnit::MyLivingUnit(MyLivingUnit::Type type_, const model::LivingUnit &unit)
    : m_type(type_),
      m_x(unit.getX()),
      m_y(unit.getY()),
      m_r(unit.getRadius()),
      m_id(unit.getId()),
      m_life(unit.getLife()),
      m_max_life(unit.getMaxLife())
{
}

MyLivingUnit::MyLivingUnit(MyLivingUnit::Type type, double x, double y, double radius, int life, int max_life)
    : m_type(type),
      m_x(x),
      m_y(y),
      m_r(radius),
      m_life(life),
      m_max_life(max_life) {

}

MyLivingUnit::Type MyLivingUnit::getType() const {
    return m_type;
}

double MyLivingUnit::getX() const {
    return m_x;
}

double MyLivingUnit::getY() const {
    return m_y;
}

double MyLivingUnit::getRadius() const {
    return m_r;
}

long long int MyLivingUnit::getId() const {
    return m_id;
}

int MyLivingUnit::getLife() const {
    return m_life;
}

int MyLivingUnit::getMaxLife() const {
    return m_max_life;
}

geom::Point2D MyLivingUnit::getPoint() const {
    return {m_x, m_y};
}
