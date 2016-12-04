//
// Created by valdemar on 04.12.16.
//

#include "MyBonus.h"
#include <algorithm>

MyBonus::MyBonus(double x, double y, int tick) : m_x(x), m_y(y), m_last_update_tick(tick) {
    m_next_time = 2502;
}

double MyBonus::getX() const {
    return m_x;
}

double MyBonus::getY() const {
    return m_y;
}

void MyBonus::update(int tick) {
    m_last_update_tick = tick;
}

void MyBonus::setTaken() {
    m_next_time += 2500;
    if (m_next_time >= 20000) {
        m_next_time = 100500;
    }
}

int MyBonus::getRemainingTime() const {
    return std::max(m_next_time - m_last_update_tick, 0);
}

geom::Point2D MyBonus::getPoint() const {
    return {m_x, m_y};
}
