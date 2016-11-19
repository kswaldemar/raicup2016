//
// Created by valdemar on 19.11.16.
//

#include "FieldMap.h"

namespace fields {

FieldMap::FieldMap(FieldMap::Type sum_rules): m_rules(sum_rules) {

}

void FieldMap::add_field(std::unique_ptr<fields::IVectorField> field) {
    m_fields.push_back(std::move(field));
}

double FieldMap::get_value(double x, double y) const {
    double force = 0;
    for (const auto &fld : m_fields) {
        switch (m_rules) {
            case ADD:
                force += fld->apply_force(x, y).len();
                break;
            case MAX:
                force = std::max(force, fld->apply_force(x, y).len());
                break;
            case MIN:
                force = std::min(force, fld->apply_force(x, y).len());
                break;
        }
    }
    return force;
}

void FieldMap::clear() {
    m_fields.clear();
}


}