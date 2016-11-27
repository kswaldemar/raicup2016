//
// Created by valdemar on 26.11.16.
//

#include "CustomField.h"
#include <cassert>

namespace fields {

CustomField::CustomField(const geom::Point2D &center, double r1, double r2, CustomField::FuncType fn)
    : IField(center), m_r1(r1), m_r2(r2), m_custom_func(fn) {
}

double CustomField::get_value(double x, double y) const {
    assert(m_custom_func);
    const double dist = geom::Vec2D(m_center.x - x, m_center.y - y).len();
    if (dist >= m_r1 && dist <= m_r2) {
        return m_custom_func(dist);
    }
    return 0;
}

}