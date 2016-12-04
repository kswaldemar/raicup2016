//
// Created by valdemar on 11.11.16.
//

#include "FieldsDescription.h"
#include <cassert>

namespace fields {

ConstField::ConstField(const geom::Point2D &center, double r1, double r2, double force)
: PotentialField(center, r1, r2), m_force(force) {
    assert(r1 >= 0);
    assert(r2 >= 0);
}

double ConstField::calc_force(double sqr_x) const {
    return m_force;
}


LinearField::LinearField(const geom::Point2D &center, double r1, double r2, double force)
    : PotentialField(center, r1, r2), m_force(force) {

}

double LinearField::calc_force(double sqr_x) const {
    const double x = sqrt(sqr_x);
    return m_force - (m_force / m_r2) * x;
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
: PotentialField(center, r1, r2), m_is_attractive(is_attractive), m_k(conf.k), m_V(conf.V) {
    assert(r1 >= 0);
    assert(r2 >= 0);
}

double ExpRingField::calc_force(double sqr_x) const {
    double mod = m_V * exp(-sqrt(sqr_x) * m_k);
    return mod;
}

}
