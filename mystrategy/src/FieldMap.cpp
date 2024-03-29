//
// Created by valdemar on 19.11.16.
//

#include "FieldMap.h"

namespace fields {

FieldMap::FieldMap(FieldMap::Type sum_rules): m_rules(sum_rules) {

}

void FieldMap::add_field(std::unique_ptr<fields::PotentialField> field) {
    m_fields.push_back(std::move(field));
}

double FieldMap::get_value(double x, double y) const {
    double force = 0;
    for (const auto &fld : m_fields) {
        switch (m_rules) {
            case ADD:
                force += fld->get_value(x, y);
                break;
            case MAX:
                force = std::max(force, fld->get_value(x, y));
                break;
            case MIN:
                force = std::min(force, fld->get_value(x, y));
                break;
        }
    }
    return force;
}

double FieldMap::get_value(const geom::Point2D &pt) const {
    return get_value(pt.x, pt.y);
}

void FieldMap::clear() {
    m_fields.clear();
}


}