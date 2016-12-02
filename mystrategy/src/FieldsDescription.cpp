//
// Created by valdemar on 11.11.16.
//

#include "FieldsDescription.h"
#include <cassert>

namespace fields {

ConstField::ConstField(const geom::Point2D &center, double r1, double r2, double force)
: IField(center), m_r1(r1 * r1), m_r2(r2 * r2), m_force(force) {
    assert(r1 >= 0);
    assert(r2 >= 0);
}

double ConstField::get_value(double x, double y) const {
    geom::Vec2D v(m_center.x - x, m_center.y - y);
    double dist = v.sqr();
    if (dist >= m_r1 && dist <= m_r2) {
        return m_force;
    }
    return 0;
}


ExpConfig ExpConfig::from_two_points(double curvature, double y, double x) {
    ExpConfig ret;
    ret.k = curvature;
    ret.V = y * exp(x * curvature);
    return ret;
}

ExpRingField::ExpRingField(const geom::Point2D &center,
                           double r1,
                           double r2,
                           bool is_attractive,
                           const ExpConfig &conf)
: IField(center), m_r1(r1 * r1), m_r2(r2 * r2), m_is_attractive(is_attractive), m_k(conf.k), m_V(conf.V) {
    assert(r1 >= 0);
    assert(r2 >= 0);
}


double ExpRingField::get_value(double x, double y) const {
    double dist = geom::sqr(m_center.x - x) + geom::sqr(m_center.y - y);
    if (dist >= m_r1 && dist <= m_r2) {
        double mod = m_V * exp(-sqrt(dist) * m_k);
        return mod;
    }
    return 0;
}

LinearField::LinearField(const geom::Point2D &center, double r1, double r2, double force)
: IField(center), m_r1(r1), m_r2(r2), m_force(force) {

}

double LinearField::get_value(double x, double y) const {
    geom::Vec2D v(m_center.x - x, m_center.y - y);
    double dist = v.len();
    if (dist >= m_r1 && dist <= m_r2) {
        return m_force - (m_force / m_r2) * dist;
    }
    return 0;
}


}
