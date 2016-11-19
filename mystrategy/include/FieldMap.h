//
// Created by valdemar on 19.11.16.
//

#pragma once

#include "VectorField.h"
#include <memory>
#include <list>

namespace fields {

class FieldMap {
public:

    enum Type {
        ADD, MAX, MIN
    };

    FieldMap(Type sum_rules);

    void add_field(std::unique_ptr<IVectorField> field);

    double get_value(double x, double y) const;

    void clear();
private:
    std::list<std::unique_ptr<IVectorField>> m_fields;
    Type m_rules;
};

}