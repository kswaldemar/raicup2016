//
// Created by valdemar on 04.12.16.
//

#include "PotentialField.h"

namespace fields {

PotentialField::PotentialField(const geom::Point2D &center,
                               double r1,
                               double r2)
    : m_center(center),
      m_r1(r1),
      m_r2(r2) {
}

double PotentialField::get_value(const geom::Point2D &pt) const {
    return get_value(pt.x, pt.y);
}

double PotentialField::get_value(double x, double y) const {
    geom::Vec2D dist = {x - m_center.x, y - m_center.y};
    double len = dist.sqr();
    if (len >= (m_r1 * m_r1) && len <= (m_r2 * m_r2)) {
        return calc_force(len);
    }
    return 0;
}

}